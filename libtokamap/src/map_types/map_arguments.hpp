#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace libtokamap
{

enum class SignalType : uint8_t { DEFAULT, DATA, TIME, ERROR, DIM, INVALID };

enum class DataType : uint8_t { Unknown, Short, Int, Long, Int64, UShort, UInt, ULong, UInt64, Float, Double };

inline DataType type_index_map(std::type_index type_index)
{
    if (type_index == std::type_index{typeid(short)}) {
        return DataType::Short;
    }
    if (type_index == std::type_index{typeid(int)}) {
        return DataType::Int;
    }
    if (type_index == std::type_index{typeid(long)}) {
        return DataType::Long;
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
            throw std::runtime_error{"dimension size too large"};
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
    explicit TypedDataArray(const std::vector<T>& array, std::vector<size_t> shape = {}, bool owning = true)
        : m_type_index{typeid(T)}, m_size{array.size()}, m_shape{std::move(shape)}, m_owning{owning}
    {
        if (m_owning) {
            m_buffer = new char[m_size * sizeof(T)];
            std::memcpy(m_buffer, reinterpret_cast<const char*>(array.data()), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<char*>(const_cast<T*>(array.data()));
        }
        if (m_shape.empty()) {
            m_shape.push_back(m_size);
        }
    }

    template <typename T>
    explicit TypedDataArray(T* array, size_t size, std::vector<size_t> shape, bool owning = false)
        : m_type_index{typeid(T)}, m_size{size}, m_shape{std::move(shape)}, m_owning{owning}
    {
        if (m_owning) {
            m_buffer = new char[m_size * sizeof(T)];
            std::memcpy(m_buffer, reinterpret_cast<const char*>(array), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<char*>(array);
        }
    }

    template <typename T, std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
    explicit TypedDataArray(const T value) : m_type_index{typeid(T)}, m_size{1}, m_owning{true}
    {
        m_buffer = new char[sizeof(T)];
        std::memcpy(m_buffer, reinterpret_cast<const char*>(&value), sizeof(T));
    }

    explicit TypedDataArray(const std::string& value)
        : m_type_index{typeid(const char)}, m_size{value.size() + 1}, m_shape{value.size() + 1}, m_owning{true}
    {
        m_buffer = new char[m_size * sizeof(char)];
        std::memcpy(m_buffer, value.data(), m_size);
    }

    ~TypedDataArray()
    {
        if (m_owning) {
            delete[] m_buffer;
        }
    }

    template <typename T> void apply(double scale_factor, double offset)
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw std::runtime_error{"invalid type given to apply"};
        }

        gsl::span<T> data{reinterpret_cast<T*>(m_buffer), m_size};
        for (T& element : data) {
            element *= scale_factor;
            element += offset;
        }
    }

    template <typename T> void slice(const std::vector<SubsetInfo>& subsets)
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw std::runtime_error{"invalid type given to slice"};
        }
        if (subsets.size() != m_shape.size()) {
            throw std::runtime_error{"invalid number of subsets given"};
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

        gsl::span<T> array{reinterpret_cast<T*>(m_buffer), m_size};

        auto* new_buffer = new char[sizeof(T) * new_size];
        gsl::span<T> new_array{reinterpret_cast<T*>(new_buffer), new_size};

        auto offsets = compute_offsets(m_shape, subsets);
        size_t n = 0;
        for (const auto offset : offsets) {
            new_array[n] = array[offset];
            ++n;
        }

        if (m_owning) {
            delete[] m_buffer;
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

    template <typename T> [[nodiscard]] gsl::span<T> span() const
    {
        if (m_type_index != std::type_index{typeid(T)}) {
            throw std::runtime_error{"invalid type given to span"};
        }
        return gsl::span<T>{reinterpret_cast<T*>(m_buffer), m_size};
    }

    [[nodiscard]] size_t element_size()
    {
        switch (type_index_map(m_type_index)) {
            case DataType::Unknown:
                throw std::runtime_error{"unknown data type"};
            case DataType::Short:
                return sizeof(short);
            case DataType::Int:
                return sizeof(int);
            case DataType::Long:
                return sizeof(long);
            case DataType::Int64:
                return sizeof(int64_t);
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
    SignalType sig_type;
    std::type_index data_type;
    int rank;

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const SignalType sig_type, const std::type_index data_type,
                          const int rank)
        : entries{entries}, global_data{global_data}, sig_type{sig_type}, data_type{data_type}, rank{rank}
    {
    }
};

/**
 * @brief Deduce the type of signal being requested/mapped,
 * currently using string comparisons
 *
 * The final_path_element for error can be either be error_upper
 * or error_lower so search for substring error.
 *
 * @param element_back_str requested IDS path suffix (eg. data, time, error).
 * @note if no string is supplied, SignalType set to invalid
 * @return SignalType Enum class containing the current signal type
 * [DEFAULT, INVALID, DATA, TIME, ERROR]
 */
SignalType deduce_signal_type(std::string_view final_path_element);

} // namespace libtokamap
