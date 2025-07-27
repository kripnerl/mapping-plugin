#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"

namespace libtokamap
{

enum class DataType : uint8_t {
    Unknown,
    Char,
    Short,
    Int,
    Long,
    Int64,
    UChar,
    UShort,
    UInt,
    ULong,
    UInt64,
    Float,
    Double
};

inline DataType type_index_map(std::type_index type_index)
{
    if (type_index == std::type_index{typeid(char)}) {
        return DataType::Char;
    }
    if (type_index == std::type_index{typeid(short)}) {
        return DataType::Short;
    }
    if (type_index == std::type_index{typeid(int)}) {
        return DataType::Int;
    }
    if (type_index == std::type_index{typeid(long)}) {
        return DataType::Long;
    }
    if (type_index == std::type_index{typeid(unsigned char)}) {
        return DataType::UChar;
    }
    if (type_index == std::type_index{typeid(unsigned short)}) {
        return DataType::UShort;
    }
    if (type_index == std::type_index{typeid(unsigned int)}) {
        return DataType::UInt;
    }
    if (type_index == std::type_index{typeid(unsigned long)}) {
        return DataType::ULong;
    }
    if (type_index == std::type_index{typeid(float)}) {
        return DataType::Float;
    }
    if (type_index == std::type_index{typeid(double)}) {
        return DataType::Double;
    }
    return DataType::Unknown;
}

class SubsetInfo
{
  public:
    SubsetInfo(int64_t start, int64_t stop, int64_t stride, size_t size)
        : m_start{start}, m_stop{stop}, m_stride{stride}, m_dim_size{static_cast<int64_t>(size)}
    {
        if (size > std::numeric_limits<int64_t>::max()) {
            throw libtokamap::ProcessingError{"dimension size too large"};
        }
        // negative indexes mean that many elements from the end
        if (start < 0) {
            m_start = m_dim_size + start;
        }
        if (stop < 0) {
            m_stop = m_dim_size + stop + 1;
        }
    }

    [[nodiscard]] bool empty() const { return m_start == m_stop; }

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
    int64_t m_dim_size;
};

std::vector<size_t> compute_offsets(const std::vector<size_t>& shape, const std::vector<SubsetInfo>& subsets);

class TypedDataArray
{
  public:
    TypedDataArray() : m_type_index{typeid(void)}, m_size{0}, m_owning{false} {}

    template <typename T>
    explicit TypedDataArray(const std::vector<T>& array, std::vector<size_t> shape = {})
        : m_type_index{typeid(T)}, m_size{array.size()}, m_shape{std::move(shape)}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(m_size * sizeof(T)));
        std::memcpy(m_buffer, reinterpret_cast<const char*>(array.data()), m_size * sizeof(T));
        if (m_shape.empty()) {
            m_shape.push_back(m_size);
        }
    }

    template <typename T>
    explicit TypedDataArray(T* array, size_t size, std::vector<size_t> shape, bool owning = true)
        : m_type_index{typeid(T)}, m_size{size}, m_shape{std::move(shape)}, m_owning{owning}
    {
        if (m_owning) {
            m_buffer = static_cast<char*>(malloc(m_size * sizeof(T)));
            std::memcpy(m_buffer, reinterpret_cast<const char*>(array), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<char*>(array);
        }
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    explicit TypedDataArray(const T value) : m_type_index{typeid(T)}, m_size{1}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(sizeof(T)));
        std::memcpy(m_buffer, reinterpret_cast<const char*>(&value), sizeof(T));
    }

    explicit TypedDataArray(const std::string& value)
        : m_type_index{typeid(char)}, m_size{value.size() + 1}, m_shape{value.size() + 1}, m_owning{true}
    {
        m_buffer = static_cast<char*>(malloc(m_size * sizeof(char)));
        std::memcpy(m_buffer, value.data(), m_size);
    }

    ~TypedDataArray()
    {
        if (m_owning) {
            free(m_buffer);
        }
    }

    template <typename T> void apply(double scale_factor, double offset)
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw libtokamap::DataTypeError{"invalid type given to apply"};
        }

        auto* data = reinterpret_cast<T*>(m_buffer);
        for (size_t idx = 0; idx < m_size; ++idx) {
            data[idx] *= scale_factor;
            data[idx] += offset;
        }
    }

    template <typename T> void slice(const std::vector<SubsetInfo>& subsets)
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw libtokamap::DataTypeError{"invalid type given to slice"};
        }
        if (subsets.size() != m_shape.size()) {
            throw libtokamap::ParameterError{"invalid number of subsets given"};
        }

        if (subsets.empty()) {
            return;
        }

        const size_t n_dims = m_shape.size();

        size_t new_size = 1;
        std::vector<size_t> new_shape;
        for (size_t dim = 0; dim < n_dims; ++dim) {
            auto len = subsets[dim].size();
            if (len > 1) {
                new_shape.push_back(len);
            }
            new_size *= len;
        }

        auto* array = reinterpret_cast<T*>(m_buffer);

        auto* new_buffer = static_cast<char*>(malloc(sizeof(T) * new_size));
        auto* new_array = reinterpret_cast<T*>(new_buffer);

        auto offsets = compute_offsets(m_shape, subsets);
        size_t idx = 0;
        for (const auto offset : offsets) {
            new_array[idx] = array[offset];
            ++idx;
        }

        if (m_owning) {
            free(m_buffer);
        }
        m_size = new_size;
        m_shape = new_shape;
        m_buffer = new_buffer;
        m_owning = true;
    }

    [[nodiscard]] bool empty() const { return m_size == 0; }

    [[nodiscard]] size_t size() const { return m_size; }

    [[nodiscard]] size_t rank() const { return m_shape.size(); }

    [[nodiscard]] std::type_index type_index() const { return m_type_index; }

    [[nodiscard]] const std::vector<size_t>& shape() const { return m_shape; }

    [[nodiscard]] char* buffer() const { return m_buffer; }

    [[nodiscard]] char* release()
    {
        char* ptr = m_buffer;
        m_buffer = nullptr;
        m_owning = false;
        return ptr;
    }

