#pragma once

#include "Array.h"

#include <functional>
#include <cstddef>
#include <type_traits>

namespace hstl
{
	template<typename T, typename Hash = std::hash<T>, typename Eq = std::equal_to<T>>
	class Hash_Set
	{
	public:
		Hash_Set()
		{
			states.resize(GROWTH_SIZE * GROWTH_FACTOR);

			// FIXME: This will invoke the default constrctor for all entries in the array
			// if for example we started with 1024 entry it will invoke 1024 constructor call and it gets
			// bad if the defalt constructor wasn't trivial and had some complex stuff going e.g. memory allocation
			// better use raw memory and do in-place constrction upon insertion
			values.resize(GROWTH_SIZE * GROWTH_FACTOR);
		}

		Hash_Set(const Hash_Set& other):
			equalizer{other.equalizer},
			hasher{other.hasher},
			filled_buckets{other.filled_buckets},
			states{other.states}
		{
			size_t count = GROWTH_SIZE * GROWTH_FACTOR;

			values.resize(count);

			if constexpr (std::is_scalar_v<T> == true)
			{
				memcpy(values.buffer(), other.values.buffer(), sizeof(T) * count);
			}
			else
			{
				for (size_t i = 0u; i < count; ++i)
				{
					if (other.states[i] == BUCKET_STATE::EMPTY)
						continue;

					values[i] = other.values[i];
				}
			}
		}

		Hash_Set& operator=(const Hash_Set& other)
		{
			if (this == &other)
			{
				return *this;
			}

			equalizer = other.equalizer;
			hasher = other.hasher;
			filled_buckets = other.filled_buckets;
			states = other.states;

			size_t count = other.values.size();

			values.resize(count);

			if constexpr (std::is_scalar_v<T> == true)
			{
				memcpy(values.buffer(), other.values.buffer(), sizeof(T) * count);
			}
			else
			{
				// This will do many redundunt copies but we have to do it this way to make sure we
				// override all elements in values, see the fixme note in the constructor
				values = other.values;
			}

			return *this;
		}

		Hash_Set(Hash_Set&& other):
			equalizer{std::move(other.equalizer)},
			hasher{std::move(other.hasher)},
			filled_buckets{other.filled_buckets},
			states{std::move(other.states)},
			values{std::move(other.values)}
		{
			other.filled_buckets = 0u;
		}

		Hash_Set& operator=(Hash_Set&& other)
		{
			if (this == &other)
			{
				return *this;
			}

			equalizer = std::move(other.equalizer);
			hasher = std::move(other.hasher);
			filled_buckets = other.filled_buckets;
			states = std::move(other.states);
			values = std::move(other.values);

			other.filled_buckets = 0u;

			return *this;
		}

	public:
		template<typename K>
		T& insert(K&& key)
		{
			if (filled_buckets >= static_cast<size_t>(LOAD_FACTOR * states.size()))
			{
				grow_then_rehash();
			}

			// NOTE: I didn't want to force `std::is_same<T, K>` to allow implicit conversions
			// e.g. Hash_Set<Str> set should accept set.insert("SSSS")
			auto hash = hasher(key) & (states.size() - 1u);

			while (states[hash] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, values[hash]))
				{
					return values[hash];
				}

				hash = (hash + 1u) & (states.size() - 1u); // linear probing
			}

			states[hash] = BUCKET_STATE::OCCUPIED;
			values[hash] = std::forward<K>(key);

			filled_buckets++;

