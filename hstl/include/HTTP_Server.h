#pragma once

#include "Str.h"
#include "Hash_Map.h"
#include "Memory.h"
#include "Result.h"
#include "Log.h"

#include <assert.h>

// The following is a summary of the connection between the server and the networking layer...
//    1. The server asks the operating system to open a "socket" on a specific port, like 8080. It then sits and listens for incoming traffic.
//    2. A client (like a web browser) reaches out to that IP address and port. The OS accepts the connection and gives the server a unique handle to talk directly to that specific client.
//    3. The client sends the HTTP request as a stream of bytes over this connection. The server uses a network function (like recv()) to read these bytes into a memory buffer.
//    4. That memory buffer contains the raw http request ("POST /api/login HTTP/1.1\r\n..."). The server passes this buffer directly into the HTTP_Request::parse() method to parse it.
//    5. The server takes the newly parsed request.route (e.g., "/api/login") and looks it up in its internal routing table to find the developer's custom function (e.g., login_handler).
//    6. The server calls this custom function, passing in the parsed HTTP_Request and an empty HTTP_Response for the handler to fill out.
//    7. The handler processes the request, fills out the HTTP_Response (e.g., response_code=200, body="Login successful"), and returns.

namespace hstl
{
	struct HTTP_Header
	{
		Str_View key;
		Str_View value;
	};

	class HTTP_Request
	{
	private:
		Str_View method;
		Str_View route;
		Str_View protocol;
		Str_View body;
		Array<HTTP_Header> headers;

	public:
		static Result<HTTP_Request> parse(Str_View raw_http, Allocator* allocator = Default_Allocator::get())
		{
			/*
				POST /api/login HTTP/1.1\r\n
				Host: example.com\r\n
				User-Agent: curl/7.68.0\r\n
				Content-Type: application/json\r\n
				Content-Length: 36\r\n
				\r\n
				{"username":"admin","password":"123"}
			*/

			HTTP_Request request;
			request.headers.reassign_allocator(allocator);

			auto first_line_end = raw_http.find('\n');
			auto first_line = raw_http.substr(0, first_line_end);
			auto first_line_parts = first_line.split(' ');

			assert(first_line_parts.count() == 3 && "Invalid HTTP request: First line must contain method, route and protocol");
			if (first_line_parts.count() != 3)
				return Err("Invalid HTTP request: First line must contain method, route and protocol");

			request.method   = first_line_parts[0];
			request.route    = first_line_parts[1];
			request.protocol = first_line_parts[2];

			auto rest_of_body = Str_View{raw_http.data() + first_line_end + 1u, raw_http.count() - (first_line_end + 1u)};
			auto header_end = rest_of_body.find('\n');
			auto header = rest_of_body.substr(0, header_end);

			while (header.count() > 1)
			{
				auto header_parts = header.split(':');
				if (header_parts[1][0] == ' ')
					header_parts[1] = header_parts[1].substr(1, header_parts[1].count() - 1u); // Remove leading space if any
				if (header_parts[1].ends_with("\r"))
					header_parts[1] = header_parts[1].substr(0, header_parts[1].count() - 1u); // Remove trailing \r if any

				request.headers.push({header_parts[0], header_parts[1]});

				rest_of_body = Str_View{rest_of_body.data() + header_end + 1u, rest_of_body.count() - (header_end + 1u)};
				header_end = rest_of_body.find('\n');
				header = rest_of_body.substr(0, header_end);
			}

			request.body = Str_View{rest_of_body.data() + 1u, rest_of_body.count() - 1u}; // Remove the leading \n from body

			return request;
		}
	};

	struct HTTP_Response
	{
		uint32_t response_code;
		Array<HTTP_Header> headers;
		Str body;
	};

	using Route_Handler = void(*)(const HTTP_Request& request, HTTP_Response& response);

	struct Route_Key
	{
		Str_View route;
		Str_View method;

		bool operator==(const Route_Key& other) const
		{
			return route == other.route && method == other.method;
		}

		std::size_t operator()(const Route_Key& key) const
		{
			std::size_t h1 = std::hash<const char*>{}(key.route.data());
			std::size_t h2 = std::hash<const char*>{}(key.method.data());
			return h1 ^ (h2 << 1);
		}
	};

	class HTTP_Server
	{
	private:
		Hash_Map<Route_Key, Route_Handler> routes;
	};
};