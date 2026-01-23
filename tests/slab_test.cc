#include <gtest/gtest.h>
#include <random>
#include <ytalloc/ytalloc.h>

#include "tests_common/DuplicatedWrite.h"

class SlabHeapTest : public testing::Test {
  protected:
    void SetUp() override {
        storage = nullptr;
    }

    void TearDown() override {
        if (storage) { delete[] storage; }
        for (DuplicatedWrite &write : writes) {
            write.delete_copy();
        }
    }

    void set_underlying_storage(size_t size) {
        storage = new uint8_t[size];
        this->size = size;
    }

    void init_with_size(size_t size, size_t alloc_size) {
        set_underlying_storage(size);
        alloc_slab_init(&heap, storage, size, alloc_size);
    }

    void random_write(void *ptr, size_t num_bytes, bool save = true) {
        auto write = DuplicatedWrite::random_write(rng, ptr, num_bytes);
        if (save) {
            writes.push_back(write);
        } else {
            write.delete_copy();
        }
    }

    void check_writes() {
        size_t idx = 0;
        for (const DuplicatedWrite &write : writes) {
            EXPECT_TRUE(write.check_integrity())
                << "write #" << idx << " (" << write.num_bytes
                << " bytes) has been overwritten";
        }
    }

    alloc_slab_t heap;
    uint8_t *storage;
    size_t size;

    std::minstd_rand rng;
    std::vector<DuplicatedWrite> writes;
};

TEST_F(SlabHeapTest, InitWithNullHeapAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_slab_init(NULL, storage, size, 8), "");
}

TEST_F(SlabHeapTest, InitWithNullStartAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_slab_init(&heap, NULL, size, 8), "");
}

TEST_F(SlabHeapTest, InitWithZeroSizeAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_slab_init(&heap, storage, 0, 8), "");
}

TEST_F(SlabHeapTest, InitWithSmallAllocSizeAborts) {
    set_underlying_storage(32);
    ASSERT_DEATH(alloc_slab_init(&heap, storage, 8, 2), "");
}

TEST_F(SlabHeapTest, AllocNotFull) {
    init_with_size(32, 8);

    void *const ptr = alloc_slab(&heap);
    ASSERT_NE(ptr, nullptr);

    random_write(ptr, 8);
    check_writes();
}

TEST_F(SlabHeapTest, AllocWhole) {
    init_with_size(8, 8);

    void *const ptr = alloc_slab(&heap);
    ASSERT_NE(ptr, nullptr);

    random_write(ptr, 8);
    check_writes();
}

TEST_F(SlabHeapTest, AllocUntilFull) {
    init_with_size(32, 8);

    void *const ptr1 = alloc_slab(&heap);
    ASSERT_NE(ptr1, nullptr);
    void *const ptr2 = alloc_slab(&heap);
    ASSERT_NE(ptr2, nullptr);
    void *const ptr3 = alloc_slab(&heap);
    ASSERT_NE(ptr3, nullptr);
    void *const ptr4 = alloc_slab(&heap);
    ASSERT_NE(ptr4, nullptr);

    void *const ptr5 = alloc_slab(&heap);
    ASSERT_EQ(ptr5, nullptr);

    random_write(ptr1, 8);
    random_write(ptr2, 8);
    random_write(ptr3, 8);
    random_write(ptr4, 8);
    check_writes();
}

TEST_F(SlabHeapTest, AllocFree) {
    init_with_size(16, 8);

    void *const ptr = alloc_slab(&heap);
    ASSERT_NE(ptr, nullptr);

    alloc_slab_free(&heap, ptr);
}

TEST_F(SlabHeapTest, AllocFreeAlloc) {
    init_with_size(16, 8);

    void *const ptr1 = alloc_slab(&heap);
    ASSERT_NE(ptr1, nullptr);

    alloc_slab_free(&heap, ptr1);

    void *const ptr2 = alloc_slab(&heap);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_EQ(ptr2, ptr1);
}

TEST_F(SlabHeapTest, InterleavingAllocFrees) {
    init_with_size(32, 8);

    void *const ptr1_1 = alloc_slab(&heap);
    ASSERT_NE(ptr1_1, nullptr);
    void *const ptr2 = alloc_slab(&heap);
    ASSERT_NE(ptr2, nullptr);
    void *const ptr3_1 = alloc_slab(&heap);
    ASSERT_NE(ptr3_1, nullptr);

    alloc_slab_free(&heap, ptr1_1);
    void *const ptr1_2 = alloc_slab(&heap);
    ASSERT_NE(ptr1_2, nullptr);
    ASSERT_EQ(ptr1_2, ptr1_1);

    void *const ptr4 = alloc_slab(&heap);
    ASSERT_NE(ptr4, nullptr);

    alloc_slab_free(&heap, ptr3_1);
    void *const ptr3_2 = alloc_slab(&heap);
    ASSERT_NE(ptr3_2, nullptr);
    ASSERT_EQ(ptr3_2, ptr3_1);

    random_write(ptr1_2, 8);
    random_write(ptr2, 8);
    random_write(ptr3_1, 8);
    random_write(ptr4, 8);
    check_writes();
}
