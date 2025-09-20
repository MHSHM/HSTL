#pragma once

#include "Result.h"

#include <vector>
#include <string>
#include <string_view>

namespace hstl
{
	enum class TOKEN_TYPE
	{
		LEFT_BRACE,    // {
		RIGHT_BRACE,   // }
		LEFT_BRACKET,  // [
		RIGHT_BRACKET, // ]
		COLON,         // :
		COMMA,         // ,
		STRING,
		NUMBER,
		TRUE,
		FALSE,
		NIL,
		END, // end of file
	};

	struct Token
	{
		TOKEN_TYPE type;
		uint32_t line;
		std::string payload;
	};

	class Lexer
	{
	public:
		Lexer(const std::string& _json_stream);
		Result<Token> next();
		Result<Token> peek();

	private:
		std::string json_stream;
		size_t current;
		uint32_t line;
		size_t byte_offset;
	};
};