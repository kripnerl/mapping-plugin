// Dependency-free test of the row-major to column-major conversion applied to
// every array of rank 2 or more that the plugin returns to UDA. Built only with
// -DMAPPING_PLUGIN_TESTS=ON; needs neither UDA nor libtokamap.

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "array_order.hpp"

namespace
{

int failures = 0;

template <typename T> std::vector<T> reorder(const std::vector<T>& row_major, const std::vector<std::size_t>& shape)
{
    std::vector<T> column_major(row_major.size());
    mapping_plugin::to_column_major(reinterpret_cast<const char*>(row_major.data()), shape, sizeof(T),
                                    reinterpret_cast<char*>(column_major.data()));
    return column_major;
}

template <typename T> void expect_equal(const std::string& name, const std::vector<T>& actual, const std::vector<T>& expected)
{
    if (actual == expected) {
        std::cout << "ok      " << name << "\n";
        return;
    }
    ++failures;
    std::cout << "FAILED  " << name << "\n  actual:  ";
    for (const auto& value : actual) {
        std::cout << +value << " ";
    }
    std::cout << "\n  expected:";
    for (const auto& value : expected) {
        std::cout << " " << +value;
    }
    std::cout << "\n";
}

std::vector<double> iota(std::size_t size)
{
    std::vector<double> values(size);
    std::iota(values.begin(), values.end(), 0.0);
    return values;
}

} // namespace

int main()
{
    // Element (i, j) of a (2, 3) array holds 3*i + j when stored row-major.
    // Column-major storage puts it at i + 2*j.
    expect_equal<double>("non-square rank 2", reorder(iota(6), {2, 3}), {0, 3, 1, 4, 2, 5});

    // A square array comes out as its transpose, which keeps its shape: the
    // case that hid the defect for the 33 x 33 EFIT poloidal flux map.
    expect_equal<double>("square rank 2 is transposed", reorder(iota(4), {2, 2}), {0, 2, 1, 3});

    // Rank 3: element (i, j, k) of a (2, 3, 4) array moves from 12*i + 4*j + k
    // to i + 2*j + 6*k.
    {
        const std::vector<double> source = iota(24);
        std::vector<double> expected(24);
        for (std::size_t i = 0; i < 2; ++i) {
            for (std::size_t j = 0; j < 3; ++j) {
                for (std::size_t k = 0; k < 4; ++k) {
                    expected[i + (2 * j) + (6 * k)] = source[(12 * i) + (4 * j) + k];
                }
            }
        }
        expect_equal<double>("rank 3", reorder(source, {2, 3, 4}), expected);
    }

    // Rank 1 and rank 0 have one layout only.
    expect_equal<double>("rank 1 is unchanged", reorder(iota(5), {5}), iota(5));
    expect_equal<double>("rank 0 is unchanged", reorder<double>({42.0}, {}), {42.0});

    // The element size is honoured for narrower types.
    expect_equal<std::int8_t>("one-byte elements", reorder<std::int8_t>({1, 2, 3, 4, 5, 6}, {3, 2}), {1, 3, 5, 2, 4, 6});
    expect_equal<std::int32_t>("four-byte elements", reorder<std::int32_t>({1, 2, 3, 4, 5, 6}, {3, 2}), {1, 3, 5, 2, 4, 6});

    // A dimension of length one does not disturb the others.
    expect_equal<double>("singleton dimension", reorder(iota(6), {1, 2, 3}), {0, 3, 1, 4, 2, 5});

    if (failures != 0) {
        std::cout << failures << " test(s) failed\n";
        return 1;
    }
    return 0;
}
