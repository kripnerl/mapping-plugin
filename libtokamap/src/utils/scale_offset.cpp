#include "scale_offset.hpp"

#include <bit>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include "exceptions/exceptions.hpp"

#include "map_types/map_arguments.hpp"

namespace
{

template <typename T> int offset_value(T& var, float offset)
{
    *var += offset;
    return 0;
}

template <typename T> int offset_span(std::span<T> span, float offset)
{
    if (span.empty()) {
        return 1;
    }

    std::for_each(span.begin(), span.end(), [&](T& elem) { elem += offset; });

    return 0;
}

template <typename T> int scale_value(T& var, float scale)
{
    *var *= scale;
    return 0;
}

template <typename T> int scale_span(std::span<T> span, float scale)
{
    if (span.empty()) {
        return 1;
    }

    std::for_each(span.begin(), span.end(), [&](T& elem) { elem *= scale; });

    return 0;
}

} // namespace

int libtokamap::map_transform::transform_offset(TypedDataArray& array, float offset)
{
    int err{1};
    if (array.rank() > 0) {
        const size_t size = array.size();
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = std::bit_cast<short*>(array.buffer());
                err = offset_span(std::span{data, size}, offset);
                break;
            }
            case DataType::Int: {
                auto* data = std::bit_cast<int*>(array.buffer());
                err = offset_span(std::span{data, size}, offset);
                break;
            }
            case DataType::Long: {
                auto* data = std::bit_cast<long*>(array.buffer());
                err = offset_span(std::span{data, size}, offset);
                break;
            }
            case DataType::Float: {
                auto* data = std::bit_cast<float*>(array.buffer());
                err = offset_span(std::span{data, size}, offset);
                break;
            }
            case DataType::Double: {
                auto* data = std::bit_cast<double*>(array.buffer());
                err = offset_span(std::span{data, size}, offset);
                break;
            }
            default:
                throw libtokamap::DataTypeError{"unrecognised type"};
        }
    } else {
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = std::bit_cast<short*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Int: {
                auto* data = std::bit_cast<int*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Long: {
                auto* data = std::bit_cast<long*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Float: {
                auto* data = std::bit_cast<float*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Double: {
                auto* data = std::bit_cast<double*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            default:
                throw libtokamap::DataTypeError{"unrecognised type"};
        }
    }

    return err;
}

int libtokamap::map_transform::transform_scale(TypedDataArray& array, float scale)
{
    int err{1};
    if (array.rank() > 0) {
        const size_t size = array.size();
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = std::bit_cast<short*>(array.buffer());
                err = scale_span(std::span{data, size}, scale);
                break;
            }
            case DataType::Int: {
                auto* data = std::bit_cast<int*>(array.buffer());
                err = scale_span(std::span{data, size}, scale);
                break;
            }
            case DataType::Long: {
                auto* data = std::bit_cast<long*>(array.buffer());
                err = scale_span(std::span{data, size}, scale);
                break;
            }
            case DataType::Float: {
                auto* data = std::bit_cast<float*>(array.buffer());
                err = scale_span(std::span{data, size}, scale);
                break;
            }
            case DataType::Double: {
                auto* data = std::bit_cast<double*>(array.buffer());
                err = scale_span(std::span{data, size}, scale);
                break;
            }
            default:
                throw libtokamap::DataTypeError{"unrecognised type"};
        }
    } else {
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = std::bit_cast<short*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Int: {
                auto* data = std::bit_cast<int*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Long: {
                auto* data = std::bit_cast<long*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Float: {
                auto* data = std::bit_cast<float*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Double: {
                auto* data = std::bit_cast<double*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            default:
                throw libtokamap::DataTypeError{"unrecognised type"};
        }
    }

    return err;
}
