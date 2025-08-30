#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>

/**
 * A mechanism to check data integrity by copying the original data and later
 * checking against it.
 *
 * - The copy is automatically created on the heap in #random_write().
 * - Data integrity is checked with #check_integrity().
 * - The copy is deleted with #delete_copy().
 *
 * @warning
 * The default destructor does not delete the copy, use #delete_copy().
 */
struct DuplicatedWrite {
    void *dest = nullptr;
    uint8_t *copy = nullptr;
    size_t num_bytes;

    DuplicatedWrite() = delete;

    /**
     * Writes @a num_bytes random bytes to @a v_dest and remembers them.
     * @param rng       Random number generator used to generate the bytes
     *                  written to @a v_dest.
     * @param v_dest    Destination buffer of size @a num_bytes.
     * @param num_bytes The number of bytes to write.
     */
    static DuplicatedWrite random_write(std::minstd_rand rng, void *v_dest,
                                        size_t num_bytes);

    /**
     * Checks whether @a num_bytes at @a dest and @a copy are the same.
     * @returns `true` if they hold the same data, otherwise `false`.
     */
    bool check_integrity() const;

    /**
     * Deletes the memory allocated for the duplicate.
     * After this, the object becomes effectively useless and should be
     * deconstructed.
     */
    void delete_copy();
};
