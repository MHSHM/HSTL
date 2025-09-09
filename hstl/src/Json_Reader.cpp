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

	if (current >= json_stream.size())
	{
		token.type = TOKEN_TYPE::END;
		token.loc.byte_offset = byte_offset++;
		token.loc.line = line;
		return token;
	}

	while (current < json_stream.size() && std::isspace(json_stream[current]))
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

	while (current < json_stream.size() && std::isdigit(json_stream[current]))
	{
		token.type = TOKEN_TYPE::NUMBER;
		token.loc.byte_offset = byte_offset++;
		token.loc.line = line;
		token.payload += json_stream[current++];
	}

	if (token.type == TOKEN_TYPE::NUMBER)
	{
		return token;
	}

	while (current < json_stream.size() && std::isalpha(json_stream[current]))
	{
		token.type = TOKEN_TYPE::STRING;
		token.loc.byte_offset = byte_offset++;
		token.loc.line = line;
		token.payload += json_stream[current++];
	}

	if (token.type == TOKEN_TYPE::STRING)
	{
		return token;
	}

	// TODO: Handle the rest of the tokens
	return Err{"Inrecognized token"};
}

Result<Token> Lexer::peek()
{
	return Err{"Empty"};
}
