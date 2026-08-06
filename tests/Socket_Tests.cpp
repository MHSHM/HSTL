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
