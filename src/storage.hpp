#pragma once

#include <cstddef>
#include <stdexcept>
#include <shared_mutex>
#include <vector>
#include <mutex>
#include <optional>
#include <utility>

template<typename KeyT, typename ValueT>
class Storage {
public:
    Storage(std::size_t capacity, std::size_t cache_size)
        : capacity_(capacity), cache_size_(cache_size), data_(capacity), cache_(cache_size), cache_used_(cache_size, false) {
            if (capacity_ == 0) {
                throw std::invalid_argument("Capacity = 0");
            }
            if (cache_size_ == 0) {
                throw std::invalid_argument("Cache = 0");
            }
            if (cache_size_ > capacity_) {
                throw std::invalid_argument("Cache size exceeds capacity");
            }
        }

    void store(const KeyT& key, const ValueT& value) {
        std::unique_lock lock(mutex_);
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (data_[i].has_value() && data_[i]->key == key) {
                data_[i]->value = value;
                store_to_cache(i);
                return;
            }
        }

        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (!data_[i].has_value()) {
                data_[i] = Entry{key, value};
                store_to_cache(i);
                return;
            }
        }

        throw std::runtime_error("Storage is full");
    }

    ValueT load(const KeyT& key) {
        ValueT value;
        if (load_from_cache(key, value)) {
            return value;
        }

        std::shared_lock lock(mutex_);
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (data_[i].has_value() && data_[i]->key == key) {
                value = data_[i]->value;
                store_to_cache(i);
                return value;
            }
        }

        throw std::out_of_range("Key not found");
    }

    void clear() {
        data_.clear();
        cache_used_.clear();
        next_cache_slot_ = 0;
    }

    std::pair<KeyT, ValueT> iterate_elements() {
        if (!data_.size()) {
            throw std::out_of_range("Empty storage");
        }

        if (iterate_idx_ < data_.size() && data_[iterate_idx_].has_value()) {
            iterate_idx_ += 1;
            return std::make_pair(data_[iterate_idx_-1]->key, data_[iterate_idx_-1]->value);
        }

        iterate_idx_ = 1;
        return std::make_pair(data_[0]->key, data_[0]->value);
    }

private:
    std::size_t capacity_;
    std::size_t cache_size_;
    mutable std::shared_mutex mutex_;
    mutable std::mutex cache_mutex_;
    mutable std::size_t next_cache_slot_ = 0;
    std::size_t iterate_idx_ = 0;

    struct Entry {
        KeyT key;
        ValueT value;
    };

    std::vector<std::optional<Entry>> data_;
    std::vector<std::size_t> cache_;
    std::vector<bool> cache_used_;

    std::optional<std::size_t> find_index(const KeyT& key) const {
        std::shared_lock lock(mutex_);
        for (std::size_t i = 0; i < data_.size(); ++i) {
            if (data_[i].has_value() && data_[i]->key == key) {
                return i;
            }
        }
        return std::nullopt;
    }

    bool load_from_cache(const KeyT& key, ValueT& out) const {
        std::lock_guard lock(cache_mutex_);
        for (std::size_t i = 0; i < cache_size_; ++i) {
            if (cache_used_[i]) {
                const auto& opt_entry = data_[cache_[i]];
                if (opt_entry.has_value() && opt_entry->key == key) {
                    out = opt_entry->value;
                    return true;
                }
            }
        }
        return false;
    }

    void store_to_cache(std::size_t idx) {
        std::lock_guard lock(cache_mutex_);
        cache_[next_cache_slot_] = idx;
        cache_used_[next_cache_slot_] = true;
        next_cache_slot_ = (next_cache_slot_ + 1) % cache_size_;
    }

    FRIEND_TEST(StorageCacheTest, LoadFromCacheReturnsTrueIfPresent);
    FRIEND_TEST(StorageCacheTest, LoadFromCacheReturnsFalseIfAbsent);
    FRIEND_TEST(StorageCacheTest, StoreToCacheAddsEntry); 
    FRIEND_TEST(ClearTest, ClearEmptiesStorage);
    FRIEND_TEST(ClearTest, ClearEmptiesCache);
    FRIEND_TEST(StorageCacheTest, FindIndexReturnsNulloptIfKeyNotFound);
};