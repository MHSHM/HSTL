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
	private:
		static constexpr size_t GROWTH_SIZE = 1024;
		static constexpr size_t GROWTH_FACTOR = 2u;
		static constexpr float LOAD_FACTOR = 0.7f;
		// The MSB (Most Significant Bit) marks a slot as OCCUPIED.
		// 0b1xxxxxxx = Occupied
		// 0b0xxxxxxx = Empty
		static constexpr uint8_t BIT_OCCUPIED = 0b10000000;

		Eq equalizer;
		Hash hasher;
		size_t filled_buckets{0u};
		Array<uint8_t> states;
		T* values{nullptr};

	private:
		static bool is_empty(uint8_t control_byte)
		{
			return (control_byte & BIT_OCCUPIED) == 0;
		}

		static bool is_match(uint8_t control_byte, uint8_t fingerprint)
		{
			return control_byte == (fingerprint | BIT_OCCUPIED);
		}

		static uint8_t make_control_byte(size_t hash)
		{
			static_assert(sizeof(size_t) == 8, "The logic is built upon the assumption that size_t is 8 bytes");

			uint8_t control_byte = static_cast<uint8_t>(hash >> 57);

			return control_byte | BIT_OCCUPIED;
		}

		void grow_then_rehash()
		{
			size_t new_size = states.size() * GROWTH_FACTOR;
			auto new_states_list = Array<uint8_t>{new_size};
			auto new_values_list = static_cast<T*>(::operator new(new_size * sizeof(T)));

			for (size_t i = 0u; i < states.size(); ++i)
			{
				if (!is_empty(states[i]))
				{
					auto new_hash = hasher(values[i] /*key*/);
					auto new_index = new_hash & (new_size - 1u);

					while (!is_empty(new_states_list[new_index]))
					{
						new_index = (new_index + 1u) & (new_size - 1u);
					}

					new_states_list[new_index] = states[i];
					new (&new_values_list[new_index]) T(std::move(values[i]));

					std::destroy_at(&values[i]);
				}
			}
			::operator delete(values);

			states = std::move(new_states_list);
			values = new_values_list;
		}

		void destroy_values()
		{
			if constexpr (std::is_trivially_destructible_v<T> == false)
			{
				if (filled_buckets > 0u)
				{
					size_t count = states.size();

					for (size_t i = 0; i < count; ++i)
					{
						if (is_empty(states[i]))
							continue;

						std::destroy_at(&values[i]);
					}
				}
			}
			::operator delete(values);
		}

	public:
		Hash_Set()
		{
			size_t count = GROWTH_SIZE * GROWTH_FACTOR;

			states.resize(count);

			values = static_cast<T*>(::operator new(count * sizeof(T)));
		}

		Hash_Set(const Hash_Set& other):
			equalizer{other.equalizer},
			hasher{other.hasher},
			filled_buckets{other.filled_buckets},
			states{other.states}
		{
			size_t count = other.states.size();

			values = static_cast<T*>(::operator new(count * sizeof(T)));

			if constexpr (std::is_trivially_copyable_v<T> == true)
			{
				memcpy(values, other.values, sizeof(T) * count);
			}
			else
			{
				for (size_t i = 0u; i < count; ++i)
				{
					if (is_empty(other.states[i]))
						continue;

					new (&values[i]) T(other.values[i]);
				}
			}
		}

		Hash_Set& operator=(const Hash_Set& other)
		{
			if (this == &other)
			{
				return *this;
			}

			destroy_values();

			size_t count = other.states.size();

			values = static_cast<T*>(::operator new(count * sizeof(T)));

			if constexpr (std::is_trivially_copyable_v<T> == true)
			{
				memcpy(values, other.values, sizeof(T) * count);
			}
			else
			{
				for (size_t i = 0u; i < count; ++i)
				{
					if (is_empty(other.states[i]))
						continue;

					new (&values[i]) T(other.values[i]);
				}
			}

			equalizer = other.equalizer;
			hasher = other.hasher;
			filled_buckets = other.filled_buckets;
			states = other.states;

			return *this;
		}

		Hash_Set(Hash_Set&& other):
			equalizer{std::move(other.equalizer)},
			hasher{std::move(other.hasher)},
			filled_buckets{other.filled_buckets},
			states{std::move(other.states)},
			values{other.values}
		{
			other.values = nullptr;
			other.filled_buckets = 0u;
		}

		Hash_Set& operator=(Hash_Set&& other)
		{
			if (this == &other)
			{
				return *this;
			}

			destroy_values();

			equalizer = std::move(other.equalizer);
			hasher = std::move(other.hasher);
			filled_buckets = other.filled_buckets;
			states = std::move(other.states);
			values = other.values;

			other.values = nullptr;
			other.filled_buckets = 0u;

			return *this;
		}

		~Hash_Set()
		{
			destroy_values();
		}

	public:
		template<typename K>
		T& insert(K&& key)
		{
			auto size = states.size();

			if (filled_buckets >= static_cast<size_t>(LOAD_FACTOR * size))
			{
				grow_then_rehash();
			}

			// NOTE: I didn't want to force `std::is_same<T, K>` to allow implicit conversions
			// e.g. Hash_Set<Str> set should accept set.insert("SSSS")
			auto mask = size - 1u;
			auto hash = hasher(key);
			uint8_t control_byte = make_control_byte(hash);
			size_t index = hash & mask;

			while (!is_empty(states[index]))
			{
				if (control_byte == states[index])
				{
					if (equalizer(key, values[index]))
					{
						return values[index];
					}
				}

				index = (index + 1u) & mask; // linear probing
			}

			states[index] = control_byte;
			new (&values[index]) T(std::forward<K>(key));

			filled_buckets++;

			return values[index];
		}

		const T* get(const T& key) const
		{
			if (filled_buckets == 0)
			{
				return nullptr;
			}

			auto size = states.size();
			auto mask = size - 1u;
			auto hash = hasher(key);
			uint8_t control_byte = make_control_byte(hash);
			size_t index = hash & mask;

			while (!is_empty(states[index]))
			{
				if (control_byte == states[index])
				{
					if (equalizer(key, values[index]) == true)
					{
						return &values[index];
					}
				}

				index = (index + 1u) & mask;
			}

			return nullptr;
		}

		bool contains(const T& key) const
		{
			if (filled_buckets == 0)
			{
				return false;
			}

			return get(key) != nullptr;
		}

		bool remove(const T& key)
		{
			if (filled_buckets == 0)
			{
				return false;
			}

			auto _size = states.size();
			auto mask = (_size - 1u);
			auto hash = hasher(key);
			auto hole_control_byte = make_control_byte(hash);
			auto hole_index = hash & mask;
			auto found = false;

			while (!is_empty(states[hole_index]))
			{
				if (states[hole_index] == hole_control_byte)
				{
					if (equalizer(key, values[hole_index]) == true)
					{
						found = true;
						break;
					}
				}

				hole_index = (hole_index + 1u) & mask;
			}

			if (found == false)
			{
				return false;
			}

			auto current_index = (hole_index + 1u) & mask;
			auto dist = [mask, _size](size_t a, size_t b) { return (b + _size - a) & mask; };

			while(!is_empty(states[current_index]))
			{
				auto home_hash = hasher(values[current_index] /*key*/);
				auto home_index = home_hash & mask;
				auto dist_home_to_hole = dist(home_index, hole_index);
				auto dist_home_to_current = dist(home_index, current_index);

				// Shift the ruines to preserve the probing path
				if (dist_home_to_hole <= dist_home_to_current)
				{
					values[hole_index] = std::move(values[current_index]);
					states[hole_index] = states[current_index];
					hole_index = current_index;
				}

				current_index = (current_index + 1u) & mask;
			}

			states[hole_index] = 0;
			std::destroy_at(&values[hole_index]);
			filled_buckets--;

			return true;
		}

		size_t count() const { return filled_buckets; }
		size_t capacity() const { return states.size(); }

	public: // Iterator-related
		class Iterator // Input Iterator
		{
		public:
			Iterator(const uint8_t* state_ptr, const T* value_ptr, const uint8_t* state_end):
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

			const T* operator->() const
			{
				return value_ptr;
			}

		private:
			void skip_empty()
			{
				while (state_ptr != state_end && is_empty(*state_ptr))
				{
					state_ptr++;
					value_ptr++;
				}
			}

		private:
			const uint8_t* state_ptr{nullptr};
			const T* value_ptr{nullptr};
			const uint8_t* state_end{nullptr};
		};

		Iterator begin() const
		{
			return Iterator{states.buffer(), values, states.end()};
		}

		Iterator end() const
		{
			auto s_end = states.end();

			return Iterator{s_end, values + states.size(), s_end};
		}
	};
};
