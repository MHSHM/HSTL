#include <catch2/catch_test_macros.hpp>

#include "Socket.h"
#include "Thread.h"
#include "Array.h"

#include <utility>
#include <cstring>

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

// A live connection, returned as the server side of it. out_client is the raw client handle and
// the caller closes it. The listener dies with this function, which an already-established
// connection is indifferent to.
static Socket connected_pair(SOCKET& out_client)
{
    sockaddr_in address{};
    Socket listener = listening_socket(address);

    out_client = connected_client(address);

    auto res = listener.accept();
    REQUIRE(res);

    return std::move(res.get_value());
}

// Reads exactly length bytes, looping because TCP may split a payload across several reads.
// False means the peer closed before that many arrived.
static bool read_exactly(SOCKET handle, char* buffer, size_t length)
{
    size_t filled = 0;

    while (filled < length)
    {
        int received = ::recv(handle, buffer + filled, static_cast<int>(length - filled), 0);

        if (received <= 0)
        {
            return false;
        }

        filled += static_cast<size_t>(received);
    }

    return true;
}

// How many bytes are sitting unread on handle. Unlike recv this never blocks, so it is the only
// safe way to assert that *nothing* arrived.
static u_long pending_bytes(SOCKET handle)
{
    u_long pending = 0;
    REQUIRE(ioctlsocket(handle, FIONREAD, &pending) == 0);

    return pending;
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

TEST_CASE("Socket close", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("releases the handle and empties the socket")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());
        SOCKET raw = sock.handle();

        sock.close();

        CHECK_FALSE(sock.is_valid());
        CHECK(sock.handle() == INVALID_SOCKET);

        sockaddr_in address{};
        int length = sizeof(address);
        CHECK(getsockname(raw, (sockaddr*)&address, &length) == SOCKET_ERROR);
        CHECK(WSAGetLastError() == WSAENOTSOCK);
    }

    SECTION("closing twice is harmless")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        sock.close();
        // The second call must not hand the freed handle back to closesocket — which is what
        // lets the destructor call close() without checking whether the caller already did.
        sock.close();

        CHECK_FALSE(sock.is_valid());
    }

    SECTION("closing an already-empty socket is harmless")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket source = std::move(res.get_value());
        Socket sink = std::move(source);

        source.close();

        CHECK_FALSE(source.is_valid());
        CHECK(sink.is_valid());
    }

    SECTION("a closed socket can still be move assigned into")
    {
        auto first_res = Socket::create();
        REQUIRE(first_res);
        Socket sock = std::move(first_res.get_value());
        sock.close();

        auto second_res = Socket::create();
        REQUIRE(second_res);
        Socket other = std::move(second_res.get_value());
        SOCKET raw = other.handle();

        sock = std::move(other);

        CHECK(sock.handle() == raw);
        CHECK(sock.is_valid());
    }
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

// NOTE: recv() blocks until at least one byte arrives, so every section below either has the
// peer write first, or expects an immediate failure, or waits on a close that is guaranteed to
// come. Reading with nothing in flight would hang the suite.
TEST_CASE("Socket recv", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("reads what the peer wrote")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        const char payload[] = "GET / HTTP/1.1";
        constexpr size_t LENGTH = sizeof(payload) - 1;
        REQUIRE(::send(client, payload, static_cast<int>(LENGTH), 0) == static_cast<int>(LENGTH));

        char buffer[64]{};
        auto res = server.recv(buffer, sizeof(buffer));

        REQUIRE(res);
        REQUIRE(res.get_value() == LENGTH);
        CHECK(memcmp(buffer, payload, LENGTH) == 0);

        closesocket(client);
    }

    SECTION("returns only what has arrived, not a full buffer")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        REQUIRE(::send(client, "ping", 4, 0) == 4);

        // A 4096-byte window over 4 bytes of traffic must come back as 4. recv() waits for the
        // first byte, never for the buffer to fill — which is exactly why a caller has to loop
        // until it decides a request is complete.
        char buffer[4096]{};
        auto res = server.recv(buffer, sizeof(buffer));

        REQUIRE(res);
        CHECK(res.get_value() == 4);

        closesocket(client);
    }

    SECTION("never exceeds the window it was given")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        REQUIRE(::send(client, "0123456789", 10, 0) == 10);

        char buffer[16]{};
        auto first = server.recv(buffer, 4);
        REQUIRE(first);
        REQUIRE(first.get_value() == 4);
        CHECK(memcmp(buffer, "0123", 4) == 0);

        // The untaken bytes stay queued on the socket and come back on the next read.
        auto second = server.recv(buffer, sizeof(buffer));
        REQUIRE(second);
        CHECK(second.get_value() == 6);
        CHECK(memcmp(buffer, "456789", 6) == 0);

        closesocket(client);
    }

    SECTION("returns 0 once the peer closes")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        closesocket(client);

        // A clean shutdown is success carrying a count of 0, not an error. This is the ordinary
        // end of every connection, so it must never look like a failure.
        char buffer[64]{};
        auto res = server.recv(buffer, sizeof(buffer));

        REQUIRE(res);
        CHECK(res.get_value() == 0);
    }

    SECTION("drains queued bytes before reporting the close")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        REQUIRE(::send(client, "last", 4, 0) == 4);
        closesocket(client);

        // The bytes were already in flight when the peer hung up, so they must be delivered
        // first; only the read after them sees the 0.
        char buffer[64]{};
        auto first = server.recv(buffer, sizeof(buffer));
        REQUIRE(first);
        CHECK(first.get_value() == 4);

        auto second = server.recv(buffer, sizeof(buffer));
        REQUIRE(second);
        CHECK(second.get_value() == 0);
    }

    SECTION("fails on a socket that was never connected")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        // NOTE: WSAENOTCONN, reported immediately — there is no peer to wait on.
        char buffer[16]{};
        CHECK_FALSE(sock.recv(buffer, sizeof(buffer)));
    }
}

