#pragma once

#include "Array.h"

#include <functional>
#include <utility>
#include <type_traits>
#include <new>

namespace hstl
{
	template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Eq = std::equal_to<Key>>
	class Hash_Map
	{
	private:
		static constexpr size_t GROWTH_SIZE = 1024;
		static constexpr size_t GROWTH_FACTOR = 2u;
		static constexpr float LOAD_FACTOR = 0.7f;

		struct Slot
		{
			Key key;
			Value value;
		};

		enum class BUCKET_STATE : uint8_t
		{
			EMPTY,
			OCCUPIED
		};

		Eq equalizer;
		Hash hasher;
		size_t filled_buckets{0u};
		Array<BUCKET_STATE> states;
		Slot* slots{nullptr};

	private:
		void destroy_slots()
		{
			if constexpr (std::is_trivially_destructible_v<Slot> == false)
			{
				if (filled_buckets > 0u)
				{
					size_t count = states.size();

					for (size_t i = 0; i < count; ++i)
					{
						if (states[i] == BUCKET_STATE::EMPTY)
							continue;

						std::destroy_at(&slots[i]);
					}
				}
			}
			::operator delete(slots);
		}

		void grow_then_rehash()
		{
			size_t new_size = states.size() * GROWTH_FACTOR;

			auto new_states = Array<BUCKET_STATE>{new_size};
			auto new_slots = static_cast<Slot*>(::operator new(new_size * sizeof(Slot)));

			for (size_t i = 0u; i < states.size(); ++i)
			{
				if (states[i] == BUCKET_STATE::OCCUPIED)
				{
					auto new_hash = hasher(slots[i].key) & (new_size - 1u);

					// Probe in new table
					while (new_states[new_hash] == BUCKET_STATE::OCCUPIED)
					{
						new_hash = (new_hash + 1u) & (new_size - 1u);
					}

					new_states[new_hash] = BUCKET_STATE::OCCUPIED;
					new (&new_slots[new_hash]) Slot{std::move(slots[i].key), std::move(slots[i].value)};

					std::destroy_at(&slots[i]);
				}
			}
			::operator delete(slots);

			states = std::move(new_states);
			slots = new_slots;
		}

	public:
		Hash_Map()
		{
			size_t count = GROWTH_SIZE * GROWTH_FACTOR;

			states.resize(count);

			slots = static_cast<Slot*>(::operator new(count * sizeof(Slot)));
		}

		Hash_Map(const Hash_Map& other):
			equalizer{other.equalizer},
			hasher{other.hasher},
			filled_buckets{other.filled_buckets},
			states{other.states}
		{
			size_t count = other.states.size();

			slots = static_cast<Slot*>(::operator new(sizeof(Slot) * count));

			if constexpr (std::is_trivially_copyable_v<Slot>)
			{
				memcpy(slots, other.slots, sizeof(Slot) * count);
			}
			else
			{
				for (size_t i = 0u; i < count; ++i)
				{
					if (other.states[i] == BUCKET_STATE::OCCUPIED)
					{
						new (&slots[i]) Slot(other.slots[i]);
					}
				}
			}
		}

		Hash_Map& operator=(const Hash_Map& other)
		{
			if (this == &other)
			{
				return *this;
			}

			destroy_slots();

			size_t count = other.states.size();

			slots = static_cast<Slot*>(::operator new(sizeof(Slot) * count));

			if constexpr (std::is_trivially_copyable_v<Slot>)
			{
				memcpy(slots, other.slots, sizeof(Slot) * count);
			}
			else
			{
				for (size_t i = 0u; i < count; ++i)
				{
					if (other.states[i] == BUCKET_STATE::EMPTY)
						continue;

					new (&slots[i]) Slot(other.slots[i]);
				}
			}

			equalizer = other.equalizer;
			hasher = other.hasher;
			filled_buckets = other.filled_buckets;
			states = other.states;

			return *this;
		}

		Hash_Map(Hash_Map&& other):
			equalizer{std::move(other.equalizer)},
			hasher{std::move(other.hasher)},
			filled_buckets{other.filled_buckets},
			states{std::move(other.states)},
			slots{other.slots}
		{
			other.slots = nullptr;
			other.filled_buckets = 0u;
		}

