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
		static constexpr uint8_t BIT_OCCUPIED = 0b10000000;

		struct Slot
		{
			Key key;
			Value value;
		};

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
			uint8_t fingerprint = static_cast<uint8_t>(hash >> 57);
			return fingerprint | BIT_OCCUPIED;
		}

		Eq equalizer;
		Hash hasher;
		size_t filled_buckets{0u};
		Array<uint8_t> states;
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
						if (is_empty(states[i]))
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
			auto new_states_list = Array<uint8_t>{new_size};
			auto new_slots_list = static_cast<Slot*>(::operator new(new_size * sizeof(Slot)));

			for (size_t i = 0u; i < states.size(); ++i)
			{
				if (!is_empty(states[i]))
				{
					auto new_hash = hasher(slots[i].key);
					auto new_index = new_hash & (new_size - 1u);

					while (!is_empty(new_states_list[new_index]))
					{
						new_index = (new_index + 1u) & (new_size - 1u);
					}

					new_states_list[new_index] = states[i];
					new (&new_slots_list[new_index]) Slot{std::move(slots[i].key), std::move(slots[i].value)};

					std::destroy_at(&slots[i]);
				}
			}
			::operator delete(slots);

			states = std::move(new_states_list);
			slots = new_slots_list;
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
					if (!is_empty(other.states[i]))
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
					if (!is_empty(other.states[i]))
					{
						new (&slots[i]) Slot(other.slots[i]);
					}
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

			auto hash = hasher(key);
			uint8_t control_byte = make_control_byte(hash);
			size_t index = hash & (states.size() - 1u);

			while (!is_empty(states[index]))
			{
				if (states[index] == control_byte)
				{
					if (equalizer(key, slots[index].key))
					{
						slots[index].value = std::forward<V>(value); // overwrite existing
						return slots[index].value;
					}
				}

				index = (index + 1u) & (states.size() - 1u);
			}

			states[index] = control_byte;
			new (&slots[index]) Slot{std::forward<K>(key), std::forward<V>(value)};

			filled_buckets++;

			return slots[index].value;
		}

		const Value* get(const Key& key) const
		{
			if (filled_buckets == 0)
			{
				return nullptr;
			}

			auto hash = hasher(key);
			uint8_t control_byte = make_control_byte(hash);
			size_t index = hash & (states.size() - 1u);

			while (!is_empty(states[index]))
			{
				if (states[index] == control_byte)
				{
					if (equalizer(key, slots[index].key))
					{
						return &slots[index].value;
					}
				}

				index = (index + 1u) & (states.size() - 1u);
			}

			return nullptr;
		}

		bool contains(const Key& key) const
		{
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

			auto hash = hasher(key);
			auto hole_control_byte = make_control_byte(hash);
			auto hole_index = hash & mask;
			auto found = false;

			while (!is_empty(states[hole_index]))
			{
				if (states[hole_index] == hole_control_byte)
				{
					if (equalizer(key, slots[hole_index].key))
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
				auto home_hash = hasher(slots[current_index].key);
				auto home_index = home_hash & mask;

				auto dist_home_to_hole = dist(home_index, hole_index);
				auto dist_home_to_current = dist(home_index, current_index);

				if (dist_home_to_hole <= dist_home_to_current)
				{
					slots[hole_index] = std::move(slots[current_index]);
					states[hole_index] = states[current_index];

					hole_index = current_index;
				}

				current_index = (current_index + 1u) & mask;
			}

			states[hole_index] = 0;
			std::destroy_at(&slots[hole_index]);
			filled_buckets--;

			return true;
		}

		size_t count() const { return filled_buckets; }
		size_t capacity() const { return states.size(); }

	public: // Iterator-related
		class Iterator // Input Iterator
		{
		public:
			Iterator(const uint8_t* state_ptr, const Slot* slot_ptr, const uint8_t* state_end):
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
				while (state_ptr != state_end && is_empty(*state_ptr))
				{
					state_ptr++;
					slot_ptr++;
				}
			}

			const uint8_t* state_ptr{nullptr};
			const Slot* slot_ptr{nullptr};
			const uint8_t* state_end{nullptr};
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
