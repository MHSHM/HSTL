#pragma once

#include <string>
#include <optional>
#include <assert.h>

namespace hstl
{
	using Err = std::string;

	template<typename T>
	class Result
	{
	public:
		Result(const Err& _err):
			err(_err),
			value(std::nullopt) {}

		Result(const T& _value):
			value(_value),
			err(std::nullopt) {}

		operator bool() const { return !err.has_value(); }

		const Err& get_error() const
		{
			assert(err.has_value() && "The result is successful, it has no error attached");
			return *err;
		}

		const T& get_value() const
		{
			assert(!err.has_value() && "The result isn't successful, it has no valid value");
			return *value;
		}

	private:
		std::optional<Err> err;
		std::optional<T> value;
	};
};