#include "tests_common/DuplicatedWrite.h"

DuplicatedWrite DuplicatedWrite::random_write(std::minstd_rand rng,
                                              void *v_dest, size_t num_bytes) {
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

    return DuplicatedWrite{
        .dest = v_dest,
        .copy = u8_copy,
        .num_bytes = num_bytes,
    };
}

bool DuplicatedWrite::check_integrity() const {
    return !memcmp(dest, copy, num_bytes);
}

void DuplicatedWrite::delete_copy() {
    delete[] copy;
}
