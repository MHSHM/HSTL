#pragma once

#include "Str.h"

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

	struct HTTP_Header
	{
		Str_View name;
		Str_View value;
	};

	struct HTTP_Request
	{
		Str_View target;
		Str_View body;
		const HTTP_Header* headers;
		uint32_t headers_count;
		HTTP_METHOD method;
		HTTP_VERSION version;
	};
};
