#include <gtest/gtest.h>

#include "storage.hpp"

TEST(StorageTest, CanCreateAndDestroy) {
    Storage<int, std::string> storage(100, 10);
}

TEST(StorageTest, ThrowsOnZeroCapacity) {
    EXPECT_THROW((Storage<int, int>(10, 0)), std::invalid_argument);
}

TEST(StorageTest, ThrowsOnZeroCacheSize) {
    EXPECT_THROW((Storage<int, int>(10, 0)), std::invalid_argument);
}

TEST(StorageTest, ThrowsWhenCacheLargerThanCapacity) {
    EXPECT_THROW((Storage<int, int>(10, 20)), std::invalid_argument);
}