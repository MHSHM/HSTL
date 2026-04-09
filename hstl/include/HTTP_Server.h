#pragma once

#include "Str.h"
#include "Hash_Map.h"
#include "Memory.h"
#include "Result.h"
#include "Log.h"

#include <assert.h>

namespace hstl
{
	class HTTP_Request
	{
	private:
		struct HTTP_Header
		{
			Str_View key;
			Str_View value;
		};

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
};