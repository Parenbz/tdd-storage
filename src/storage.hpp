#pragma once

#include <cstddef>
#include <stdexcept>

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

private:
    std::size_t capacity_;
    std::size_t cache_size_;
};