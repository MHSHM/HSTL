#include <catch2/catch_test_macros.hpp>

// FIXME: Add helpers to get the current value of a json value
#define private public                 // make internals visible *just* here
#include "Json_Value.h"
#undef private

using namespace hstl;

TEST_CASE("Default constructor")
{
	Json_Value v;
	REQUIRE(v.active_value_type == Json_Value::VALUE_TYPE::EMPTY);
}

TEST_CASE("Scalar constructors")
{
	SECTION("bool")
	{
		Json_Value jb(true);
		REQUIRE(jb.active_value_type == Json_Value::VALUE_TYPE::BOOL);
		REQUIRE(jb.value.b == true);
	}

	SECTION("double")
	{
		Json_Value jd(3.14159);
		REQUIRE(jd.active_value_type == Json_Value::VALUE_TYPE::DOUBLE);
		REQUIRE(jd.value.d == 3.14159);
	}

	SECTION("std::string")
	{
		Json_Value js(std::string("hello"));
		REQUIRE(js.active_value_type == Json_Value::VALUE_TYPE::STRING);
		REQUIRE(js.value.s == "hello");
	}

	SECTION("C-string")
	{
		Json_Value js("world");
		REQUIRE(js.active_value_type == Json_Value::VALUE_TYPE::STRING);
		REQUIRE(js.value.s == "world");
	}
}

TEST_CASE("Array constructor (std::vector)")
{
	std::vector<Json_Value> src = {1.0, 2.0, 3.0};
	Json_Value jarr(src);

	REQUIRE(jarr.active_value_type == Json_Value::VALUE_TYPE::ARRAY);
	REQUIRE(jarr.value.array.size() == 3);
	REQUIRE(jarr.value.array[1].value.d == 2);
}

TEST_CASE("Array constructor (std::initializer_list)")
{
	Json_Value jarr{1.0, "Test", true, "Hello"};

	REQUIRE(jarr.active_value_type == Json_Value::VALUE_TYPE::ARRAY);
	REQUIRE(jarr.value.array.size() == 4);

	REQUIRE(jarr.value.array[0].active_value_type == Json_Value::VALUE_TYPE::DOUBLE);
	REQUIRE(jarr.value.array[0].value.d == 1.0);

	REQUIRE(jarr.value.array[3].active_value_type == Json_Value::VALUE_TYPE::STRING);
	REQUIRE(jarr.value.array[3].value.s == "Hello");
}

TEST_CASE("Object constructor (std::unordered_map)")
{
	std::unordered_map<std::string, Json_Value> object;

	object["name"] = "Mohamed";
	object["age"] = 25.0;
	object["salary"] = 900.0;
	object["depressed?"] = true;

	Json_Value jobject(object);

	REQUIRE(jobject.active_value_type == Json_Value::VALUE_TYPE::OBJECT);
	REQUIRE(jobject.value.object.size() == 4);

	REQUIRE(jobject.value.object["name"].active_value_type == Json_Value::VALUE_TYPE::STRING);
	REQUIRE(jobject.value.object["name"].value.s == "Mohamed");

	REQUIRE(jobject.value.object["depressed?"].active_value_type == Json_Value::VALUE_TYPE::BOOL);
	REQUIRE(jobject.value.object["depressed?"].value.b == true);
}

TEST_CASE("Object constructor (std::initializer_list)")
{
	Json_Value jobject({
		std::make_pair("name", "Mohamed"),
		std::make_pair("age", 25.0),
		std::make_pair("salary", 900.0),
		std::make_pair("depressed?", true)
	});

	REQUIRE(jobject.active_value_type == Json_Value::VALUE_TYPE::OBJECT);
	REQUIRE(jobject.value.object.size() == 4);

	REQUIRE(jobject.value.object["name"].active_value_type == Json_Value::VALUE_TYPE::STRING);
	REQUIRE(jobject.value.object["name"].value.s == "Mohamed");

	REQUIRE(jobject.value.object["depressed?"].active_value_type == Json_Value::VALUE_TYPE::BOOL);
	REQUIRE(jobject.value.object["depressed?"].value.b == true);
}

TEST_CASE("Copy constructor")
{
	Json_Value arr_0({1.0, "Test", false});
	Json_Value arr_1(arr_0);

	REQUIRE(arr_0.active_value_type == arr_1.active_value_type);

	REQUIRE(arr_0.value.array[0].active_value_type == arr_1.value.array[0].active_value_type);
	REQUIRE(arr_0.value.array[0].value.d == arr_1.value.array[0].value.d);

	REQUIRE(arr_0.value.array[1].active_value_type == arr_1.value.array[1].active_value_type);
	REQUIRE(arr_0.value.array[1].value.s == arr_1.value.array[1].value.s);

	REQUIRE(arr_0.value.array[2].active_value_type == arr_1.value.array[2].active_value_type);
	REQUIRE(arr_0.value.array[2].value.b == arr_1.value.array[2].value.b);
}

TEST_CASE("Copy assignment")
{
	Json_Value arr_0({1.0, "Test", false});
	Json_Value arr_1;

	arr_1 = arr_0;

	REQUIRE(arr_0.active_value_type == arr_1.active_value_type);

	REQUIRE(arr_0.value.array[0].active_value_type == arr_1.value.array[0].active_value_type);
	REQUIRE(arr_0.value.array[0].value.d == arr_1.value.array[0].value.d);

	REQUIRE(arr_0.value.array[1].active_value_type == arr_1.value.array[1].active_value_type);
	REQUIRE(arr_0.value.array[1].value.s == arr_1.value.array[1].value.s);

	REQUIRE(arr_0.value.array[2].active_value_type == arr_1.value.array[2].active_value_type);
	REQUIRE(arr_0.value.array[2].value.b == arr_1.value.array[2].value.b);
}