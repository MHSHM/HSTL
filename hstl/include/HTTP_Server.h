#pragma once

#include "Str.h"
#include "Array.h"
#include "Memory.h"

#include <cctype>
#include <cstdint>

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
		// Written by the parser
		void set_target(const char* start, size_t length) { _target = Str_View{start, length}; }
		void set_body(const char* start, size_t length) { _body = Str_View{start, length}; }
		void set_header(const HTTP_Header& header) { _headers.push(header); }
		void set_method(HTTP_METHOD method) { _method = method; }
		void set_version(HTTP_VERSION version) { _version = version; }

	public:
		// NOTE: The views point into the connection's receive buffer and are only valid for
		// the duration of the handler call.
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
};
