#include <catch2/catch_test_macros.hpp>

#include "HTTP_Server.h"

#include <cstring>

using namespace hstl;

// parse_request() never touches a socket - the caller feeds it bytes and it reports whether they
// add up to a whole request. So these are pure string-in, status-out tests: no Winsock, no
// listener, nothing that can block the suite.

// The request under test, spelled with explicit CRLFs. Adjacent string literals concatenate, so a
// multi-line literal here is still one contiguous buffer with no hidden newlines.
static HTTP_PARSE_STATUS parse(const char* text, HTTP_Request& out_request)
{
    return parse_request(text, strlen(text), out_request);
}

TEST_CASE("parse_request reads a complete GET")
{
    const char* text =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Accept: text/html\r\n"
        "\r\n";

    HTTP_Request request{Default_Allocator::get()};

    REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);

    REQUIRE(request.method() == HTTP_METHOD::HTTP_GET);
    REQUIRE(request.target() == "/index.html");
    REQUIRE(request.version() == HTTP_VERSION::HTTP_1_1);

    REQUIRE(request.headers().count() == 2);

    const HTTP_Header* host = request.find_header("Host");
    REQUIRE(host != nullptr);
    REQUIRE(host->value == "localhost");

    // No Content-Length, so there is no body at all - and an empty view, not a null one.
    REQUIRE(request.body().count() == 0);
}

TEST_CASE("parse_request accepts complete requests")
{
    HTTP_Request request{Default_Allocator::get()};

    SECTION("a request with no headers at all")
    {
        // The request line's own CRLF is the first half of the blank line here, which is the
        // case the blank-line search has to start at request_line_end rather than past it.
        REQUIRE(parse("GET / HTTP/1.1\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.headers().count() == 0);
        REQUIRE(request.body().count() == 0);
    }

    SECTION("a POST whose body is exactly as long as it promised")
    {
        const char* text =
            "POST /submit HTTP/1.1\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "hello world";

        REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.method() == HTTP_METHOD::HTTP_POST);
        REQUIRE(request.body() == "hello world");
    }

    SECTION("a body followed by the start of the next request")
    {
        // A pipelining client may send the next request straight after this body. Extra bytes
        // are not an error, and the body must stop at Content-Length rather than run to the end
        // of the buffer.
        const char* text =
            "POST /submit HTTP/1.1\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "hello world"
            "GET / HTTP/1.1\r\n\r\n";

        REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.body() == "hello world");
    }

    SECTION("header values are stripped of the whitespace around them")
    {
        const char* text =
            "GET / HTTP/1.1\r\n"
            "Host:   localhost  \r\n"
            "Accept:\ttext/html\t\r\n"
            "X-Empty:   \r\n"
            "\r\n";

        REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.find_header("Host")->value == "localhost");
        REQUIRE(request.find_header("Accept")->value == "text/html");

        // An all-whitespace value trims down to nothing rather than walking off the end.
        REQUIRE(request.find_header("X-Empty")->value.count() == 0);
    }

    SECTION("header names are matched without regard to case")
    {
        REQUIRE(parse("GET / HTTP/1.1\r\ncontent-length: 2\r\n\r\nhi", request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.body() == "hi");
        REQUIRE(request.find_header("CONTENT-LENGTH") != nullptr);
    }

    SECTION("an unknown method or version parses, it does not fail")
    {
        // The request line is well formed, so this is a 501/505 for the server to answer - not
        // a 400. The parser's job is only to report what it saw.
        REQUIRE(parse("BREW /coffee HTTP/1.1\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.method() == HTTP_METHOD::HTTP_UNKNOWN);

        REQUIRE(parse("GET / HTTP/9.9\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);
        REQUIRE(request.version() == HTTP_VERSION::HTTP_UNKNOWN);
    }
}

TEST_CASE("parse_request asks for more bytes when the request is cut short")
{
    HTTP_Request request{Default_Allocator::get()};

    SECTION("the request line has not finished arriving")
    {
        REQUIRE(parse("GET /index.html HT", request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);
    }

    SECTION("the headers have not been terminated by a blank line")
    {
        REQUIRE(parse("GET / HTTP/1.1\r\nHost: localhost\r\n", request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);
    }

    SECTION("only part of the promised body has arrived")
    {
        const char* text =
            "POST /submit HTTP/1.1\r\n"
            "Content-Length: 11\r\n"
            "\r\n"
            "hello";

        REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);
    }
}

TEST_CASE("parse_request can be called repeatedly on a growing buffer")
{
    // This is what the server actually does: recv into one buffer, re-parse the whole thing from
    // the start, repeat. The same HTTP_Request is filled in on every pass, and set_header()
    // appends - so without the clear() at the top of parse_request, each attempt would push
    // another copy of every header.
    const char* text =
        "POST /submit HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";

    const size_t through_request_line = strlen("POST /submit HTTP/1.1\r\n");
    const size_t through_headers = strlen("POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 11\r\n\r\n");

    HTTP_Request request{Default_Allocator::get()};

    // Mid request line: no CRLF has arrived at all.
    REQUIRE(parse_request(text, 10, request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);

    // The request line is whole, but the header block has not been terminated.
    REQUIRE(parse_request(text, through_request_line, request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);

    // Headers are whole and were read for the first time here - but none of the body has come.
    REQUIRE(parse_request(text, through_headers, request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);
    REQUIRE(request.headers().count() == 2);

    // Five of the eleven promised bytes. The headers get read a second time on this pass.
    REQUIRE(parse_request(text, through_headers + 5, request) == HTTP_PARSE_STATUS::HTTP_INCOMPLETE);
    REQUIRE(request.headers().count() == 2);

    REQUIRE(parse_request(text, strlen(text), request) == HTTP_PARSE_STATUS::HTTP_COMPLETE);

    // Three passes read the header block; the request must still hold exactly one of each.
    REQUIRE(request.headers().count() == 2);
    REQUIRE(request.find_header("Host")->value == "localhost");
    REQUIRE(request.find_header("Content-Length")->value == "11");
    REQUIRE(request.body() == "hello world");
}

TEST_CASE("parse_request rejects requests that can never become valid")
{
    HTTP_Request request{Default_Allocator::get()};

    SECTION("the request line is missing a field")
    {
        REQUIRE(parse("GET\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
        REQUIRE(parse("GET /\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
    }

    SECTION("the request line carries a fourth field")
    {
        REQUIRE(parse("GET / HTTP/1.1 extra\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
    }

    SECTION("a header line has no colon")
    {
        REQUIRE(parse("GET / HTTP/1.1\r\nBadHeader\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
    }

    SECTION("Content-Length is not a number")
    {
        REQUIRE(parse("POST / HTTP/1.1\r\nContent-Length: abc\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
        REQUIRE(parse("POST / HTTP/1.1\r\nContent-Length: -1\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
        REQUIRE(parse("POST / HTTP/1.1\r\nContent-Length:\r\n\r\n", request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
    }

    SECTION("Content-Length does not fit in a size_t")
    {
        // 26 nines. The digit loop has to catch this before the multiply wraps it into some
        // small, plausible-looking value.
        const char* text =
            "POST / HTTP/1.1\r\n"
            "Content-Length: 99999999999999999999999999\r\n"
            "\r\n";

        REQUIRE(parse(text, request) == HTTP_PARSE_STATUS::HTTP_MALFORMED);
    }
}
