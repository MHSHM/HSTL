#pragma once

#include <atomic>

namespace hstl
{
	struct Job
	{
		using Function = void(*)(void* data);

		Function function{nullptr};

		void* data{nullptr};

		std::atomic<int>* parent_counter{nullptr};
	};
};
