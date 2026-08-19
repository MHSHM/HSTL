#pragma once

#include "Str.h"
#include "Array.h"
#include "Memory.h"
#include "Socket.h"

#include <cctype>
#include <cstring>
#include <cstdint>
#include <utility>

namespace hstl
{
	enum class HTTP_METHOD : uint8_t
	{
		HTTP_GET,
		HTTP_POST,
		HTTP_PUT,
		HTTP_DELETE,
		HTTP_HEAD,
		HTTP_PATCH,
		HTTP_OPTIONS,
		HTTP_UNKNOWN
	};

	enum class HTTP_VERSION : uint8_t
	{
		HTTP_1_0,
		HTTP_1_1,
		HTTP_UNKNOWN
	};

	enum class HTTP_STATUS_CODE : uint16_t
	{
		HTTP_200 = 200,
		HTTP_201 = 201,
		HTTP_204 = 204,

		HTTP_301 = 301,
		HTTP_304 = 304,

		HTTP_400 = 400,
		HTTP_403 = 403,
		HTTP_404 = 404,
		HTTP_405 = 405,
		HTTP_408 = 408,
		HTTP_413 = 413,
		HTTP_414 = 414,
		HTTP_431 = 431,

		HTTP_500 = 500,
		HTTP_501 = 501,
		HTTP_505 = 505
	};

	struct HTTP_Header
	{
		Str_View name;
		Str_View value;
	};

	class HTTP_Request
	{
	private:
		Str_View _target;
		Str_View _body;
		Array<HTTP_Header> _headers;
		HTTP_METHOD _method{HTTP_METHOD::HTTP_UNKNOWN};
		HTTP_VERSION _version{HTTP_VERSION::HTTP_UNKNOWN};

	public:
		HTTP_Request(Allocator* allocator):
			_headers{allocator} {}

	public:
		// NOTE: parse_request() re-reads a growing buffer from the start on every pass, and
		// set_header() appends - so without this a half-arrived request would push its headers
		// again on each retry. _headers.clear() keeps the capacity, which is why a reused
		// request stops allocating after the first one.
		void clear()
		{
			_target = Str_View{};
			_body = Str_View{};
			_headers.clear();
			_method = HTTP_METHOD::HTTP_UNKNOWN;
			_version = HTTP_VERSION::HTTP_UNKNOWN;
		}

	public:
		void set_target(const char* start, size_t length) { _target = Str_View{start, length}; }
		void set_body(const char* start, size_t length) { _body = Str_View{start, length}; }
		void set_header(const HTTP_Header& header) { _headers.push(header); }
		void set_method(HTTP_METHOD method) { _method = method; }
		void set_version(HTTP_VERSION version) { _version = version; }

	public:
		Str_View target() const { return _target; }
		Str_View body() const { return _body; }
		HTTP_METHOD method() const { return _method; }
		HTTP_VERSION version() const { return _version; }
		const Array<HTTP_Header>& headers() const { return _headers; }

	public:
		const HTTP_Header* find_header(Str_View name) const
		{
			for (const auto& header : _headers)
			{
				if (header.name.count() != name.count())
					continue;

				bool matched = true;
				for (size_t i = 0; i < name.count(); ++i)
				{
					// NOTE: tolower takes an int and is UB on a negative char, so the bytes
					// are widened through unsigned char first.
					auto lhs = tolower(static_cast<unsigned char>(header.name[i]));
					auto rhs = tolower(static_cast<unsigned char>(name[i]));

					if (lhs != rhs)
					{
						matched = false;
						break;
					}
				}

				if (matched)
				{
					return &header;
				}
			}

			return nullptr;
		}
	};

	class HTTP_Response
	{
	private:
		Str_View _body;
		Array<HTTP_Header> _headers;
		HTTP_STATUS_CODE _status_code{HTTP_STATUS_CODE::HTTP_200};

	public:
		HTTP_Response(Allocator* allocator):
			_headers{allocator} {}

	public:
		void set_status_code(HTTP_STATUS_CODE status_code) { _status_code = status_code; }
		void set_body(Str_View body) { _body = body; }
		void set_header(const HTTP_Header& header) { _headers.push(header); }

	public:
		Str_View body() const { return _body; }
		HTTP_STATUS_CODE status_code() const { return _status_code; }
		const Array<HTTP_Header>& headers() const { return _headers; }
	};

	using HTTP_Handler = HTTP_Response (*)(const HTTP_Request&);

	struct HTTP_Route
	{
		Str_View path;
		HTTP_Handler handler;
		HTTP_METHOD method;
	};

	enum class HTTP_PARSE_STATUS : uint8_t
	{
		// The whole request is present and out_request is filled in.
		HTTP_COMPLETE,
		// There are not enough bytes yet. Read more and call again.
		HTTP_INCOMPLETE,
		// These bytes will never become a valid request. Answer 400 and close.
		HTTP_MALFORMED
	};

