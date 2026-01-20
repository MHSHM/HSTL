#pragma once

#include <cstddef>
#include <atomic>

namespace hstl
{
	template<typename T>
	struct Result_Storage
	{
		alignas(T) char _data[sizeof(T)];

		T* data() { return reinterpret_cast<T*>(_data); }
		const T* data() const { return reinterpret_cast<const T*>(_data); }
	};

	template<>
	struct Result_Storage<void> { };

	template<typename T>
	class Task
	{
	private:
		static constexpr size_t LAMBDA_STORAGE_SIZE = 64u;

		[[no_unique_address]] Result_Storage<T> result;
		alignas(std::max_align_t) char lambda_storage[LAMBDA_STORAGE_SIZE];

		enum class LAMBDA_OP
		{
			INVOKE,
			DESTROY,
			MOVE
		};

		using Manage_FN  = void(*)(Task* task, Task* dst, LAMBDA_OP op);
		Manage_FN manage_fn{nullptr};

		// TODO: Guard against false sharing?
		std::atomic<bool> completed{false};

		template<typename Package_Type>
		static void _manage(Task* src, Task* dst, LAMBDA_OP op)
		{
			Package_Type* src_package = reinterpret_cast<Package_Type*>(src->lambda_storage);

			switch (op)
			{
			case LAMBDA_OP::INVOKE:
			{
				if constexpr (std::is_same_v<T, void> == false)
				{
					new (src->result.data()) T{(*src_package)()};
				}
				else
				{
					(*src_package)();
				}

				src->completed.store(true, std::memory_order_release);
				break;
			}
			case LAMBDA_OP::MOVE:
			{
				Package_Type* dst_package = reinterpret_cast<Package_Type*>(dst->lambda_storage);
				new (dst_package) Package_Type{std::move(*src_package)};
				src_package->~Package_Type();
				break;
			}
			case LAMBDA_OP::DESTROY:
			{
				src_package->~Package_Type();
				break;
			}
			default:
				break;
			}
		}

	public:
		template<typename Func, typename... Args>
		Task(Func&& func, Args&&... args)
		{
			auto package = [func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable -> decltype(auto) {
				return std::invoke(std::move(func), std::move(args)...);
			};

			using Package_Type = std::decltype(package);
			static_assert(sizeof(Package_Type) < LAMBDA_STORAGE_SIZE, "The provided task is too big");

			Package_Type* package_loc = reinterpret_cast<Package_Type*>(lambda_storage);
			new (package_loc) Package_Type(std::move(package));

			manage_fn = &_manage<Package_Type>;
		}

		Task(Task&& source)
		{
			source.manage_fn(source, this, LAMBDA_OP::MOVE);

			// NOTE: Moving the result along with the package? If we need this we must be sure
			// that result is actually valid and source task has been completed

			// if constexpr (std::is_same_v<void, T> == false)
			// {
			// 	auto data = result.data();
			//
			// 	new (data) T(std::move(*source.result.data()));
			// }

			manage_fn = source.manage_fn;

			source.manage_fn = nullptr;
		}

		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;
		Task& operator=(Task&&) = delete;

		~Task()
		{
			if constexpr (std::is_same_v<void, T> == false)
			{
				if (completed.load(std::memory_order_acquire))
				{
					result.data()->~T();
				}
			}

			if (manage_fn)
			{
				manage_fn(this, nullptr, LAMBDA_OP::DESTROY);
			}
		}

		void invoke()
		{
			// TODO: Guard against multiple invocations
			manage_fn(this, nullptr, LAMBDA_OP::INVOKE);
		}

		bool is_completed() const
		{
			return completed.load(std::memory_order_acquire);
		}

		decltype(auto) get_result() const
		{
			completed.wait(false, std::memory_order_acquire);

			if constexpr (!std::is_void_v<T>)
			{
				return *result.data();
			}
		}
	};
};
