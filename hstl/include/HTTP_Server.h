#pragma once

#include "Str.h"
#include "Hash_Map.h"
#include "Memory.h"
#include "Result.h"

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
			HTTP_Request request;
			request.headers.reassign_allocator(allocator);
		}
	};
};