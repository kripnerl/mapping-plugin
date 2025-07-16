#include "subset.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctre/ctre.hpp>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "map_types/map_arguments.hpp"

namespace
{

class SubsetInfo
{
  public:
    SubsetInfo(int64_t start, int64_t stop, int64_t stride, uint64_t size)
        : m_start{start}, m_stop{stop}, m_stride{stride}, m_dim_size{size}
    {
        // negative indexes mean that many elements from the end
        if (start < 0) {
            m_start = size + start;
        }
        if (stop < 0) {
            m_stop = size + stop + 1;
        }
    }

    [[nodiscard]] uint64_t size() const { return (m_stop - m_start) / m_stride; }

    [[nodiscard]] bool validate() const
    {
        return m_start <= m_dim_size - 1 && m_stop <= m_dim_size && m_start <= m_stop && m_stride < m_dim_size;
    }

    [[nodiscard]] uint64_t start() const { return m_start; }

    [[nodiscard]] uint64_t stop() const { return m_stop; }

    [[nodiscard]] int64_t stride() const { return m_stride; }

    [[nodiscard]] uint64_t dim_size() const { return m_dim_size; }

  private:
    int64_t m_start;
    int64_t m_stop;
    int64_t m_stride = 1;
    uint64_t m_dim_size;
};

/*
 * The input_id "factors" of each dim into a flattened buffer of multi-dimensional
 * data is given by the product of the dimension lengths of all higher-frequency
 * dimensions preceding it. These are constants that don't need to be recalculated
 * in each loop.
 *
 * i.e. for 3d data, index is: i + (Ni * j) + (Ni * Nj * k)
 * "index factors" here would be {1, Ni, (Ni * Nj)}
 */
std::vector<unsigned int> get_index_factors(const std::vector<unsigned int>& dim_sizes)
{
    std::vector<unsigned int> factors = {1};
    for (unsigned int i = 1; i < dim_sizes.size(); ++i) {
        factors.emplace_back(factors[i - 1] * dim_sizes[i - 1]);
    }
    return factors;
}

/*
 * Get the linear input_id into a flattened buffer of multi-dimensional data
 * given the current index of each dimension and the previosly calculated
 * index factors.
 *
 * e.g. for 3d data: input_id = i + (Ni * j) + (Ni * Nj * k)
 * this is the dot-product of the vectors of position-index and input_id-factors
 * for each data dimension.
 *
 */
unsigned int get_input_offset(std::vector<unsigned int>& current_indices, std::vector<unsigned int>& index_factors)
{
    unsigned int result = 0;
    for (unsigned int i = 0; i < current_indices.size(); ++i) {
        result += current_indices[i] * index_factors[i];
    }
    return result;
}

int64_t to_int(std::optional<std::string> value, int64_t default_value)
{
    if (value && !value.value().empty()) {
        return std::stoi(value.value());
    }
    return default_value;
}

constexpr auto token_re = ctll::fixed_string{R"(\[([^\[\]]*)\])"};
constexpr auto slice_re = ctll::fixed_string{R"((\d*)(:(\d*)(:(-?\d*))?)?)"};

template <typename T> SubsetInfo parse_slice(const T& slice, size_t dimension)
{
    const auto& match = ctre::match<slice_re>(slice);
    if (match) {
        int64_t start = to_int(match.template get<1>().to_optional_string(), 0);
        int64_t stop = to_int(match.template get<3>().to_optional_string(), -1);
        int64_t stride = to_int(match.template get<5>().to_optional_string(), 1);
        auto subset = SubsetInfo{start, stop, stride, dimension};
        if (!subset.validate()) {
            throw std::runtime_error{"invalid subset: " + slice.to_string()};
        }
        return subset;
    }
    throw std::runtime_error{"invalid subset: " + slice.to_string()};
}

std::vector<SubsetInfo> parse_slices(const std::string& slice, const std::vector<size_t> shape)
{
    size_t dim_idx = 0;
    std::vector<SubsetInfo> subsets;
    for (const auto& token : ctre::search_all<token_re>(slice)) {
        if (dim_idx == shape.size()) {
            std::runtime_error{"to many slices provided"};
        }
        subsets.push_back(parse_slice(token.get<1>(), shape[dim_idx]));
        ++dim_idx;
    }
    return {};
}

template <typename T>
void do_subset(size_t idx, json_mapping::TypedDataArray& input, const SubsetInfo& subset, double scale_factor,
               double offset)
{
}

template <typename T>
void do_subset(json_mapping::TypedDataArray& input, const std::vector<SubsetInfo>& subsets, double scale_factor,
               double offset)
{
    size_t idx = 0;
    for (const auto& subset : subsets) {
        do_subset<T>(idx, input, subset, scale_factor, offset);
        ++idx;
    }
}

} // anon namespace

