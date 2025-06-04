#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>

class Json_Value
{

private:
	enum VALUE_TYPE
	{
		BOOL,
		DOUBLE,
		STRING,
		ARRAY,
		OBJECT,
		EMPTY
	};

	union Value
	{
		Value() { }
		~Value() {}

		bool b;
		double d;
		std::string s;
		std::vector<Json_Value> array;
		std::unordered_map<std::string, Json_Value> object;
	};

	Value value;
	VALUE_TYPE active_value_type;

public:
	Json_Value():
		active_value_type(VALUE_TYPE::EMPTY)
	{
	
	}

	Json_Value(const bool& b):
		active_value_type(VALUE_TYPE::BOOL)
	{
		value.b = b;
	}

	Json_Value(const double& d):
		active_value_type(VALUE_TYPE::DOUBLE)
	{
		value.d = d;
	}

	Json_Value(const std::string& s):
		active_value_type(VALUE_TYPE::STRING)
	{
		new (&value.s) std::string(s);
	}

	~Json_Value()
	{
	}
};

int main()
{

	return 0;
}