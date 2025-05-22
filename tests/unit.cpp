#include <gtest/gtest.h>
#include <thread>
#include <vector>

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

TEST(StorageTest, LoadThrowsIfKeyDoesNotExist) {
    Storage<int, std::string> storage(10, 5);
    EXPECT_THROW(storage.load(999), std::out_of_range);
}

TEST(StorageTest, StoreThenLoadReturnsSameValue) {
    Storage<int, std::string> storage(10, 5);
    storage.store(42, "value");
    EXPECT_EQ(storage.load(42), "value");
}

TEST(StorageTest, StoreOverwritesValue) {
    Storage<int, std::string> storage(10, 5);
    storage.store(1, "first");
    storage.store(1, "second");
    EXPECT_EQ(storage.load(1), "second");
}

TEST(StorageTest, CanStoreUpToCapacityUniqueKeys) {
    Storage<int, int> storage(3, 2);
    storage.store(1, 100);
    storage.store(2, 200);
    storage.store(3, 300);
    EXPECT_EQ(storage.load(1), 100);
    EXPECT_EQ(storage.load(2), 200);
    EXPECT_EQ(storage.load(3), 300);
}

TEST(StorageTest, ConcurrentStoreDifferentKeys) {
    Storage<int, std::string> storage(100, 10);
    auto store_fn = [&](int id) {
        storage.store(id, "v" + std::to_string(id));
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(store_fn, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(storage.load(i), "v" + std::to_string(i));
    }
}

TEST(StorageTest, ConcurrentStoreSameKey) {
    Storage<int, int> storage(10, 2);
    auto store_fn = [&]() {
        for (int i = 0; i < 1000; ++i) {
            storage.store(1, i);
        }
    };

    std::thread t1(store_fn);
    std::thread t2(store_fn);

    t1.join();
    t2.join();

    EXPECT_NO_THROW(storage.load(1));
}

TEST(StorageTest, ConcurrentLoadAfterStore) {
    Storage<int, int> storage(10, 2);
    storage.store(5, 123);

    auto load_fn = [&]() {
        for (int i = 0; i < 1000; ++i) {
            EXPECT_EQ(storage.load(5), 123);
        }
    };

    std::thread t1(load_fn);
    std::thread t2(load_fn);
    t1.join();
    t2.join();
}