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
        if (storage) {
            operator delete[](storage, std::align_val_t(YTALLOC_STATIC_ALIGN));
        }
        for (DuplicatedWrite &write : writes) {
            write.delete_copy();
        }
    }

    void set_underlying_storage(size_t size) {
        storage = new (std::align_val_t(YTALLOC_STATIC_ALIGN)) uint8_t[size];
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

TEST_F(StaticTest, AllocZeroSize) {
    init_with_size(32);
    void *const ptr = alloc_static(&alloc, 0);
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(StaticTest, AllocOneTimeNotFull) {
    init_with_size(2 * YTALLOC_STATIC_ALIGN);

    void *const ptr = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr, nullptr);

    random_write(ptr, YTALLOC_STATIC_ALIGN);
    check_writes();
}

TEST_F(StaticTest, AllocOneTimeFull) {
    init_with_size(YTALLOC_STATIC_ALIGN);

    void *const ptr1 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    EXPECT_EQ(ptr2, nullptr);

    random_write(ptr1, YTALLOC_STATIC_ALIGN);
    check_writes();
}

TEST_F(StaticTest, AllocTwoTimesNotFull) {
    init_with_size(3 * YTALLOC_STATIC_ALIGN);

    void *const ptr1 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr2, nullptr);

    random_write(ptr1, YTALLOC_STATIC_ALIGN);
    random_write(ptr2, YTALLOC_STATIC_ALIGN);

    check_writes();
}

TEST_F(StaticTest, AllocTwoTimesFull) {
    init_with_size(2 * YTALLOC_STATIC_ALIGN);

    void *const ptr1 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr1, nullptr);

    void *const ptr2 = alloc_static(&alloc, YTALLOC_STATIC_ALIGN);
    ASSERT_NE(ptr2, nullptr);

    void *const ptr3 = alloc_static(&alloc, 1);
    EXPECT_EQ(ptr3, nullptr);

    random_write(ptr1, YTALLOC_STATIC_ALIGN);
    random_write(ptr2, YTALLOC_STATIC_ALIGN);

    check_writes();
}

TEST_F(StaticTest, AllocReturnsAlignedPointers) {
    init_with_size(2 * YTALLOC_STATIC_ALIGN);

    void *const ptr1 = alloc_static(&alloc, 1);
    void *const ptr2 = alloc_static(&alloc, 1);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr1, ptr2);

    constexpr size_t expected_alignment = YTALLOC_STATIC_ALIGN;

    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % expected_alignment, 0);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % expected_alignment, 0);
}
