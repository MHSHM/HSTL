#include "Json_Value.h"
#include <new>

using namespace hstl;

void Json_Value::value_destruct(Json_Value* json_value)
{
	switch (json_value->active_value_type)
	{
	case VALUE_TYPE::STRING: json_value->value.s.~basic_string(); break;
	case VALUE_TYPE::ARRAY:  json_value->value.array.~vector();   break;
	case VALUE_TYPE::OBJECT: json_value->value.object.~unordered_map(); break;
	default: break;                       // BOOL, NUMBER, EMPTY
	}
	json_value->active_value_type = VALUE_TYPE::EMPTY;
}

void Json_Value::value_construct(const Json_Value& src, Json_Value* out)
{
	switch (src.active_value_type)
	{
	case VALUE_TYPE::BOOL:   out->value.b = src.value.b; break;
	case VALUE_TYPE::NUMBER: out->value.d = src.value.d; break;
	case VALUE_TYPE::STRING: new (&out->value.s) std::string(src.value.s); break;
	case VALUE_TYPE::ARRAY:  new (&out->value.array) Array(src.value.array); break;
	case VALUE_TYPE::OBJECT: new (&out->value.object) Object(src.value.object); break;
	case VALUE_TYPE::EMPTY:
	default: break;
	}
	out->active_value_type = src.active_value_type;
}

Json_Value::Json_Value()
	:active_value_type(VALUE_TYPE::EMPTY) {}

Json_Value::~Json_Value()
{
	value_destruct(this);
}

Json_Value::Json_Value(const Json_Value& src)
	:active_value_type(VALUE_TYPE::EMPTY)
{
	value_construct(src, this);
}

Json_Value& Json_Value::operator=(const Json_Value& src)
{
	if (this == &src)
		return *this;

	// destroy current payload (if any)
	if (active_value_type != VALUE_TYPE::EMPTY)
	{
		value_destruct(this);
	}

	value_construct(src, this);
	return *this;
}

Json_Value::Json_Value(const bool& b)
	:active_value_type(VALUE_TYPE::BOOL)
{
	value.b = b;
}

Json_Value::Json_Value(const double& d)
	:active_value_type(VALUE_TYPE::NUMBER)
{
	value.d = d;
}

Json_Value::Json_Value(const std::string& s)
	:active_value_type(VALUE_TYPE::STRING)
{
	new (&value.s) std::string(s);
}

Json_Value::Json_Value(const char* s)
	:active_value_type(VALUE_TYPE::STRING)
{
	new (&value.s) std::string(s);
}

Json_Value::Json_Value(const Array& array)
	:active_value_type(VALUE_TYPE::ARRAY)
{
	new (&value.array) Array(array);
}

Json_Value::Json_Value(const std::initializer_list<Json_Value>& il)
	:active_value_type(VALUE_TYPE::ARRAY)
{
	new (&value.array) Array(il);
}

Json_Value::Json_Value(const Object& object)
	:active_value_type(VALUE_TYPE::OBJECT)
{
	new (&value.object) Object(object);
}

Json_Value::Json_Value(const std::initializer_list<std::pair<std::string, Json_Value>>& il)
	:active_value_type(VALUE_TYPE::OBJECT)
{
	new (&value.object) Object(il.begin(), il.end());
}

bool Json_Value::get_as_bool()  const
{
	return value.b;
}

double Json_Value::get_as_number() const
{
	return value.d;
}

const std::string& Json_Value::get_as_string() const
{
	return value.s;
}

const Json_Value::Array& Json_Value::get_as_array() const
{
	return value.array;
}

const Json_Value::Object& Json_Value::get_as_object() const
{
	return value.object;
}

void Json_Value::set_as_bool(bool _b)
{
	value_destruct(this);
	value.b = _b;
	active_value_type = VALUE_TYPE::BOOL;
}

void Json_Value::set_as_number(double _d)
{
	value_destruct(this);
	value.d = _d;
	active_value_type = VALUE_TYPE::NUMBER;
}

void Json_Value::set_as_string(const std::string& _s)
{
	value_destruct(this);
	new (&value.s) std::string(_s);
	active_value_type = VALUE_TYPE::STRING;
}

void Json_Value::set_as_array(const std::initializer_list<Json_Value>& il)
{
	value_destruct(this);
	new (&value.array) Array(il);
	active_value_type = VALUE_TYPE::ARRAY;
}

void Json_Value::set_as_object(const std::initializer_list<std::pair<std::string, Json_Value>>& il)
{
	value_destruct(this);
	new (&value.object) Object(il.begin(), il.end());
	active_value_type = VALUE_TYPE::OBJECT;
}
