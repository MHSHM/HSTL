#include "Json_Reader.h"

#include <string>
#include <vector>
#include <cctype>

using namespace hstl;

Lexer::Lexer(const std::string& _json_stream):
	json_stream(_json_stream),
	current(0),
	line(1u),
	byte_offset(0)
{
}

Result<Token> Lexer::next()
{
	Token token {};
	token.type = TOKEN_TYPE::END;
	size_t json_size = json_stream.size();

	if (current >= json_stream.size())
	{
		token.type = TOKEN_TYPE::END;
		token.line = line;
		return token;
	}

	while (current < json_size && std::isspace(json_stream[current]))
	{
		switch (json_stream[current++])
		{
		case '\n':
			++line;
			break;
		default:
			break;
		}
		byte_offset += sizeof(char);
	}

	// TODO:This is a very naive number processing, handle -, fractions etc
	while (current < json_size && std::isdigit(json_stream[current]))
	{
		token.type = TOKEN_TYPE::NUMBER;
		token.line = line;
		token.payload += json_stream[current++];
	}

	if (token.type == TOKEN_TYPE::NUMBER)
	{
		return token;
	}

	// TODO: This is a very naive string processing, handle escape charachter etc
	if (json_stream[current] == '\"')
	{
		token.type = TOKEN_TYPE::STRING;
		auto current_char = json_stream[++current];
		while (current < json_size && current_char != '\"')
		{
			token.line = line;
			token.payload += json_stream[current];
			current_char = json_stream[++current];
		}
		++current; // the closing quote
		return token;
	}

	while (current < json_size && std::isalpha(json_stream[current]))
	{
		token.line = line;
		token.payload += std::tolower(json_stream[current++]);
	}

	if (token.payload == "true")
	{
		token.type = TOKEN_TYPE::TRUE;
	}
	else if (token.payload == "false")
	{
		token.type = TOKEN_TYPE::FALSE;
	}
	else if (token.payload == "null")
	{
		token.type = TOKEN_TYPE::NIL;
	}

	if (token.type != TOKEN_TYPE::END)
	{
		return token;
	}

	if (json_stream[current] == '}')
	{
		token.type = TOKEN_TYPE::RIGHT_BRACE;
		token.line = line;
		++current;
	}

	if (json_stream[current] == '{')
	{
		token.type = TOKEN_TYPE::LEFT_BRACE;
		token.line = line;
		++current;
	}

	if (json_stream[current] == ']')
	{
		token.type = TOKEN_TYPE::RIGHT_BRACKET;
		token.line = line;
		++current;
	}

	if (json_stream[current] == '[')
	{
		token.type = TOKEN_TYPE::LEFT_BRACKET;
		token.line = line;
		++current;
	}

	if (json_stream[current] == ':')
	{
		token.type = TOKEN_TYPE::COLON;
		token.line = line;
		++current;
	}

	if (json_stream[current] == ',')
	{
		token.type = TOKEN_TYPE::COMMA;
		token.line = line;
		++current;
	}

	if (token.type != TOKEN_TYPE::END)
	{
		return token;
	}

	return Err{"Unrecognized token"};
}

Result<Token> Lexer::peek()
{
	return Err{"Empty"};
}
