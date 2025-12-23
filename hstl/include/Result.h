#pragma once

#include "Str.h"

#include <memory>
#include <assert.h>

namespace hstl
{
	class Err
	{
	private:
		Str_View message;

	public:
		Err(const char* message):
			message{message}
		{

		}

		Str_View get_message() const
		{
			return message;
		}
	};

	template<typename T>
	class Result
	{
	private:
		union
		{
			Err err;
			T res;
		};

		bool has_err{false};

	public:
		Result(const Err& _err):
			has_err{true}
		{
			new (&err) Err(_err);
		}

		Result(const T& t):
			has_err{false}
		{
			new (&res) T(t);
		}

		Result(T&& t):
			has_err{false}
		{
			new (&res) T(std::move(t));
		}

		Result(const Result& other):
			has_err{other.has_err}
		{
			if (other.has_err)
			{
				new (&err) Err(other.err);
			}
			else
			{
				new (&res) T(other.res);
			}
		}

		Result(Result&& other):
			has_err{other.has_err}
		{
			if (other.has_err)
			{
				new (&err) Err(std::move(other.err));
			}
			else
			{
				new (&res) T(std::move(other.res));
			}
		}

		operator bool() const
		{
			return !has_err;
		}

		Result& operator=(const Result&) = delete;
		Result& operator=(Result&&) = delete;

		T& get_value()
		{
			assert(has_err == false);

			return res;
		}

		Str_View get_err() const
		{
			assert(has_err == true);

			return err.get_message();
		}

		~Result()
		{
			if (has_err == false)
			{
				std::destroy_at(&res);
			}
		}
	};
};
