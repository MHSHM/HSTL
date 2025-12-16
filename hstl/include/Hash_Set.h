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
			buckets.resize(GROWTH_SIZE * GROWTH_FACTOR);
		}

	public:
		// Will do nothing with duplicates
		T& insert(const T& key)
		{
			if (filled_buckets >= static_cast<size_t>(LOAD_FACTOR * buckets.capacity()))
			{
				grow_then_rehash();
			}

			auto hash = hasher(key) % buckets.capacity();

			while (buckets[hash].state == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, buckets[hash].value))
				{
					return buckets[hash].value;
				}

				hash = ++hash % buckets.capacity(); // linear probing
			}

			buckets[hash].state = BUCKET_STATE::OCCUPIED;
			buckets[hash].value = key;

			filled_buckets++;

			return buckets[hash].value;
		}

		// Will do nothing with duplicates
		T& insert(T&& key)
		{
			if (filled_buckets >= static_cast<size_t>(LOAD_FACTOR * buckets.capacity()))
			{
				grow_then_rehash();
			}

			auto hash = hasher(key) % buckets.capacity();

			while (buckets[hash].state == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, buckets[hash].value) == true)
				{
					return buckets[hash].value;
				}

				hash = (hash + 1u) % buckets.capacity(); // linear probing
			}

			buckets[hash].state = BUCKET_STATE::OCCUPIED;
			buckets[hash].value = std::move(key);

			filled_buckets++;

			return buckets[hash].value;
		}

		const T* get(const T& key) const
		{
			auto hash = hasher(key) % buckets.capacity();

			while (buckets[hash].state == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, buckets[hash].value) == true)
				{
					return &buckets[hash].value;
				}

				hash = (hash + 1u) % buckets.capacity();
			}

			return nullptr;
		}

		bool contains(const T& key) const
		{
			return get(key) != nullptr;
		}

		bool remove(const T& key)
		{
			auto capacity = buckets.capacity();
			auto hole  = hasher(key) % capacity;
			auto found = false;

			while (buckets[hole].state == BUCKET_STATE::OCCUPIED)
			{
				if (equalizer(key, buckets[hole].value) == true)
				{
					found = true;
					break;
				}

				hole = (hole + 1u) % capacity;
			}

			if (found == false)
			{
				return false;
			}

			auto current = (hole + 1u) % capacity;
			auto dist = [capacity](size_t a, size_t b) { return (b + capacity - a) % capacity; };

			while(buckets[current].state == BUCKET_STATE::OCCUPIED)
			{
				auto home = hasher(buckets[current].value /*key*/) % capacity;
				auto dist_home_to_hole = dist(home, hole);
				auto dist_home_to_current = dist(home, current);

				// Shift the ruines to preserve the probing path
				if (dist_home_to_hole <= dist_home_to_current)
				{
					buckets[hole].state = buckets[current].state;
					buckets[hole].value = std::move(buckets[current].value);
					hole = current;
				}

				current = (current + 1u) % capacity;
			}

			buckets[hole].state = BUCKET_STATE::EMPTY;
			filled_buckets--;

			return true;
		}

		size_t count() const { return filled_buckets; }
		size_t capacity() const { return buckets.capacity(); }

	private:
		void grow_then_rehash()
		{
			size_t new_size = buckets.size() * GROWTH_FACTOR;
			auto new_buckets_list = Array<Bucket>{new_size};

			for (size_t i = 0u; i < buckets.size(); ++i)
			{
				if (buckets[i].state == BUCKET_STATE::OCCUPIED)
				{
					auto new_hash = hasher(buckets[i].value /*key*/) % new_buckets_list.capacity();

					while (new_buckets_list[new_hash].state == BUCKET_STATE::OCCUPIED)
					{
						new_hash = ++new_hash % new_buckets_list.capacity();
					}

					new_buckets_list[new_hash].state = BUCKET_STATE::OCCUPIED;
					new_buckets_list[new_hash].value = std::move(buckets[i].value);
				}
			}

			buckets = std::move(new_buckets_list);
		}

	private:
		static constexpr size_t GROWTH_SIZE = 5096;
		static constexpr size_t GROWTH_FACTOR = 2u;
		static constexpr float LOAD_FACTOR = 0.7f;

		enum class BUCKET_STATE
		{
			EMPTY,
			OCCUPIED
		};

		struct Bucket
		{
			BUCKET_STATE state{BUCKET_STATE::EMPTY};
			T value;
		};

		Eq equalizer;
		Hash hasher;
		size_t filled_buckets{0u};
		Array<Bucket> buckets;
	};
};
