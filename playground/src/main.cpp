#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>
#include <cstddef>

#include <Json_Value.h>
#include <Result.h>
#include <Json_Reader.h>
#include <Defer.h>
#include <Array.h>

hstl::Result<int> calc(int i)
{
	if (i % 2 == 0)
	{
		return i;
	}

	return hstl::Err{"The provided number is not even\n"};
}

const char* _token_type_to_str(hstl::TOKEN_TYPE type)
{
	switch (type)
	{
	case hstl::TOKEN_TYPE::LEFT_BRACE: return "LEFT_BRACE";
	case hstl::TOKEN_TYPE::RIGHT_BRACE: return "RIGHT_BRACE";
	case hstl::TOKEN_TYPE::LEFT_BRACKET: return "LEFT_BRACKET";
	case hstl::TOKEN_TYPE::RIGHT_BRACKET: return "RIGHT_BRACKET";
	case hstl::TOKEN_TYPE::COLON: return "COLON";
	case hstl::TOKEN_TYPE::COMMA: return "COMMA";
	case hstl::TOKEN_TYPE::STRING: return "STRING";
	case hstl::TOKEN_TYPE::NUMBER: return "NUMBER";
	case hstl::TOKEN_TYPE::TRUE: return "TRUE";
	case hstl::TOKEN_TYPE::FALSE: return "FALSE";
	case hstl::TOKEN_TYPE::NIL: return "NULL";
	case hstl::TOKEN_TYPE::END: return "EOF";
	}

	return "";
}

template<typename T, size_t length>
T& get(T(&arr)[length], size_t index)
{
	if (index >= length)
	{
		throw std::out_of_range{"out of bounds"};
	}

	return arr[index];
}

template<typename T, typename... Args>
auto sum(T first, Args... args)
{
	static_assert((std::is_same<T, Args>::value && ...), "All must be the same type");
	static_assert(sizeof...(Args) > 0u, "Needs at least one argument");
	static_assert(std::is_default_constructible<T>::value, "T needs to be default constructable");
	static_assert(std::is_arithmetic<T>::value, "T needs to be arithmitic");

	T result{};
	((result += args), ...);
	return result;
}

struct Point
{
	int32_t x;
	int32_t y;
	int32_t z;
};

class A
{
public:
	A() = default;

	A(int a):
		a{a}
	{
	
	}

private:
	int a;
};

template<typename... Ts>
struct Tuple;

template<>
struct Tuple<> {};

template<typename Head, typename... Tail>
struct Tuple<Head, Tail...>
{
	Head head;
	Tuple<Tail...> tail;

	template<typename H, typename... T>
	Tuple(H&& h, T&&... t):
		head{std::forward<H>(h)},
		tail{std::forward<T>(t)...}
	{
		
	}
};

class Test
{
public:
	Test(int a, double b, std::string c):
		a{a},
		b{b},
		c{c}
	{}

private:
	int a;
	double b;
	std::string c;
};

template<typename T>
auto value_of(const T& v)
{
	if constexpr (std::is_pointer_v<T>)
	{
		if (v == nullptr)
		{
			throw std::runtime_error{"Null pointer dereference"};
		}

		return *v;
	}
	else
	{
		return v;
	}
}

struct Tracer {
	static inline int alive = 0;
	int id{};
	Tracer(int i):
		id(i)
	{
		++alive;
	}

	Tracer(const Tracer& o):
		id(o.id)
	{
		++alive;
	}

	~Tracer()
	{
		--alive;
	}
};

int main()
{
	std::vector<int> v{10, 10, 10, 10, 10, 10};
	v.resize(2);

	hstl::Array<Tracer> tracers;
	tracers.emplace(0);
	tracers.emplace(1);
	tracers.emplace(2);
	tracers.emplace(3);
	tracers.emplace(4);

	tracers.remove_if([](const Tracer& tracer) {
		if (tracer.id == 1 || tracer.id == 4)
		{
			return true;
		}

		return false;
	});

	for (const auto& tracer : tracers)
	{
		std::cout << tracer.id << '\n';
	}

	int xx = 1;
	auto yy = ++xx;
	auto zz = xx++;

	short x = 1;
	auto y = +x;

	std::byte data[1024]{};
	void* p = data;
	int* pp = (int*)p;
	*pp = 1;

	void* memory = malloc(1024);
	Point* p0 = new(memory) Point{};

	A* dataa = new A[100];
	new (&dataa[0]) A{1};
	new (&dataa[1]) A{2};
	new (&dataa[2]) A{3};
	new (&dataa[3]) A{4};

	A* dataa_2 = new A[100];
	memcpy(dataa_2, dataa, sizeof(A) * 100);

	std::string json = R"({
        "name": "Alice",
        "active": true,
        "deleted": false,
        "age": 30,
        "score": 99.0000,
        "status": "ok"
    })";

	auto lexer = hstl::Lexer(json);

	while (auto token = lexer.next())
	{
		if (token.get_value().type == hstl::TOKEN_TYPE::END)
		{
			break;
		}
	}

	return 0;
}