TEST_CASE("Socket send", "[Socket]")
{
    Winsock_Scope winsock;
    REQUIRE(winsock.ok);

    SECTION("delivers the payload to the peer")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        const char payload[] = "HTTP/1.1 200 OK\r\n\r\n";
        constexpr size_t LENGTH = sizeof(payload) - 1;

        REQUIRE(server.send(payload, LENGTH));

        char buffer[64]{};
        REQUIRE(read_exactly(client, buffer, LENGTH));
        CHECK(memcmp(buffer, payload, LENGTH) == 0);

        closesocket(client);
    }

    SECTION("a zero length sends nothing and succeeds")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        CHECK(server.send("unused", 0));
        CHECK(pending_bytes(client) == 0);

        closesocket(client);
    }

    SECTION("a payload past the kernel's buffers still arrives whole")
    {
        SOCKET client = INVALID_SOCKET;
        Socket server = connected_pair(client);

        // Comfortably larger than any send buffer the OS hands out, so ::send is forced into
        // partial writes and send()'s loop has to do real work rather than succeeding in one go.
        constexpr size_t PAYLOAD_SIZE = 8 * 1024 * 1024;

        Array<char> payload(PAYLOAD_SIZE);
        for (size_t i = 0; i < PAYLOAD_SIZE; ++i)
        {
            payload[i] = static_cast<char>(i % 251);
        }

        size_t drained = 0;
        bool intact = true;

        // The peer has to drain concurrently. With nobody reading, the kernel buffers fill,
        // send() blocks forever, and the test deadlocks instead of failing.
        // NOTE: no Catch2 macros in here — they report by throwing, which off the main thread
        // would take down the process instead of failing a check.
        auto thread_res = Thread::create([&]()
        {
            char chunk[64 * 1024];

            while (drained < PAYLOAD_SIZE)
            {
                int received = ::recv(client, chunk, sizeof(chunk), 0);

                if (received <= 0)
                {
                    break;
                }

                for (int i = 0; i < received; ++i)
                {
                    if (chunk[i] != static_cast<char>((drained + i) % 251))
                    {
                        intact = false;
                    }
                }

                drained += static_cast<size_t>(received);
            }
        });

        REQUIRE(thread_res);
        Thread drainer = std::move(thread_res.get_value());

        CHECK(server.send(&payload[0], PAYLOAD_SIZE));

        // join() is the only synchronisation point here, so drained and intact are read after it.
        drainer.join();

        CHECK(drained == PAYLOAD_SIZE);
        CHECK(intact);

        closesocket(client);
    }

    SECTION("fails on a socket that was never connected")
    {
        auto res = Socket::create();
        REQUIRE(res);
        Socket sock = std::move(res.get_value());

        // NOTE: WSAENOTCONN — there is nowhere for the bytes to go.
        CHECK_FALSE(sock.send("payload", 7));
    }
}
