#include "Json_Reader.h"

#include <string>
#include <vector>

namespace hstl
{
	enum class TOKEN_TYPE
	{
		L_BRACE,
		R_BRACE,
		L_BRACKET,
		R_BRACKET,
		STRING,
		COLON,
		COMMA,
		NUMBER,
		FALSE,
		TRUE,
		NIL,
		END_OF_INPUT
	};

	struct Token
	{
		TOKEN_TYPE type;

		union
		{
			const char* text;
			double number;
			bool boolean;
		} data;
	};

	class Tokenizer
	{
	public:
		Tokenizer(const std::string& raw_json);

	private:
		std::vector<Token> tokens;
	};

	Tokenizer::Tokenizer(const std::string& raw_json)
	{
		// Generate tokens
	}
};