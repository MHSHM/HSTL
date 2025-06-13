#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <initializer_list>
#include <span>
#include <ranges>

namespace hstl
{
	class Json_Value
	{
	private:
		using Array  = std::vector<Json_Value>;
		using Object = std::unordered_map<std::string, Json_Value>;

		enum VALUE_TYPE
		{
			BOOL,
			NUMBER,
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
			Array array;
			Object object;
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

		Json_Value(const Array& array);
		Json_Value(const std::initializer_list<Json_Value>& il);

		Json_Value(const Object& object);
		Json_Value(const std::initializer_list<std::pair<std::string, Json_Value>>& il);

	public:
		bool get_as_bool() const;
		double get_as_number() const;
		const std::string& get_as_string() const;
		const Array& get_as_array() const;
		const Object& get_as_object() const;

		void set_as_bool(bool _b);
		void set_as_number(double _d);
		void set_as_string(const std::string& _s);
		void set_as_array(const std::initializer_list<Json_Value>& il);
		void set_as_object(const std::initializer_list<std::pair<std::string, Json_Value>>& il);
	};
} // namespace hstl