	static HTTP_METHOD parse_method(Str_View text)
	{
		if (text == "GET")     return HTTP_METHOD::HTTP_GET;
		if (text == "POST")    return HTTP_METHOD::HTTP_POST;
		if (text == "PUT")     return HTTP_METHOD::HTTP_PUT;
		if (text == "DELETE")  return HTTP_METHOD::HTTP_DELETE;
		if (text == "HEAD")    return HTTP_METHOD::HTTP_HEAD;
		if (text == "PATCH")   return HTTP_METHOD::HTTP_PATCH;
		if (text == "OPTIONS") return HTTP_METHOD::HTTP_OPTIONS;

		return HTTP_METHOD::HTTP_UNKNOWN;
	}

	static HTTP_VERSION parse_version(Str_View text)
	{
		if (text == "HTTP/1.1") return HTTP_VERSION::HTTP_1_1;
		if (text == "HTTP/1.0") return HTTP_VERSION::HTTP_1_0;

		return HTTP_VERSION::HTTP_UNKNOWN;
	}

	static HTTP_PARSE_STATUS parse_request(const char* buffer, size_t length, HTTP_Request& out_request)
	{
		assert(buffer != nullptr);

		out_request.clear();

		auto payload = Str_View{buffer, length};

		auto request_line_end = payload.find("\r\n");
		if (request_line_end == Str_View::npos)
		{
			return HTTP_PARSE_STATUS::HTTP_INCOMPLETE;
		}

		auto request_line = payload.substr(0, request_line_end);

		const size_t method_end = request_line.find(' ');
		if (method_end == Str_View::npos)
		{
			return HTTP_PARSE_STATUS::HTTP_MALFORMED;
		}

		auto method = request_line.substr(0, method_end);

		const size_t target_start = method_end + 1;
		auto remainder = request_line.substr(target_start, request_line.count() - target_start);

		const size_t target_end = remainder.find(' ');
		if (target_end == Str_View::npos)
		{
			return HTTP_PARSE_STATUS::HTTP_MALFORMED;
		}

		auto target = remainder.substr(0, target_end);

		const size_t version_start = target_end + 1;
		auto version = remainder.substr(version_start, remainder.count() - version_start);

		if (version.find(' ') != Str_View::npos)
		{
			return HTTP_PARSE_STATUS::HTTP_MALFORMED;
		}

		out_request.set_method(parse_method(method));
		out_request.set_target(target.data(), target.count());
		out_request.set_version(parse_version(version));

		auto tail = payload.substr(request_line_end, length - request_line_end);

		const size_t blank_line = tail.find("\r\n\r\n");
		if (blank_line == Str_View::npos)
		{
			return HTTP_PARSE_STATUS::HTTP_INCOMPLETE;
		}

		const size_t headers_start = request_line_end + 2;
		const size_t headers_end = request_line_end + blank_line + 2;
		auto headers = payload.substr(headers_start, headers_end - headers_start);

		size_t cursor = 0;
		while (cursor < headers.count())
		{
			auto rest = headers.substr(cursor, headers.count() - cursor);

			const size_t line_end = rest.find("\r\n");
			assert(line_end != Str_View::npos);

			auto line = rest.substr(0, line_end);

			auto header_col = line.find(':');
			if (header_col == Str_View::npos)
			{
				return HTTP_PARSE_STATUS::HTTP_MALFORMED;
			}

			auto name = line.substr(0, header_col);

			const size_t value_start = header_col + 1;
			auto value = line.substr(value_start, line.count() - value_start);

			// trim forward white spaces
			size_t value_begin = 0;
			while (value_begin < value.count() && (value[value_begin] == ' ' || value[value_begin] == '\t'))
			{
				++value_begin;
			}

			value = value.substr(value_begin, value.count() - value_begin);

			// trim backward white spaces
			size_t value_end = value.count();
			while (value_end > 0 && (value[value_end - 1] == ' ' || value[value_end - 1] == '\t'))
			{
				--value_end;
			}

			value = value.substr(0, value_end);

			out_request.set_header(HTTP_Header{name, value});

			cursor += line_end + 2;
		}

		const size_t body_start = request_line_end + blank_line + 4;

		// If the content length is not present we assume the request is complete but has no body
		const HTTP_Header* content_length = out_request.find_header("Content-Length");
		if (content_length == nullptr)
		{
			// TODO: Transfer-Encoding
			out_request.set_body(buffer + body_start, 0);
			return HTTP_PARSE_STATUS::HTTP_COMPLETE;
		}

		auto digits = content_length->value;
		if (digits.count() == 0)
		{
			return HTTP_PARSE_STATUS::HTTP_MALFORMED;
		}

		size_t body_length = 0;
		for (size_t i = 0; i < digits.count(); ++i)
		{
			const char c = digits[i];
			if (c < '0' || c > '9')
			{
				return HTTP_PARSE_STATUS::HTTP_MALFORMED;
			}

			const size_t digit = size_t(c - '0');

			// NOTE: overflow test
			// body_length * 10 + digit > SIZE_MAX -> overflow, divide both side by 10
			if (body_length > (SIZE_MAX - digit) / 10)
			{
				return HTTP_PARSE_STATUS::HTTP_MALFORMED;
			}

			body_length = body_length * 10 + digit;
		}

		if (length - body_start < body_length)
		{
			return HTTP_PARSE_STATUS::HTTP_INCOMPLETE;
		}

		out_request.set_body(buffer + body_start, body_length);

		return HTTP_PARSE_STATUS::HTTP_COMPLETE;
	}

