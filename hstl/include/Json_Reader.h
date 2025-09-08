#pragma once

#include <vector>
#include <string>

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

	class Token
	{
	public:

	private:
		TOKEN_TYPE type;
		struct
		{
			uint32_t line;
			size_t byte_offset;
			uint32_t column;
		} loc;
		std::string payload;
	};

	class Lexer
	{
	public:

	private:
		std::vector<Token> tokens;
	};
};