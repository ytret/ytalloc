#include <gtest/gtest.h>
#include <random>
#include <ytalloc/ytalloc.h>

#include "tests_common/DuplicatedWrite.h"

class ListHeapTest : public testing::Test {
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

    void init_with_size(size_t size) {
        set_underlying_storage(size);
        alloc_list_init(&heap, storage, size);
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

    alloc_list_t heap;
    uint8_t *storage;
    size_t size;

    std::minstd_rand rng;
    std::vector<DuplicatedWrite> writes;
};

TEST_F(ListHeapTest, InitNullHeapAborts) {
    set_underlying_storage(64);
    ASSERT_DEATH(alloc_list_init(NULL, storage, size), "");
}
TEST_F(ListHeapTest, InitNullStartAborts) {
    set_underlying_storage(64);
    ASSERT_DEATH(alloc_list_init(&heap, NULL, size), "");
}
TEST_F(ListHeapTest, InitZeroSizeAborts) {
    set_underlying_storage(64);
    ASSERT_DEATH(alloc_list_init(&heap, storage, 0), "");
}
TEST_F(ListHeapTest, InitInsufficientSizeAborts) {
    set_underlying_storage(1);
    ASSERT_DEATH(alloc_list_init(&heap, storage, 0), "");
}
TEST_F(ListHeapTest, InitMisalignedStartAborts) {
    set_underlying_storage(64);
    ASSERT_DEATH(alloc_list_init(&heap, &storage[1], size - 1), "");
}
TEST_F(ListHeapTest, Init) {
    set_underlying_storage(64);
    alloc_list_init(&heap, storage, size);
}

TEST_F(ListHeapTest, Alloc1Time) {
    init_with_size(128);

    void *const ptr1 = alloc_list(&heap, 16);
    ASSERT_NE(ptr1, nullptr);

    random_write(ptr1, 16);
    check_writes();
}

TEST_F(ListHeapTest, Alloc2Times) {
    init_with_size(256);

    void *const ptr1 = alloc_list(&heap, 16);
    ASSERT_NE(ptr1, nullptr);
    void *const ptr2 = alloc_list(&heap, 16);
    ASSERT_NE(ptr2, nullptr);

    random_write(ptr1, 16);
    random_write(ptr2, 16);
    check_writes();
}

TEST_F(ListHeapTest, AllocFull) {
    init_with_size(1024);

    while (true) {
        void *const ptr = alloc_list(&heap, 16);
        if (!ptr) { break; }
        random_write(ptr, 16);
    }

    check_writes();
}

TEST_F(ListHeapTest, Free) {
    init_with_size(128);

    for (size_t idx = 0; idx < 32; idx++) {
        void *const ptr = alloc_list(&heap, 16);
        ASSERT_NE(ptr, nullptr) << "probably alloc_list_free() does not free";
        random_write(ptr, 16, false);
        alloc_list_free(&heap, ptr);
    }
}