void json_mapping::subset::apply_subset(json_mapping::TypedDataArray& input, std::optional<std::string> slice,
                                        std::optional<float> scale_factor, std::optional<float> offset)
{
    std::vector<SubsetInfo> subset_info;
    if (slice) {
        subset_info = parse_slices(slice.value(), input.shape());
    }
    using json_mapping::DataType;
    switch (type_index_map(input.type_index())) {
        case DataType::Int:
            do_subset<int>(input, subset_info, scale_factor.value_or(1.), offset.value_or(0.));
            break;
        default:
            throw std::runtime_error{"unsupported data type"};
    }
}

// /*
//  * Subset a flattened buffer of multidimensional data and apply optional scale and offset factors
//  *
//  * avoids recursion
//  */
// template <typename T>
// gsl::span<T> json_mapping::subset::subset(gsl::span<T>& input, std::vector<SubsetInfo>& subset_dims,
//                                           double scale_factor, double offset)
// {

//     log(LogLevel::DEBUG, "input size: " + std::to_string(input.size()));
//     log(LogLevel::DEBUG, "input value 1: " + std::to_string(input[0]));
//     log(LogLevel::DEBUG, "scaling factor is " + std::to_string(scale_factor));
//     unsigned int result_length = 1;
//     std::vector<unsigned int> total_dim_lengths;
//     std::vector<unsigned int> current_indices;
//     for (const auto& subset_info : subset_dims) {
//         result_length *= subset_info.size();
//         total_dim_lengths.emplace_back(subset_info.dim_size());
//         current_indices.emplace_back(subset_info.start());
//     }

//     std::vector<unsigned int> factors = get_index_factors(total_dim_lengths);
//     log(LogLevel::DEBUG, "result length is: " + std::to_string(result_length));
//     std::vector<T> result(result_length);
//     for (unsigned int output_id = 0; output_id < result_length; ++output_id) {
//         // increment vector of current_indices (cascading when they roll-over)
//         for (unsigned int k = 0; k < subset_dims.size() and current_indices[k] >= subset_dims[k].stop(); ++k) {
//             current_indices[k] = subset_dims[k].start();
//             if (k < subset_dims.size() - 1) {
//                 current_indices[k + 1] += subset_dims[k + 1].stride();
//             } else {
//                 // something wrong !
//                 throw std::runtime_error("unknown error encountered in subset function");
//             }
//         }
//         unsigned int input_id = get_input_offset(current_indices, factors);
//         result[output_id] = (input[input_id] * scale_factor) + offset;

//         current_indices[0] += subset_dims[0].stride();
//     }
//     return result;
// }

// template <typename T>
// void json_mapping::subset::apply_subset(TypedDataArray& array, SubsetInfo& subset_info, double scale_factor,
//                                         double offset)
// {
//     log(LogLevel::DEBUG, "Entering do_subset method");
//     size_t bytes_size = array.size() * array.element_size();
//     log(LogLevel::DEBUG, "data array bye size is " + std::to_string(bytes_size));
//     std::vector<T> data_in((T*)array->data, (T*)array->data + array->data_n);

//     // TODO: associate subset dimid properly
//     log(LogLevel::DEBUG, "creating subset info arrays");
//     auto subset_dims = subset_info_converter(subset_info, array);
//     log(LogLevel::DEBUG, "carrying out subset operation");
//     auto transformed_data = subset(data_in, subset_dims, scale_factor, offset);

//     log(LogLevel::DEBUG, "output size: " + std::to_string(transformed_data.size()));
//     log(LogLevel::DEBUG, "output value 1: " + std::to_string(transformed_data[0]));

//     free((void*)array->data);
//     array->data_n = transformed_data.size();
//     log(LogLevel::DEBUG, "new data length is: " + std::to_string(array->data_n));
//     array->data = (char*)malloc(array->data_n * sizeof(T));
//     std::copy((char*)transformed_data.data(), (char*)transformed_data.data() + array->data_n * sizeof(T), array->data);
// }

// void json_mapping::subset::apply_subsetting(TypedDataArray& array, SubsetInfo& subset_info, double scale_factor,
//                                             double offset)
// {
//     log(LogLevel::DEBUG, "Entering apply subsetting function");
//     if (array->rank() == 0) {
//         return;
//     }

//     using json_mapping::DataType;
//     switch (type_index_map(array.type_index())) {
//         case DataType::Short:
//             do_subset<short>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::Int:
//             do_subset<int>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::Long:
//             do_subset<long>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::Int64:
//             do_subset<int64_t>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::UShort:
//             do_subset<unsigned short>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::UInt:
//             do_subset<unsigned int>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::ULong:
//             do_subset<unsigned long>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::UInt64:
//             do_subset<uint64_t>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::Float:
//             do_subset<float>(array, subset_info, scale_factor, offset);
//             break;
//         case DataType::Double:
//             do_subset<double>(array, subset_info, scale_factor, offset);
//             break;
//         default:
//             throw std::runtime_error(std::string("uda type ") + std::to_string(array->data_type) +
//                                      " not implemented for json_imas_mapping cache");
//     }
// }
