#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <initializer_list>

namespace hstl
{
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

		Value       value;
		VALUE_TYPE  active_value_type;

	public:
		Json_Value();
		Json_Value(const Json_Value& src);
		Json_Value& operator=(const Json_Value& src);
		~Json_Value();

		Json_Value(const bool& b);
		Json_Value(const double& d);
		Json_Value(const std::string& s);
		Json_Value(const char* s);

		Json_Value(const std::vector<Json_Value>& array);
		Json_Value(const std::initializer_list<Json_Value>& il);

		Json_Value(const std::unordered_map<std::string, Json_Value>& object);
		Json_Value(const std::initializer_list<std::pair<std::string, Json_Value>>& il);
	};
} // namespace hstl