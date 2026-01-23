#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>

/**
 * A mechanism to check data integrity by duplicating the original data into a
 * safe location and later checking the original memory location against it.
 *
 * - The copy is automatically created on the heap when using #random_write().
 * - Data integrity can be checked using #check_integrity().
 * - The copy can be deleted using #delete_copy().
 *
 * @warning
 * The default destructor does not delete the copy, you need to call
 * #delete_copy() manually.
 */
struct DuplicatedWrite {
    void *dest = nullptr;
    uint8_t *copy = nullptr;
    size_t num_bytes;

    DuplicatedWrite() = delete;

    /**
     * Writes @a num_bytes random bytes to @a v_dest and remembers them.
     *
     * @param rng       Random number generator used to generate the bytes
     *                  written to @a v_dest.
     * @param v_dest    Destination buffer of size @a num_bytes.
     * @param num_bytes The number of bytes to write.
     */
    static DuplicatedWrite random_write(std::minstd_rand rng, void *v_dest,
                                        size_t num_bytes);

    /**
     * Checks whether @a num_bytes at @a dest and @a copy are the same.
     *
     * @returns `true` if they hold the same data, otherwise `false`.
     */
    bool check_integrity() const;

    /**
     * Deletes the memory allocated for the duplicate.
     *
     * After this, the object becomes effectively useless and should be
     * deconstructed.
     */
    void delete_copy();
};
