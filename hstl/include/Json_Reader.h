#pragma once

#include <vector>
#include <string>

namespace hstl
{
	enum class TOKEN_TYPE
	{
	
	};

	class Token
	{
	public:

	private:
		TOKEN_TYPE type;
		struct
		{
			uint32_t line;
			uint32_t byte_offset;
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