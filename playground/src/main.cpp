#include <iostream>
#include <string>
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
		Value() {}
		~Value() {}
		Value(const Value&) = delete;
		Value& operator=(const Value&) = delete;

		bool b;
		double d;
		std::string s;
		std::vector<Json_Value> array;
		std::unordered_map<std::string, Json_Value> object;
	};

	Value value;
	VALUE_TYPE active_value_type;

public:
	Json_Value(const Json_Value& src):
		active_value_type(VALUE_TYPE::EMPTY)
	{
		switch (src.active_value_type)
		{
		case Json_Value::BOOL:   value.b = src.value.b; break;
		case Json_Value::DOUBLE: value.d = src.value.d; break;
		case Json_Value::STRING: new (&value.s) std::string(src.value.s); break;
		case Json_Value::ARRAY:  new (&value.array) std::vector<Json_Value>(src.value.array); break;
		case Json_Value::OBJECT: new (&value.object) std::unordered_map<std::string, Json_Value>(src.value.object); break;
		case Json_Value::EMPTY:
		default:
			break;
		}

		active_value_type = src.active_value_type;
	}

	Json_Value& operator=(const Json_Value& src)
	{
		if (this == &src)
		{
			return *this;
		}

		if (active_value_type != VALUE_TYPE::EMPTY)
		{
			switch (active_value_type)
			{
			case Json_Value::BOOL:
			case Json_Value::DOUBLE:
			case Json_Value::EMPTY:
				break;
			case Json_Value::STRING: value.s.~basic_string(); break;
			case Json_Value::ARRAY:  value.array.~vector(); break;
			case Json_Value::OBJECT: value.object.~unordered_map(); break;
			default:
				break;
			}

			active_value_type = VALUE_TYPE::EMPTY;
		}

		switch (src.active_value_type)
		{
		case Json_Value::BOOL:    value.b = src.value.b; break;
		case Json_Value::DOUBLE:  value.d = src.value.d; break;
		case Json_Value::STRING:  new (&value.s) std::string(src.value.s); break;
		case Json_Value::ARRAY:   new (&value.array) std::vector<Json_Value>(src.value.array); break;
		case Json_Value::OBJECT:  new (&value.object) std::unordered_map<std::string, Json_Value>(src.value.object); break;
		case Json_Value::EMPTY:
		default:
			break;
		}

		active_value_type = src.active_value_type;
		return *this;
	}

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

	Json_Value(const char *s):
		active_value_type(VALUE_TYPE::STRING)
	{
		new (&value.s) std::string(s);
	}

	Json_Value(const std::vector<Json_Value>& array):
		active_value_type(VALUE_TYPE::ARRAY)
	{
		new (&value.array) std::vector<Json_Value>(array);
	}

	Json_Value(const std::unordered_map<std::string, Json_Value>& object) :
		active_value_type(VALUE_TYPE::OBJECT)
	{
		new (&value.object) std::unordered_map<std::string, Json_Value>(object);
	}

	~Json_Value()
	{
		switch (active_value_type)
		{
		case Json_Value::STRING: value.s.~basic_string(); break;
		case Json_Value::ARRAY:  value.array.~vector(); break;
		case Json_Value::OBJECT: value.object.~unordered_map(); break;
		case Json_Value::BOOL:
		case Json_Value::DOUBLE:
		case Json_Value::EMPTY:
		default:
			break;
		}
	}
};



int main()
{
	Json_Value a;
	Json_Value b{"test"};
	Json_Value c{true};
	Json_Value d{1.0};
	Json_Value e{
		std::vector<Json_Value>{
			{1.0},
			{2.0}
		}
	};
	Json_Value f{
		std::unordered_map<std::string, Json_Value>{
			std::make_pair("Name", Json_Value{"Mohamed"}),
			std::make_pair("Age", Json_Value{25.0}),
		}
	};

	return 0;
}