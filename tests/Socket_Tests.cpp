#include <catch2/catch_test_macros.hpp>

#include "Socket.h"

#include <utility>

using namespace hstl;

// WSAStartup / WSACleanup are reference counted, so one of these per TEST_CASE is safe even
// though Catch2 may run several in the same process.
struct Winsock_Scope
{
    bool ok{false};

    Winsock_Scope()
    {
        WSAData data{};
        ok = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~Winsock_Scope()
    {
        if (ok)
        {
            WSACleanup();
        }
    }
};

// Asks the kernel what a socket is actually bound to, so bind() is checked against reality
// rather than against the arguments it was handed.
static sockaddr_in bound_address_of(SOCKET handle)
{
    sockaddr_in address{};
    int length = sizeof(address);

    REQUIRE(getsockname(handle, (sockaddr*)&address, &length) == 0);

    return address;
}

// Brings up a listening socket on an OS-chosen loopback port and reports the address a client
// needs in order to reach it.
static Socket listening_socket(sockaddr_in& out_address)
{
    auto res = Socket::create();
    REQUIRE(res);
    Socket sock = std::move(res.get_value());

    REQUIRE(sock.bind(0, "127.0.0.1"));
    REQUIRE(sock.listen());

    out_address = bound_address_of(sock.handle());

    return sock;
}

// A raw client handle already connected to address. Deliberately not a Socket: these tests
// need a counterparty that owes nothing to the type under test. The caller closes it.
static SOCKET connected_client(const sockaddr_in& address)
{
    SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    REQUIRE(client != INVALID_SOCKET);
    REQUIRE(connect(client, (const sockaddr*)&address, sizeof(address)) == 0);

    return client;
}

TEST_CASE("Socket creation", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("create() yields a valid socket")
    {
        auto res = Socket::create();
        REQUIRE(res);
        CHECK(res.get_value().is_valid());
        CHECK(res.get_value().handle() != INVALID_SOCKET);
    }

    SECTION("adopt() takes ownership of an existing handle")
    {
        SOCKET raw = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        REQUIRE(raw != INVALID_SOCKET);

        auto res = Socket::adopt(raw);
        REQUIRE(res);
        CHECK(res.get_value().handle() == raw);
    }

    SECTION("adopt() rejects the invalid sentinel")
    {
        auto res = Socket::adopt(INVALID_SOCKET);
        CHECK_FALSE(res);
    }
}

TEST_CASE("Socket ownership transfer", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    auto res = Socket::create();
    REQUIRE(res);

    Socket original = std::move(res.get_value());
    SOCKET raw = original.handle();

    SECTION("move construction empties the source")
    {
        Socket moved = std::move(original);

        CHECK(moved.handle() == raw);
        CHECK(moved.is_valid());
        CHECK_FALSE(original.is_valid());
        CHECK(original.handle() == INVALID_SOCKET);
    }

    SECTION("move assignment steals the handle and empties the source")
    {
        auto other_res = Socket::create();
        REQUIRE(other_res);
        Socket destination = std::move(other_res.get_value());

        destination = std::move(original);

        CHECK(destination.handle() == raw);
        CHECK_FALSE(original.is_valid());
    }

    SECTION("move assignment closes the handle it is overwriting")
    {
        auto other_res = Socket::create();
        REQUIRE(other_res);
        Socket destination = std::move(other_res.get_value());
        SOCKET overwritten = destination.handle();

        destination = std::move(original);

        // The handle destination used to own must have been closed, not leaked.
        sockaddr_in address{};
        int length = sizeof(address);
        CHECK(getsockname(overwritten, (sockaddr*)&address, &length) == SOCKET_ERROR);
        CHECK(WSAGetLastError() == WSAENOTSOCK);
    }

    SECTION("self move assignment leaves the socket intact")
    {
        original = std::move(original);

        CHECK(original.is_valid());
        CHECK(original.handle() == raw);
    }
}

TEST_CASE("Socket destruction closes the handle exactly once", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SOCKET raw = INVALID_SOCKET;

    {
        auto res = Socket::create();
        REQUIRE(res);
        raw = res.get_value().handle();
    }

    sockaddr_in address{};
    int length = sizeof(address);
    CHECK(getsockname(raw, (sockaddr*)&address, &length) == SOCKET_ERROR);
    CHECK(WSAGetLastError() == WSAENOTSOCK);
}

TEST_CASE("Socket moved-from destruction is harmless", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SOCKET raw = INVALID_SOCKET;

    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket source = std::move(res.get_value());
        raw = source.handle();

        {
            Socket sink = std::move(source);
            CHECK(sink.handle() == raw);
        }

        // sink is gone and took the handle with it. source must not close it a second time
        // when it goes out of scope below.
        CHECK_FALSE(source.is_valid());

        sockaddr_in address{};
        int length = sizeof(address);
        CHECK(getsockname(raw, (sockaddr*)&address, &length) == SOCKET_ERROR);
    }
}