	static constexpr size_t MAX_DECIMAL_DIGITS = 20;

	static size_t write_decimal(char* out, size_t value)
	{
		char reversed[MAX_DECIMAL_DIGITS];
		size_t count = 0;

		do
		{
			reversed[count] = static_cast<char>('0' + (value % 10));
			++count;
			value /= 10;
		}
		while (value > 0);

		for (size_t i = 0; i < count; ++i)
		{
			out[i] = reversed[count - 1 - i];
		}

		return count;
	}

	static Str_View status_code_to_str(HTTP_STATUS_CODE status_code)
	{
		switch (status_code)
		{
			case HTTP_STATUS_CODE::HTTP_200: return "200 OK";
			case HTTP_STATUS_CODE::HTTP_201: return "201 Created";
			case HTTP_STATUS_CODE::HTTP_204: return "204 No Content";

			case HTTP_STATUS_CODE::HTTP_301: return "301 Moved Permanently";
			case HTTP_STATUS_CODE::HTTP_304: return "304 Not Modified";

			case HTTP_STATUS_CODE::HTTP_400: return "400 Bad Request";
			case HTTP_STATUS_CODE::HTTP_403: return "403 Forbidden";
			case HTTP_STATUS_CODE::HTTP_404: return "404 Not Found";
			case HTTP_STATUS_CODE::HTTP_405: return "405 Method Not Allowed";
			case HTTP_STATUS_CODE::HTTP_408: return "408 Request Timeout";
			case HTTP_STATUS_CODE::HTTP_413: return "413 Content Too Large";
			case HTTP_STATUS_CODE::HTTP_414: return "414 URI Too Long";
			case HTTP_STATUS_CODE::HTTP_431: return "431 Request Header Fields Too Large";

			case HTTP_STATUS_CODE::HTTP_500: return "500 Internal Server Error";
			case HTTP_STATUS_CODE::HTTP_501: return "501 Not Implemented";
			case HTTP_STATUS_CODE::HTTP_505: return "505 HTTP Version Not Supported";
		}

		return "500 Internal Server Error";
	}

	class HTTP_Server
	{
	private:
		Socket _listener;
		Array<HTTP_Route> _routes;
		Allocator* _allocator{nullptr};
		bool _owns_winsock{false};

	private:
		HTTP_Server(Socket&& listener, Allocator* allocator):
			_listener{std::move(listener)},
			_routes{allocator},
			_allocator{allocator},
			_owns_winsock{true}
		{

		}

	private:
		static constexpr size_t REQUEST_BUFFER_SIZE = 8192;

		// Reads one request off connection and answers it.
		void serve(Socket& connection)
		{
			// NOTE: one buffer per connection
			char buffer[REQUEST_BUFFER_SIZE];
			size_t received = 0;

			HTTP_Request request{_allocator};

			while (true)
			{
				const auto status = parse_request(buffer, received, request);

				if (status == HTTP_PARSE_STATUS::HTTP_COMPLETE)
				{
					break;
				}

				if (status == HTTP_PARSE_STATUS::HTTP_MALFORMED)
				{
					// TODO: answer 400 before closing.
					return;
				}

				const size_t free_space = REQUEST_BUFFER_SIZE - received;
				if (free_space == 0)
				{
					// NOTE: Socket::recv asserts on a zero length, so a full buffer has to be
					// caught here - it can never be handed on as a read of nothing.
					// TODO: answer 431 before closing.
					return;
				}

				auto recv_res = connection.recv(buffer + received, free_space);
				if (!recv_res)
				{
					// The connection broke mid-request. There is no longer anyone to answer.
					return;
				}

				const size_t bytes = recv_res.get_value();
				if (bytes == 0)
				{
					// A clean close, but a truncated request - the client left before finishing.
					return;
				}

				received += bytes;
			}

			// TODO: dispatch the request to its route and send the response.
		}

