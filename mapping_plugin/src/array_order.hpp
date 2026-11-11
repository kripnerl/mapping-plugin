#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

namespace mapping_plugin
{

/**
 * @brief Copy a row-major array into column-major order
 *
 * libtokamap keeps an array row-major: with shape (d0, d1, ...) the LAST index
 * varies fastest, as in C and NumPy, and its slicing relies on that. UDA and
 * the IMAS Access Layer read a data block column-major: the FIRST index varies
 * fastest, as in Fortran. The dimension order is the same on both sides, so
 * handing the buffer over unchanged gives the right shape with misplaced
 * values — a transpose for a square array, a scramble otherwise.
 *
 * Arrays of rank 0 and 1 have the same layout in both orders.
 *
 * @param row_major source buffer holding product(shape) elements
 * @param shape extent of each dimension, in libtokamap order
 * @param element_size size of one element in bytes
 * @param column_major destination buffer of the same size; must not overlap
 *        the source
 */
inline void to_column_major(const char* row_major, const std::vector<std::size_t>& shape, std::size_t element_size,
                            char* column_major)
{
    const std::size_t rank = shape.size();

    std::size_t size = 1;
    // Offset, in elements, that one step along each dimension adds in the
    // column-major destination
    std::vector<std::size_t> strides(rank);
    for (std::size_t dim = 0; dim < rank; ++dim) {
        strides[dim] = size;
        size *= shape[dim];
    }

    // Walk the source in storage order while tracking its multi-index
    std::vector<std::size_t> index(rank, 0);
    for (std::size_t source = 0; source < size; ++source) {
        std::size_t target = 0;
        for (std::size_t dim = 0; dim < rank; ++dim) {
            target += index[dim] * strides[dim];
        }
        std::memcpy(column_major + (target * element_size), row_major + (source * element_size), element_size);

        // Row-major increment: the last index moves first
        for (std::size_t dim = rank; dim-- > 0;) {
            if (++index[dim] < shape[dim]) {
                break;
            }
            index[dim] = 0;
        }
    }
}

} // namespace mapping_plugin