			return values[hash];
		}

		const T* get(const T& key) const
		{
			auto hash = hasher(key) & (states.size() - 1u);

			while (states[hash] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, values[hash]) == true)
				{
					return &values[hash];
				}

				hash = (hash + 1u) & (states.size() - 1u);
			}

			return nullptr;
		}

		bool contains(const T& key) const
		{
			return get(key) != nullptr;
		}

		bool remove(const T& key)
		{
			auto _size = states.size();
			auto hole  = hasher(key) & (_size - 1u);
			auto found = false;

			while (states[hole] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, values[hole]) == true)
				{
					found = true;
					break;
				}

				hole = (hole + 1u) & (_size - 1u);
			}

			if (found == false)
			{
				return false;
			}

			auto current = (hole + 1u) & (_size - 1u);
			auto dist = [_size](size_t a, size_t b) { return (b + _size - a) % _size; };

			while(states[current] == BUCKET_STATE::OCCUPIED)
			{
				auto home = hasher(values[current] /*key*/) & (_size - 1u);
				auto dist_home_to_hole = dist(home, hole);
				auto dist_home_to_current = dist(home, current);

				// Shift the ruines to preserve the probing path
				if (dist_home_to_hole <= dist_home_to_current)
				{
					states[hole] = states[current];
					values[hole] = std::move(values[current]);
					hole = current;
				}

				current = (current + 1u) & (_size - 1u);
			}

			states[hole] = BUCKET_STATE::EMPTY;
			filled_buckets--;

			return true;
		}

		size_t count() const { return filled_buckets; }
		size_t capacity() const { return states.size(); }

	private:
		void grow_then_rehash()
		{
			size_t new_size = states.size() * GROWTH_FACTOR;

			auto new_states_list = Array<BUCKET_STATE>{new_size};
			auto new_values_list = Array<T>{new_size};

			for (size_t i = 0u; i < states.size(); ++i)
			{
				if (states[i] == BUCKET_STATE::OCCUPIED)
				{
					auto new_hash = hasher(values[i] /*key*/) & (new_states_list.size() - 1u);

					while (new_states_list[new_hash] == BUCKET_STATE::OCCUPIED)
					{
						new_hash = (new_hash + 1u) & (new_states_list.size() - 1u);
					}

					new_states_list[new_hash] = BUCKET_STATE::OCCUPIED;
					new_values_list[new_hash] = std::move(values[i]);
				}
			}

			states = std::move(new_states_list);
			values = std::move(new_values_list);
		}

	private:
		static constexpr size_t GROWTH_SIZE = 1024;
		static constexpr size_t GROWTH_FACTOR = 2u;
		static constexpr float LOAD_FACTOR = 0.7f;

		enum class BUCKET_STATE : uint8_t
		{
			EMPTY,
			OCCUPIED
		};

		Eq equalizer;
		Hash hasher;
		size_t filled_buckets{0u};
		Array<BUCKET_STATE> states;
		Array<T> values;

	public: /*Iterator-related*/
		class Iterator // Forward Iterator
		{
		public:
			Iterator(const BUCKET_STATE* state_ptr, const T* value_ptr, const BUCKET_STATE* state_end):
				state_ptr{state_ptr},
				value_ptr{value_ptr},
				state_end{state_end}
			{
				skip_empty();
			}

			Iterator& operator++()
			{
				if (state_ptr != state_end)
				{
					++state_ptr;
					++value_ptr;
				}

				skip_empty();

				return *this;
			}

			bool operator!=(const Iterator& iterator) const
			{
				return state_ptr != iterator.state_ptr;
			}

			bool operator==(const Iterator& iterator) const
			{
				return state_ptr == iterator.state_ptr;
			}

			const T& operator*() const
			{
				return *value_ptr;
			}

		private:
			void skip_empty()
			{
				while (state_ptr != state_end && *state_ptr == BUCKET_STATE::EMPTY)
				{
					state_ptr++;
					value_ptr++;
				}
			}

		private:
			const BUCKET_STATE* state_ptr{nullptr};
			const T* value_ptr{nullptr};
			const BUCKET_STATE* state_end{nullptr};
		};

		Iterator begin() const
		{
			return Iterator{states.buffer(), values.buffer(), states.buffer() + states.size()};
		}

		Iterator end() const
		{
			auto s_end = states.buffer() + states.size();
			auto v_end = values.buffer() + values.size();

			return Iterator{s_end, v_end, s_end};
		}
	};
};
