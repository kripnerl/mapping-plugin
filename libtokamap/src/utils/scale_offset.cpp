#include "scale_offset.hpp"

#include <cstdlib>
#include <gsl/gsl-lite.hpp>
#include <stdexcept>

#include "map_types/map_arguments.hpp"

namespace
{

template <typename T> int offset_value(T& var, float offset)
{
    *var += offset;
    return 0;
}

template <typename T> int offset_span(gsl::span<T> span, float offset)
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

template <typename T> int scale_span(gsl::span<T> span, float scale)
{
    if (span.empty()) {
        return 1;
    }

    std::for_each(span.begin(), span.end(), [&](T& elem) { elem *= scale; });

    return 0;
}

} // namespace

int json_mapping::map_transform::transform_offset(TypedDataArray& array, float offset)
{
    int err{1};
    if (array.rank() > 0) {
        const size_t size = array.size();
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = reinterpret_cast<short*>(array.buffer());
                err = offset_span(gsl::span{data, size}, offset);
                break;
            }
            case DataType::Int: {
                auto* data = reinterpret_cast<int*>(array.buffer());
                err = offset_span(gsl::span{data, size}, offset);
                break;
            }
            case DataType::Long: {
                auto* data = reinterpret_cast<long*>(array.buffer());
                err = offset_span(gsl::span{data, size}, offset);
                break;
            }
            case DataType::Float: {
                auto* data = reinterpret_cast<float*>(array.buffer());
                err = offset_span(gsl::span{data, size}, offset);
                break;
            }
            case DataType::Double: {
                auto* data = reinterpret_cast<double*>(array.buffer());
                err = offset_span(gsl::span{data, size}, offset);
                break;
            }
            default:
                throw std::runtime_error{"unrecognised type"};
        }
    } else {
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = reinterpret_cast<short*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Int: {
                auto* data = reinterpret_cast<int*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Long: {
                auto* data = reinterpret_cast<long*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Float: {
                auto* data = reinterpret_cast<float*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            case DataType::Double: {
                auto* data = reinterpret_cast<double*>(array.buffer());
                err = offset_value(data, offset);
                break;
            }
            default:
                throw std::runtime_error{"unrecognised type"};
        }
    }

    return err;
}

int json_mapping::map_transform::transform_scale(TypedDataArray& array, float scale)
{
    int err{1};
    if (array.rank() > 0) {
        const size_t size = array.size();
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = reinterpret_cast<short*>(array.buffer());
                err = scale_span(gsl::span{data, size}, scale);
                break;
            }
            case DataType::Int: {
                auto* data = reinterpret_cast<int*>(array.buffer());
                err = scale_span(gsl::span{data, size}, scale);
                break;
            }
            case DataType::Long: {
                auto* data = reinterpret_cast<long*>(array.buffer());
                err = scale_span(gsl::span{data, size}, scale);
                break;
            }
            case DataType::Float: {
                auto* data = reinterpret_cast<float*>(array.buffer());
                err = scale_span(gsl::span{data, size}, scale);
                break;
            }
            case DataType::Double: {
                auto* data = reinterpret_cast<double*>(array.buffer());
                err = scale_span(gsl::span{data, size}, scale);
                break;
            }
            default:
                throw std::runtime_error{"unrecognised type"};
        }
    } else {
        switch (type_index_map(array.type_index())) {
            case DataType::Short: {
                auto* data = reinterpret_cast<short*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Int: {
                auto* data = reinterpret_cast<int*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Long: {
                auto* data = reinterpret_cast<long*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Float: {
                auto* data = reinterpret_cast<float*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            case DataType::Double: {
                auto* data = reinterpret_cast<double*>(array.buffer());
                err = scale_value(data, scale);
                break;
            }
            default:
                throw std::runtime_error{"unrecognised type"};
        }
    }

    return err;
}
