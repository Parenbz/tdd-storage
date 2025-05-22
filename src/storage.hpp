#pragma once

#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <shared_mutex>

template<typename KeyT, typename ValueT>
class Storage {
public:
    Storage(std::size_t capacity, std::size_t cache_size)
        : capacity_(capacity), cache_size_(cache_size) {
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

        if (storage_.size() >= capacity_ && storage_.find(key) == storage_.end()) {
            throw std::runtime_error("Storage capacity exceeded");
        }

        storage_[key] = value;
    }

    ValueT load(const KeyT& key) {
        std::shared_lock lock(mutex_);
        auto it = storage_.find(key);
        if (it == storage_.end()) {
            throw std::out_of_range("Key not found");
        }
        return it->second;
    }

private:
    std::size_t capacity_;
    std::size_t cache_size_;
    std::unordered_map<KeyT, ValueT> storage_;
    mutable std::shared_mutex mutex_;
};