		Hash_Map& operator=(Hash_Map&& other)
		{
			if (this == &other)
			{
				return *this;
			}

			destroy_slots();

			equalizer = std::move(other.equalizer);
			hasher = std::move(other.hasher);
			filled_buckets = other.filled_buckets;
			states = std::move(other.states);
			slots = other.slots;

			other.slots = nullptr;
			other.filled_buckets = 0u;

			return *this;
		}

		~Hash_Map()
		{
			destroy_slots();
		}

	public:
		template<typename K, typename V>
		Value& insert(K&& key, V&& value)
		{
			if (filled_buckets >= static_cast<size_t>(LOAD_FACTOR * states.size()))
			{
				grow_then_rehash();
			}

			auto hash = hasher(key) & (states.size() - 1u);

			while (states[hash] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, slots[hash].key))
				{
					slots[hash].value = std::forward<V>(value); // overwrite the existing value
					return slots[hash].value;
				}

				hash = (hash + 1u) & (states.size() - 1u);
			}

			states[hash] = BUCKET_STATE::OCCUPIED;
			new (&slots[hash]) Slot{std::forward<K>(key), std::forward<V>(value)};

			filled_buckets++;

			return slots[hash].value;
		}

		const Value* get(const Key& key) const
		{
			if (filled_buckets == 0)
			{
				return nullptr;
			}

			auto hash = hasher(key) & (states.size() - 1u);

			while (states[hash] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, slots[hash].key))
				{
					return &slots[hash].value;
				}

				hash = (hash + 1u) & (states.size() - 1u);
			}

			return nullptr;
		}

		bool contains(const Key& key) const
		{
			if (filled_buckets == 0)
			{
				return false;
			}

			return get(key) != nullptr;
		}

		bool remove(const Key& key)
		{
			if (filled_buckets == 0)
			{
				return false;
			}

			auto _size = states.size();
			auto mask = _size - 1u;
			auto hole = hasher(key) & mask;
			auto found = false;

			while (states[hole] == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, slots[hole].key) == true)
				{
					found = true;
					break;
				}

				hole = (hole + 1u) & mask;
			}

			if (found == false)
			{
				return false;
			}

			auto current = (hole + 1u) & mask;
			auto dist = [mask, _size](size_t a, size_t b) { return (b + _size - a) & mask; };

			while(states[current] == BUCKET_STATE::OCCUPIED)
			{
				auto home = hasher(slots[current].key) & mask;
				auto dist_home_to_hole = dist(home, hole);
				auto dist_home_to_current = dist(home, current);

				if (dist_home_to_hole <= dist_home_to_current)
				{
					slots[hole] = std::move(slots[current]);
					hole = current;
				}

				current = (current + 1u) & mask;
			}

			states[hole] = BUCKET_STATE::EMPTY;
			std::destroy_at(&slots[hole]);
			filled_buckets--;

			return true;
		}

		size_t count() const { return filled_buckets; }
		size_t capacity() const { return states.size(); }

	public: // Iterator-related
		class Iterator // Forward Iterator
		{
		public:
			Iterator(const BUCKET_STATE* state_ptr, const Slot* slot_ptr, const BUCKET_STATE* state_end):
				state_ptr{state_ptr},
				slot_ptr{slot_ptr},
				state_end{state_end}
			{
				skip_empty();
			}

			Iterator& operator++()
			{
				if (state_ptr != state_end)
				{
					++state_ptr;
					++slot_ptr;
				}

				skip_empty();

				return *this;
			}

			bool operator!=(const Iterator& other) const
			{
				return state_ptr != other.state_ptr;
			}

			bool operator==(const Iterator& other) const
			{
				return state_ptr == other.state_ptr;
			}

			const Slot& operator*() const
			{
				return *slot_ptr;
			}

			const Slot* operator->() const
			{
				return slot_ptr;
			}

		private:
			void skip_empty()
			{
				while (state_ptr != state_end && *state_ptr == BUCKET_STATE::EMPTY)
				{
					state_ptr++;
					slot_ptr++;
				}
			}

			const BUCKET_STATE* state_ptr{nullptr};
			const Slot* slot_ptr{nullptr};
			const BUCKET_STATE* state_end{nullptr};
		};

		Iterator begin() const
		{
			return Iterator{states.begin(), slots, states.end()};
		}

		Iterator end() const
		{
			auto s_end = states.end();

			return Iterator{s_end, slots + states.size(), s_end};
		}
	};
};