	private:
		static constexpr size_t RESPONSE_HEAD_SIZE = 2048;

		Result<void> send_response(Socket& connection, const HTTP_Response& response)
		{
			char head[RESPONSE_HEAD_SIZE];
			size_t written = 0;

			auto append = [&](Str_View piece) -> bool
			{
				if (piece.count() > RESPONSE_HEAD_SIZE - written)
				{
					return false;
				}

				memcpy(head + written, piece.data(), piece.count());
				written += piece.count();

				return true;
			};

			char digits[MAX_DECIMAL_DIGITS];
			const size_t body_length = response.body().count();

			bool fits =
				append("HTTP/1.1 ") &&
				append(status_code_to_str(response.status_code())) &&
				append("\r\n") &&
				append("Content-Length: ") &&
				append(Str_View{digits, write_decimal(digits, body_length)}) &&
				append("\r\n") &&
				append("Connection: close\r\n");

			for (const auto& header : response.headers())
			{
				fits = fits &&
					append(header.name) &&
					append(": ") &&
					append(header.value) &&
					append("\r\n");
			}

			fits = fits && append("\r\n");

			if (!fits)
			{
				return Err("Response head is larger than {} bytes", RESPONSE_HEAD_SIZE);
			}

			auto head_res = connection.send(head, written);
			if (!head_res)
			{
				return head_res.get_err();
			}

			if (body_length > 0)
			{
				return connection.send(response.body().data(), body_length);
			}

			return {};
		}

	public:
		HTTP_Server(const HTTP_Server&) = delete;
		HTTP_Server& operator=(const HTTP_Server&) = delete;

		HTTP_Server(HTTP_Server&& source):
			_listener{std::move(source._listener)},
			_routes{std::move(source._routes)},
			_allocator{source._allocator},
			_owns_winsock{source._owns_winsock}
		{
			source._owns_winsock = false;
		}

		HTTP_Server& operator=(HTTP_Server&& source)
		{
			if (this == &source)
			{
				return *this;
			}

			// NOTE: the socket goes before WSACleanup(). Winsock is reference counted, and once
			// the last reference is released closesocket() fails with WSANOTINITIALISED and the
			// handle leaks
			_listener.close();

			if (_owns_winsock)
			{
				WSACleanup();
			}

			_listener = std::move(source._listener);
			_routes = std::move(source._routes);
			_allocator = source._allocator;
			_owns_winsock = source._owns_winsock;

			source._owns_winsock = false;

			return *this;
		}

		~HTTP_Server()
		{
			// NOTE: a destructor body runs before its members are destroyed, so leaving the
			// listener to ~Socket() would close it after WSACleanup() had already torn Winsock
			// down.
			_listener.close();

			if (_owns_winsock)
			{
				WSACleanup();
				_owns_winsock = false;
			}
		}

	public:
		static Result<HTTP_Server> create(uint16_t port, Str_View ip_address = "0.0.0.0" /*must be null-terminated*/, Allocator* allocator = Default_Allocator::get())
		{
			WSAData winsock_data{};

			const int startup_error = WSAStartup(MAKEWORD(2, 2), &winsock_data);
			if (startup_error != 0)
			{
				return Err("Failed to initialize Winsock: {}", startup_error);
			}

			auto listener_res = Socket::create();
			if (!listener_res)
			{
				WSACleanup();
				return listener_res.get_err();
			}

			Socket listener = std::move(listener_res.get_value());

			auto bind_res = listener.bind(port, ip_address);
			if (!bind_res)
			{
				// NOTE: the socket goes before WSACleanup().
				listener.close();
				WSACleanup();
				return bind_res.get_err();
			}

			auto listen_res = listener.listen();
			if (!listen_res)
			{
				listener.close();
				WSACleanup();
				return listen_res.get_err();
			}

			return HTTP_Server{std::move(listener), allocator};
		}

	public:
		void add_route(HTTP_METHOD method, Str_View path, HTTP_Handler handler)
		{
			assert(handler != nullptr);
			assert(path.count() > 0);

			_routes.push(HTTP_Route{path, handler, method});
		}

		const HTTP_Route* find_route(HTTP_METHOD method, Str_View path) const
		{
			for (const auto& route : _routes)
			{
				if (route.method != method)
				{
					continue;
				}

				if (route.path == path)
				{
					return &route;
				}
			}

			return nullptr;
		}

		const Array<HTTP_Route>& routes() const { return _routes; }

	public:
		Result<void> run()
		{
			while (true)
			{
				auto socket_res = _listener.accept();
				if (!socket_res)
				{
					return socket_res.get_err();
				}

				serve(socket_res.get_value());
			}
		}
	};
};
