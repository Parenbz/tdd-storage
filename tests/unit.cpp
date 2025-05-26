#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <unistd.h>

#include "storage.hpp"

using namespace testing;

TEST(StorageTest, CanCreateAndDestroy) {
    Storage<int, std::string> storage(100, 10);
}

TEST(StorageTest, ThrowsOnZeroCapacity) {
    EXPECT_THROW((Storage<int, int>(0, 0)), std::invalid_argument);
}

TEST(StorageTest, ThrowOnFullCapacity) {
    Storage<int, int> storage(2, 1);
    EXPECT_NO_THROW(storage.store(1, 100));
    EXPECT_NO_THROW(storage.store(2, 200));
    EXPECT_THROW(storage.store(3, 300), std::runtime_error);
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

TEST(StorageTest, ClearDuringLoad) {
    Storage<int, int> storage(10000, 1);
    for (int i = 0; i < 10000; i++) {
        storage.store(i, i);
    }

    auto load_fn = [&]() {
        storage.load(9000);
    };

    auto clear_fn = [&]() {
        storage.clear();
    };

    std::thread t1(load_fn);
    sleep(1);
    std::thread t2(clear_fn);
    t1.join();
    t2.join();
}

class StorageCacheTest : public ::testing::Test {
protected:
    Storage<int, std::string> storage{10, 3};

    void SetUp() override {
        storage.store(1, "one");
        storage.store(2, "two");
    }
};

TEST_F(StorageCacheTest, LoadFromCacheReturnsFalseIfAbsent) {
    std::string value;
    EXPECT_FALSE(storage.load_from_cache(999, value));
}

TEST_F(StorageCacheTest, LoadFromCacheReturnsTrueIfPresent) {
    std::string value;

    auto idx_opt = storage.find_index(1);
    ASSERT_TRUE(idx_opt.has_value());
    storage.store_to_cache(idx_opt.value());

    EXPECT_TRUE(storage.load_from_cache(1, value));
    EXPECT_EQ(value, "one");
}

TEST_F(StorageCacheTest, StoreToCacheAddsEntry) {
    auto idx_opt = storage.find_index(2);
    ASSERT_TRUE(idx_opt.has_value());

    storage.store_to_cache(idx_opt.value());

    std::string value;
    EXPECT_TRUE(storage.load_from_cache(2, value));
    EXPECT_EQ(value, "two");
}

TEST_F(StorageCacheTest, FindIndexReturnsNulloptIfKeyNotFound) {
    Storage<int, int> storage(2, 1);
    auto result = storage.find_index(42);
    EXPECT_EQ(result, std::nullopt);
}

class ClearTest : public ::testing::Test {
protected:
    Storage<int, std::string> storage{5, 2};

    void SetUp() override {
        storage.store(1, "one");
        storage.store(2, "two");
    }
};

TEST_F(ClearTest, ClearEmptiesStorage) {
    storage.clear();
    EXPECT_THROW(storage.load(1), std::out_of_range);
    EXPECT_THROW(storage.load(2), std::out_of_range);
}

TEST_F(ClearTest, ClearEmptiesCache) {
    storage.clear();
    std::string value;
    EXPECT_FALSE(storage.load_from_cache(1, value));
    EXPECT_FALSE(storage.load_from_cache(2, value));
}

TEST_F(ClearTest, ConcurrentClear) {
    auto clear_fn = [&]() {
        storage.clear();
    };

    std::thread t1(clear_fn);
    std::thread t2(clear_fn);
    t1.join();
    t2.join();

    EXPECT_THROW(storage.load(1), std::out_of_range);
    EXPECT_THROW(storage.load(2), std::out_of_range);
}

class IterateElementsTest : public ::testing::Test {
    protected:
        Storage<int, std::string> storage{5, 2};
    
        void SetUp() override {
            storage.store(1, "one");
            storage.store(2, "two");
        }
};

TEST_F(IterateElementsTest, IterateElementsWorksCorrectly) {
    std::pair<int, std::string> it = storage.iterate_elements();
    EXPECT_EQ(it.first, 1);
    EXPECT_EQ(it.second, "one");
    it = storage.iterate_elements();
    EXPECT_EQ(it.first, 2);
    EXPECT_EQ(it.second, "two");
    it = storage.iterate_elements();
    EXPECT_EQ(it.first, 1);
    EXPECT_EQ(it.second, "one");
}

TEST_F(IterateElementsTest, IterateElementsThrowsOnEmptyStorage) {
    storage.clear();
    EXPECT_THROW(storage.iterate_elements(), std::out_of_range);
}

TEST_F(IterateElementsTest, ParallelIterateElements) {
    auto iterate_elements_fn = [&]() {
        for (int i = 0; i < 100; i++) {
            storage.iterate_elements();
        }
    };

    std::thread t1(iterate_elements_fn);
    std::thread t2(iterate_elements_fn);
    t1.join();
    t2.join();
}

TEST_F(IterateElementsTest, StoreParallelWithIterateElements) {
    auto iterate_elements_fn = [&]() {
        for (int i = 0; i < 100; i++) {
            storage.iterate_elements();
        }
    };

    auto store_fn = [&]() {
        storage.store(3, "three");
    };

    std::thread t1(iterate_elements_fn);
    sleep(1);
    std::thread t2(store_fn);
    t1.join();
    t2.join();
}