#pragma once

template<typename F>
class Defer
{
public:
	Defer(F f):
		callable{std::move(f)},
		active{true}
	{

	}

	Defer(const Defer&) = delete;
	Defer& operator=(const Defer&) = delete;

	Defer(Defer&& source) :
		callable{std::move(source.callable)}
	{
		source.active = false;
	}

	Defer& operator=(Defer&& source)
	{
		if (this == &source)
		{
			return *this;
		}

		callable = std::move(source.callable);
		source.active = false;
		return *this;
	}

	~Defer()
	{
		if (active)
		{
			callable();
		}
	}

private:
	F callable;
	bool active;
};

#define DEFER_CONCAT_IMPL(x, y) x##y
#define DEFER_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)
#define DEFER Defer DEFER_CONCAT(__defer__, __COUNTER__) = [&]()
