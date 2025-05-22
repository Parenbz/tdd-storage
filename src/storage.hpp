#pragma once

#include <cstddef>
#include <stdexcept>

template<typename KeyT, typename ValueT>
class Storage {
public:
    Storage(std::size_t capacity, std::size_t cache_size)
        : capacity_(capacity), cache_size_(cache_size) {}

private:
    std::size_t capacity_;
    std::size_t cache_size_;
};