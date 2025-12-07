#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <assert.h>
#include <cstddef>
#include <memory>

#include <Json_Value.h>
#include <Result.h>
#include <Json_Reader.h>
#include <Defer.h>
#include <Array.h>
#include <Str.h>

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
	Tracer() = default;
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

struct EvenNumbersRangeIterator
{
	EvenNumbersRangeIterator(int value):
		value{value} {}

	bool operator!=(const EvenNumbersRangeIterator& other)
	{
		return other.value != value;
	}

	EvenNumbersRangeIterator& operator++()
	{
		value = value + 2;
		return *this;
	}

	int operator*()
	{
		return value;
	}

	int value;
};

struct EvenNumbersRange
{
	EvenNumbersRange(int range_max):
		range_max{range_max}
	{

	}

	EvenNumbersRangeIterator begin()
	{
		return EvenNumbersRangeIterator{0};
	}

	EvenNumbersRangeIterator end()
	{
		return EvenNumbersRangeIterator{range_max % 2 != 0 ? range_max + 1 : range_max};
	}

	int range_max;
};

template<typename T, typename... Args>
constexpr T sum_of(T t, Args... args)
{
	return t + sum_of(args...);
}

template<typename T>
constexpr T sum_of(T t)
{
	return t;
}

template<typename Fn, typename In, typename Out>
constexpr Out fold(Fn pred, In* in, size_t length, Out initial)
{
	for (size_t i = 0; i < length - 1; ++i)
	{
		initial += pred(in[i], in[i + 1]);
	}

	return initial;
}

template<typename T, typename... Args>
std::unique_ptr<T> make_unique_ptr(Args&&... args)
{
	auto ptr = std::unique_ptr<T>(new T{std::forward<Args>(args)...});

	return ptr;
}

int main()
{
	hstl::Array<int> numbers;

	int data[]{ 100, 200, 300, 400, 500 };
	size_t data_len = 5;

	auto size_t = fold([](auto x, auto y) -> decltype(x + y) {
		return x + y;
	}, data, 5, 0);

	for (int i = 0; i < 10; ++i)
	{
		numbers.push(i);
	}

	numbers.remove_ordered(0);
	numbers.remove_ordered(3);
	numbers.remove_ordered(4);

	hstl::Str str{"Hello World!"};
	str.resize(20, 'H');

	hstl::Str empty;
	auto substr  = empty.push("Hello");
	auto substr0 = empty.push(" World!");

	auto view = empty.view();

	std::unique_ptr<int> int_ptr{new int};

	auto deleter = [](hstl::Str* str)
	{
		std::cout << "Deleting hstl::Str\n";
		delete str;
	};

	auto my_deleter = [](int* x) {
		printf("Deleting an int at %p.", x);
		delete x;
	};
	std::unique_ptr<int, decltype(my_deleter)> my_up{ new int, my_deleter};

	hstl::Str_View h = "hello world";
	hstl::Str_View hh = "hello world";

	auto found = h.find("llo");

	bool res = (h == hh);

	return 0;
}
