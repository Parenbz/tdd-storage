#include <gtest/gtest.h>

#include "storage.hpp"

TEST(StorageTest, CanCreateAndDestroy) {
    Storage<int, std::string> storage(100, 10);
}