TEST_CASE("Socket bind", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("binds to the requested address, not just the port")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        // Port 0 asks the OS for any free port, which keeps the test from colliding with
        // whatever else is running on the machine.
        REQUIRE(sock.bind(0, "127.0.0.1"));

        auto address = bound_address_of(sock.handle());
        CHECK(address.sin_family == AF_INET);
        CHECK(address.sin_addr.s_addr == htonl(INADDR_LOOPBACK));
    }

    SECTION("defaults to every interface")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        REQUIRE(sock.bind(0));

        auto address = bound_address_of(sock.handle());
        CHECK(address.sin_addr.s_addr == htonl(INADDR_ANY));
    }

    SECTION("binds the requested port")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        REQUIRE(sock.bind(0, "127.0.0.1"));
        uint16_t chosen = ntohs(bound_address_of(sock.handle()).sin_port);
        REQUIRE(chosen != 0);

        // Re-binding the port the OS just handed out proves the port argument is honoured.
        auto second_res = Socket::create();
        REQUIRE(second_res);
        Socket second = std::move(second_res.get_value());

        REQUIRE(second.bind(chosen, "127.0.0.2"));
        CHECK(ntohs(bound_address_of(second.handle()).sin_port) == chosen);
    }

    SECTION("rejects a malformed address")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        CHECK_FALSE(sock.bind(0, "not.an.ip"));
        CHECK_FALSE(sock.bind(0, "999.1.1.1"));
        CHECK_FALSE(sock.bind(0, ""));
    }

    SECTION("fails when the address and port are already taken")
    {
        auto first_res = Socket::create();
        REQUIRE(first_res);
        Socket first = std::move(first_res.get_value());
        REQUIRE(first.bind(0, "127.0.0.1"));

        uint16_t taken = ntohs(bound_address_of(first.handle()).sin_port);

        auto second_res = Socket::create();
        REQUIRE(second_res);
        Socket second = std::move(second_res.get_value());

        CHECK_FALSE(second.bind(taken, "127.0.0.1"));
    }
}

TEST_CASE("Socket listen", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("succeeds on a bound socket")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        REQUIRE(sock.bind(0, "127.0.0.1"));
        CHECK(sock.listen());
    }

    SECTION("a listening socket actually accepts connections")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket listener = std::move(res.get_value());

        REQUIRE(listener.bind(0, "127.0.0.1"));
        REQUIRE(listener.listen());

        auto address = bound_address_of(listener.handle());

        // A client connecting to the listening socket must succeed. Before listen() the OS
        // would refuse this outright, so it is the observable difference listen() makes.
        SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        REQUIRE(client != INVALID_SOCKET);

        CHECK(connect(client, (sockaddr*)&address, sizeof(address)) == 0);

        closesocket(client);
    }

    SECTION("fails on an unbound socket")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        // NOTE: Winsock rejects listen() on a socket that was never bound. POSIX would assign
        // an ephemeral port instead, so this expectation is Windows-specific.
        CHECK_FALSE(sock.listen());
    }
}

// NOTE: accept() blocks when the queue is empty, so every case below either connects a client
// first — the connection is already queued, so accept() returns without waiting — or expects
// an immediate failure. A section that calls accept() with nothing pending would hang the
// whole suite rather than fail it.
TEST_CASE("Socket accept", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("hands back a fresh socket for a queued connection")
    {
        sockaddr_in address{};
        Socket listener = listening_socket(address);

        SOCKET client = connected_client(address);

        auto res = listener.accept();
        REQUIRE(res);
        Socket connection = std::move(res.get_value());

        CHECK(connection.is_valid());
        // The connection is its own socket; the listener survives untouched and keeps listening.
        CHECK(connection.handle() != listener.handle());
        CHECK(listener.is_valid());

        closesocket(client);
    }

    SECTION("the accepted socket is paired with the client that connected")
    {
        sockaddr_in address{};
        Socket listener = listening_socket(address);

        SOCKET client = connected_client(address);

        auto res = listener.accept();
        REQUIRE(res);
        Socket connection = std::move(res.get_value());

        // The server side's peer must be exactly the client's own local address, which is what
        // proves accept() returned a handle for *this* connection rather than some other socket.
        sockaddr_in client_local{};
        int length = sizeof(client_local);
        REQUIRE(getsockname(client, (sockaddr*)&client_local, &length) == 0);

        sockaddr_in peer{};
        length = sizeof(peer);
        REQUIRE(getpeername(connection.handle(), (sockaddr*)&peer, &length) == 0);

        CHECK(peer.sin_port == client_local.sin_port);
        CHECK(peer.sin_addr.s_addr == client_local.sin_addr.s_addr);

        closesocket(client);
    }

    SECTION("serves connections in turn without disturbing the listener")
    {
        sockaddr_in address{};
        Socket listener = listening_socket(address);

        SOCKET first_client = connected_client(address);
        SOCKET second_client = connected_client(address);

        auto first_res = listener.accept();
        REQUIRE(first_res);
        Socket first = std::move(first_res.get_value());

        auto second_res = listener.accept();
        REQUIRE(second_res);
        Socket second = std::move(second_res.get_value());

        // Two clients, two distinct connection sockets, and a listener still good for more.
        CHECK(first.handle() != second.handle());
        CHECK(first.is_valid());
        CHECK(second.is_valid());
        CHECK(bound_address_of(listener.handle()).sin_port == address.sin_port);

        closesocket(first_client);
        closesocket(second_client);
    }

    SECTION("the accepted socket closes independently of the listener")
    {
        sockaddr_in address{};
        Socket listener = listening_socket(address);

        SOCKET client = connected_client(address);
        SOCKET raw = INVALID_SOCKET;

        {
            auto res = listener.accept();
            REQUIRE(res);
            Socket connection = std::move(res.get_value());
            raw = connection.handle();
        }

        // The connection's handle is gone with it, but the listener is untouched.
        sockaddr_in probe{};
        int length = sizeof(probe);
        CHECK(getsockname(raw, (sockaddr*)&probe, &length) == SOCKET_ERROR);
        CHECK(listener.is_valid());
        CHECK(getsockname(listener.handle(), (sockaddr*)&probe, &length) == 0);

        closesocket(client);
    }

    SECTION("fails on a socket that is not listening")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        // NOTE: WSAEINVAL, and Winsock reports it immediately rather than blocking — accept()
        // only ever waits on a socket that reached the listening state.
        CHECK_FALSE(sock.accept());
    }
}