#if __cplusplus >= 202002L
    template <typename T> [[nodiscard]] std::span<T> span() const
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw libtokamap::DataTypeError{"invalid type given to span"};
        }
        return std::span<T>{reinterpret_cast<T*>(m_buffer), m_size};
    }
#endif

    template <typename T> [[nodiscard]] const T* data() const
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw libtokamap::DataTypeError{"invalid type given to data"};
        }
        return reinterpret_cast<T*>(m_buffer);
    }

    template <typename T> [[nodiscard]] std::vector<T> to_vector() const
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw libtokamap::DataTypeError{"invalid type given to to_vector"};
        }
        const T* ptr = reinterpret_cast<T*>(m_buffer);
        return std::vector<T>{ptr, ptr + m_size};
    }

    [[nodiscard]] size_t element_size()
    {
        switch (type_index_map(m_type_index)) {
            case DataType::Unknown:
                throw libtokamap::DataTypeError{"unknown data type"};
            case DataType::Char:
                return sizeof(char);
            case DataType::Short:
                return sizeof(short);
            case DataType::Int:
                return sizeof(int);
            case DataType::Long:
                return sizeof(long);
            case DataType::Int64:
                return sizeof(int64_t);
            case DataType::UChar:
                return sizeof(unsigned char);
            case DataType::UShort:
                return sizeof(unsigned short);
            case DataType::UInt:
                return sizeof(unsigned int);
            case DataType::ULong:
                return sizeof(unsigned long);
            case DataType::UInt64:
                return sizeof(uint64_t);
            case DataType::Float:
                return sizeof(float);
            case DataType::Double:
                return sizeof(double);
        }
    }

    constexpr static size_t default_max_elements = 10;
    constexpr static int default_precision = 3;

    [[nodiscard]] std::string to_string(size_t max_elements = default_max_elements,
                                        int precision = default_precision) const;

    // Moveable but not copyable
    TypedDataArray(const TypedDataArray&) = delete;
    TypedDataArray& operator=(const TypedDataArray&) = delete;

    TypedDataArray(TypedDataArray&& other) noexcept : TypedDataArray()
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_type_index, other.m_type_index);
        std::swap(m_size, other.m_size);
        std::swap(m_shape, other.m_shape);
        std::swap(m_owning, other.m_owning);
    };
    TypedDataArray& operator=(TypedDataArray&& other) noexcept
    {
        std::swap(m_buffer, other.m_buffer);
        std::swap(m_type_index, other.m_type_index);
        std::swap(m_size, other.m_size);
        std::swap(m_shape, other.m_shape);
        std::swap(m_owning, other.m_owning);
        return *this;
    };

  private:
    char* m_buffer = nullptr;
    std::type_index m_type_index;
    size_t m_size;
    std::vector<size_t> m_shape;
    bool m_owning;
};

class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries;
    const nlohmann::json& global_data;
    std::type_index data_type;
    int rank;

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const std::type_index data_type, const int rank)
        : entries{entries}, global_data{global_data}, data_type{data_type}, rank{rank}
    {
    }
};

} // namespace libtokamap
