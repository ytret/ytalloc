#include <gtest/gtest.h>
#include <random>
#include <ytalloc/ytalloc.h>

#include "tests_common/DuplicatedWrite.h"

class BuddyTest : public testing::Test {
  protected:
    void SetUp() override {
        storage = nullptr;
    }

    void TearDown() override {
        if (storage) {
            operator delete[](storage, std::align_val_t(alignment));
        }
        for (DuplicatedWrite &write : writes) {
            write.delete_copy();
        }
    }

    void set_underlying_storage(size_t size, size_t alignment) {
        storage = new (std::align_val_t(alignment)) uint8_t[size];
        this->size = size;
        this->alignment = alignment;
    }

    void init_with_size(size_t size, size_t alignment) {
        set_underlying_storage(size, alignment);
        alloc_buddy_init(&alloc, storage, size);
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

    alloc_buddy_t alloc;
    uint8_t *storage;
    size_t size;
    size_t alignment;

    std::vector<DuplicatedWrite> writes;
};

TEST_F(BuddyTest, InitNullHeapAborts) {
    set_underlying_storage(YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                           YTALLOC_BUDDY_MIN_BLOCK_SIZE);
    ASSERT_DEATH(alloc_buddy_init(NULL, storage, size), "");
}

TEST_F(BuddyTest, InitNullStartAborts) {
    set_underlying_storage(YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                           YTALLOC_BUDDY_MIN_BLOCK_SIZE);
    ASSERT_DEATH(alloc_buddy_init(&alloc, NULL, size), "");
}

TEST_F(BuddyTest, InitZeroSizeAborts) {
    set_underlying_storage(YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                           YTALLOC_BUDDY_MIN_BLOCK_SIZE);
    ASSERT_DEATH(alloc_buddy_init(&alloc, storage, 0), "");
}

TEST_F(BuddyTest, InitMisalignedStartAborts) {
    set_underlying_storage(YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                           YTALLOC_BUDDY_MIN_BLOCK_SIZE / 2);
    ASSERT_DEATH(
        alloc_buddy_init(&alloc, (void *)((uintptr_t)storage + 8), size), "");
}

TEST_F(BuddyTest, AllocZeroSize) {
    init_with_size(YTALLOC_BUDDY_MIN_BLOCK_SIZE, YTALLOC_BUDDY_MIN_BLOCK_SIZE);
    void *const ptr = alloc_buddy(&alloc, 0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(BuddyTest, AllocMaxSize_1Alloc) {
    init_with_size(YTALLOC_BUDDY_MIN_BLOCK_SIZE, YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr, nullptr);

    random_write(ptr, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    check_writes();
}

TEST_F(BuddyTest, AllocMaxSize_2Allocs) {
    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr2, nullptr);

    random_write(ptr1, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    random_write(ptr2, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    check_writes();
}

TEST_F(BuddyTest, AllocMaxSize_3rdAllocFails) {
    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr2, nullptr);

    void *const ptr3 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_EQ(ptr3, nullptr);

    random_write(ptr1, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    random_write(ptr2, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    check_writes();
}

TEST_F(BuddyTest, AllocTooMuchFails) {
    init_with_size(YTALLOC_BUDDY_MIN_BLOCK_SIZE, YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr = alloc_buddy(&alloc, 2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);
    ASSERT_EQ(ptr, nullptr);
}

TEST_F(BuddyTest, AllocRoundUpSize) {
    init_with_size(YTALLOC_BUDDY_MIN_BLOCK_SIZE, YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_ALLOC_SIZE);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_buddy(&alloc, 1);
    ASSERT_EQ(ptr2, nullptr);
}

TEST_F(BuddyTest, AllocNoSuitableBlock) {
    // 64 -> 32 | 32
    // 1. Allocate 16 bytes -> ideally, 48 bytes are left free. But actually
    //    32 bytes are left free.
    // 2. Try to allocate the ideally free 32 bytes and fail.

    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_BLOCK_SIZE / 2);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_BLOCK_SIZE + 1);
    ASSERT_EQ(ptr2, nullptr);
}

TEST_F(BuddyTest, AllocNoSuitableBlock_WithFree) {
    // Same as AllocNoSuitableBlock, except this time free the first allocation
    // to make the second one work.

    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_BLOCK_SIZE / 2);
    ASSERT_NE(ptr1, nullptr);

    alloc_buddy_free(&alloc, ptr1);

    void *const ptr2 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_BLOCK_SIZE + 1);
    ASSERT_NE(ptr2, nullptr);
}

TEST_F(BuddyTest, AllocSplitMerge) {
    // 1. Split the 64 byte block into two 32 byte blocks by allocating <32
    //    bytes.
    // 2. Free the allocated block.
    // 3. Expect the two 32 byte blocks to be merged into the original 64 byte
    //    block.

    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    ASSERT_EQ(alloc.num_orders, 2);
    ASSERT_EQ(alloc.free_heads[0], 0);

    void *const ptr1 = alloc_buddy(&alloc, YTALLOC_BUDDY_MIN_BLOCK_SIZE / 2);
    ASSERT_NE(ptr1, nullptr);

    ASSERT_NE(alloc.free_heads[0], 0);
    ASSERT_EQ(alloc.free_heads[1], 0);

    alloc_buddy_free(&alloc, ptr1);

    ASSERT_EQ(alloc.free_heads[0], 0);
    ASSERT_NE(alloc.free_heads[1], 0);
}

TEST_F(BuddyTest, AllocSplitMerge2) {
    // Same as AllocSplitMerge, except this time allocate both order 0 blocks
    // and free them in such order that the right buddy is freed last.

    init_with_size(2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE,
                   2 * YTALLOC_BUDDY_MIN_BLOCK_SIZE);

    ASSERT_EQ(alloc.num_orders, 2);
    ASSERT_EQ(alloc.free_heads[0], 0);

    void *const ptr1 = alloc_buddy(&alloc, 1);
    ASSERT_NE(ptr1, nullptr);
    void *const ptr2 = alloc_buddy(&alloc, 1);
    ASSERT_NE(ptr2, nullptr);

    ASSERT_EQ(alloc.free_heads[0], 0);
    ASSERT_EQ(alloc.free_heads[1], 0);

    alloc_buddy_free(&alloc, ptr1);
    alloc_buddy_free(&alloc, ptr2);

    ASSERT_EQ(alloc.free_heads[0], 0);
    ASSERT_NE(alloc.free_heads[1], 0);
}
