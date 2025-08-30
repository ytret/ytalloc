#include <gtest/gtest.h>
#include <random>
#include <ytalloc/ytalloc.h>

struct RememberedWrite {
    void *dest = nullptr;
    uint8_t *copy = nullptr;
    size_t num_bytes;

    RememberedWrite() = delete;

    /**
     * Writes @a num_bytes random bytes to @a v_dest and remembers them.
     * @param rng       Random number generator used to generate the bytes
     *                  written to @a v_dest.
     * @param v_dest    Destination buffer of size @a num_bytes.
     * @param num_bytes The number of bytes to write.
     */
    static RememberedWrite random_write(std::minstd_rand rng, void *v_dest,
                                        size_t num_bytes) {
        uint8_t *const u8_dest = static_cast<uint8_t *>(v_dest);
        uint32_t *const u32_dest = static_cast<uint32_t *>(v_dest);

        size_t dword_idx;
        for (dword_idx = 0; dword_idx < num_bytes / 4; dword_idx++) {
            uint32_t *const dest_ptr = (uint32_t *)u32_dest + dword_idx;
            *dest_ptr = rng();
        }
        size_t byte_idx;
        for (byte_idx = num_bytes - 4 * dword_idx; byte_idx < num_bytes;
             byte_idx++) {
            uint8_t *const dest_ptr = u8_dest + byte_idx;
            *dest_ptr = static_cast<uint8_t>(rng() & 0xFF);
        }

        uint8_t *const u8_copy = new uint8_t[num_bytes];
        memcpy(u8_copy, v_dest, num_bytes);

        return RememberedWrite{
            .dest = v_dest,
            .copy = u8_copy,
            .num_bytes = num_bytes,
        };
    }

    /**
     * Checks whether @a num_bytes at @a dest and @a copy are the same.
     * @returns `true` if they hold the same data, otherwise `false`.
     */
    bool check() const {
        return !memcmp(dest, copy, num_bytes);
    }

    void delete_copy() {
        delete[] copy;
    }
};

class StaticTest : public testing::Test {
  protected:
    void SetUp() {
        storage = nullptr;
    }

    void TearDown() {
        if (storage) { delete[] storage; }
        for (RememberedWrite &write : writes) {
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
        auto write = RememberedWrite::random_write(rng, ptr, num_bytes);
        writes.push_back(write);
    }

    void check_writes() {
        size_t idx = 0;
        for (const RememberedWrite &write : writes) {
            EXPECT_TRUE(write.check())
                << "write #" << idx << " (" << write.num_bytes
                << " bytes) has been overwritten";
        }
    }

    std::minstd_rand rng;

    alloc_static_t alloc;
    uint8_t *storage;
    size_t size;

    std::vector<RememberedWrite> writes;
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

TEST_F(StaticTest, TestAlloc) {
    init_with_size(32);

    void *const ptr = alloc_static(&alloc, 16);

    ASSERT_NE(ptr, nullptr);
    random_write(ptr, 16);
    check_writes();
}
