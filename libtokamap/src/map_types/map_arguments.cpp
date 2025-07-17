#include "map_arguments.hpp"

#include <boost/algorithm/string.hpp>
#include <string>
#include <string_view>
#include <cstddef>
#include <vector>
#include <cstdint>

namespace {
    using Indices = std::vector<size_t>;
    using IndicesList = std::vector<Indices>;

    // Generate all index tuples for given start, stop, stride arrays
    IndicesList generate_indices(const std::vector<json_mapping::SubsetInfo>& subsets)
    {
        size_t n_dims = subsets.size();
        IndicesList result;
        Indices current(n_dims);

        // Initialize current index to start
        for (size_t i = 0; i < n_dims; ++i) {
            current[i] = subsets[i].start();
        }

        bool done = false;
        while (!done) {
            result.push_back(current);

            // Increment the last dimension and carry over if needed
            for (int64_t i = static_cast<int64_t>(n_dims) - 1; i >= 0; --i) {
                current[i] += subsets[i].stride();

                if (current[i] < subsets[i].stop()) {
                    break; // no carry needed
                }
                if (i == 0) {
                    done = true; // we're finished
                } else {
                    current[i] = subsets[i].start(); // reset and carry to next dimension
                }
            }
        }

        return result;
    }

    size_t compute_offset(const Indices& indices, const std::vector<size_t>& index_factors)
    {
        size_t offset = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            offset += indices[i] * index_factors[i];
        }
        return offset;
    }

    std::vector<size_t> compute_index_factors(const std::vector<size_t>& shape)
    {
        size_t n_dims = shape.size();
        std::vector<size_t> factors(n_dims);
        factors[n_dims - 1] = 1;
        for (int64_t i = static_cast<int64_t>(n_dims) - 2; i >= 0; --i) {
            factors[i] = factors[i + 1] * shape[i + 1];
        }
        return factors;
    }
} // anon namespace

json_mapping::SignalType json_mapping::deduce_signal_type(std::string_view final_path_element)
{
    // SignalType useful in determining for MAST-U
    SignalType sig_type{SignalType::DEFAULT};
    if (final_path_element.empty()) {
        sig_type = SignalType::INVALID;
    } else if (final_path_element == "data") {
        sig_type = SignalType::DATA;
    } else if (final_path_element == "time") {
        sig_type = SignalType::TIME;
    } else if (final_path_element.find("error") != std::string::npos) {
        sig_type = SignalType::ERROR;
    }
    return sig_type;
}

std::vector<size_t> json_mapping::compute_offsets(const std::vector<size_t>& shape, const std::vector<SubsetInfo>& subsets)
{
    auto indices_list = generate_indices(subsets);
    auto index_factors = compute_index_factors(shape);

    std::vector<size_t> offsets;
    for (const auto& indices : indices_list) {
        offsets.push_back(compute_offset(indices, index_factors));
    }
    return offsets;
}
