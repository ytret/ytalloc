#include <gtest/gtest.h>
#include <random>
#include <ytalloc/ytalloc.h>

#include "tests_common/DuplicatedWrite.h"

class StaticTest : public testing::Test {
  protected:
    void SetUp() {
        storage = nullptr;
    }

    void TearDown() {
        if (storage) { delete[] storage; }
        for (DuplicatedWrite &write : writes) {
            write.delete_copy();
        }
    }

    void set_underlying_storage(size_t size) {
        storage = new uint8_t[size];
        this->size = size;
    }

    void init_with_size(size_t size) {
        set_underlying_storage(size);
        alloc_static_init(&alloc, storage, size);
    }

    void random_write(void *ptr, size_t num_bytes) {
        auto write = DuplicatedWrite::random_write(rng, ptr, num_bytes);
        writes.push_back(write);
    }

    void check_writes() {
        size_t idx = 0;
        for (const DuplicatedWrite &write : writes) {
            EXPECT_TRUE(write.check_integrity())
                << "write #" << idx << " (" << write.num_bytes
                << " bytes) has been overwritten";
        }
    }

    std::minstd_rand rng;

    alloc_static_t alloc;
    uint8_t *storage;
    size_t size;

    std::vector<DuplicatedWrite> writes;
};

TEST_F(StaticTest, InitNullHeapAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_static_init(NULL, storage, size), "");
}

TEST_F(StaticTest, InitNullStartAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_static_init(&alloc, NULL, size), "");
}

TEST_F(StaticTest, InitZeroSize) {
    set_underlying_storage(32);
    alloc_static_init(&alloc, storage, 0);
}

TEST_F(StaticTest, TestAllocZeroSize) {
    init_with_size(32);
    void *const ptr = alloc_static(&alloc, 0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(StaticTest, TestAlloc1NotFull) {
    init_with_size(32);

    void *const ptr = alloc_static(&alloc, 16);
    ASSERT_NE(ptr, nullptr);

    random_write(ptr, 16);
    check_writes();
}

TEST_F(StaticTest, TestAlloc1Full) {
    init_with_size(32);

    void *const ptr1 = alloc_static(&alloc, 32);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, 1);
    EXPECT_EQ(ptr2, nullptr);

    random_write(ptr1, 32);
    check_writes();
}

TEST_F(StaticTest, TestAlloc2NotFull) {
    init_with_size(32);

    void *const ptr1 = alloc_static(&alloc, 16);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, 8);
    ASSERT_NE(ptr2, nullptr);

    random_write(ptr1, 16);
    random_write(ptr2, 8);

    check_writes();
}

TEST_F(StaticTest, TestAlloc2Full) {
    init_with_size(32);

    void *const ptr1 = alloc_static(&alloc, 16);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, 16);
    ASSERT_NE(ptr2, nullptr);

    void *const ptr3 = alloc_static(&alloc, 1);
    EXPECT_EQ(ptr3, nullptr);

    random_write(ptr1, 16);
    random_write(ptr2, 16);

    check_writes();
}
