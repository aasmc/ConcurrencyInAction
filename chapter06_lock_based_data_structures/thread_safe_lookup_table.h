#pragma once

#include "vector"
#include "utility"
#include "functional"
#include "list"
#include "mutex"
#include "shared_mutex"
#include "algorithm"
#include "numeric"
#include "map"

/**
 * Simple thread safe lookup table that supports the following operations:
 *  - Add a new key/value pair.
 *  - Change the value associated with a given key.
 *  - Remove a key and its associated value.
 *  - Obtain the value associated with a given key, if any.
 *
 *  The number of buckets in the table is set once at construction time. The default is 19 -
 *  an arbitrary prime number.
 *
 * @tparam Key
 * @tparam Value
 * @tparam Hash
 */
template<typename Key, typename Value, typename Hash=std::hash<Key>>
class thread_safe_lookup_table {
private:
    class bucket_type {
    public:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using bucket_iterator = bucket_data::iterator;
        bucket_data data;
        // allows many concurrent readers and single writer
    public:
        mutable std::shared_mutex mutex;
    private:
        bucket_iterator find_entry_for(const Key &key) const {
            return std::find_if(data.begin(), data.end(),
                                [&](const bucket_value &item) {
                                    return item.first == key;
                                });
        }

    public:

        Value value_for(const Key &key, const Value &default_value) const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            return (found_entry == data.end()) ?
                   default_value : found_entry->second;
        }

        void add_or_update_mapping(const Key &key, const Value &value) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry == data.end()) {
                data.push_back(bucket_value{key, value});
            } else {
                found_entry->second = value;
            }
        }

        void remove_mapping(const Key &key) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry != data.end()) {
                data.erase(found_entry);
            }
        }
    };

    std::vector<std::unique_ptr<bucket_type>> buckets;
    Hash hasher;

    bucket_type &get_bucket(const Key &key) const {
        const std::size_t bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }

public:
    using key_type = Key;
    using mapped_value = Value;
    using hash_type = Hash;

    thread_safe_lookup_table(
            unsigned num_buckets = 19, const Hash &hasher_ = Hash()
    ) : buckets(num_buckets), hasher(hasher_) {
        for (unsigned i = 0; i < num_buckets; ++i) {
            buckets[i].reset(new bucket_type)
        }
    }

    thread_safe_lookup_table(const thread_safe_lookup_table &) = delete;

    thread_safe_lookup_table &operator=(thread_safe_lookup_table &) = delete;

    Value value_for(const Key &key, const Value &default_value = Value()) const {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(const Key &key, const Value &value) {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(const Key &key) {
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map() const {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned i = 0; i < buckets.size(); ++i) {

            locks.push_back(
                    std::unique_lock<std::shared_mutex>(buckets[i]->mutex)
            );
        }

        std::map<Key, Value> res;
        for (unsigned i = 0; i < buckets.size(); ++i) {
            for (bucket_type::bucket_iterator it = buckets[i]->data.begin();
                 it != buckets[i]->data.end(); ++it) {
                res.insert(*it);
            }
        }
        return res;
    }
};








































