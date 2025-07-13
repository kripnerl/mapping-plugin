#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <clientserver/udaTypes.h>

namespace json_mapping
{

enum class SignalType : uint8_t { DEFAULT, DATA, TIME, ERROR, DIM, INVALID };

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
            std::memcpy(const_cast<char*>(m_buffer), reinterpret_cast<const char*>(array.data()), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<const char*>(array.data());
        }
        if (m_shape.empty()) {
            m_shape.push_back(m_size);
        }
    }

    template <typename T>
    explicit TypedDataArray(const T* array, size_t size, std::vector<size_t> shape, bool owning = false)
        : m_type_index{typeid(T)}, m_size{size}, m_shape{std::move(shape)}, m_owning{owning}
    {
        if (m_owning) {
            m_buffer = new char[m_size * sizeof(T)];
            std::memcpy(const_cast<char*>(m_buffer), reinterpret_cast<const char*>(array), m_size * sizeof(T));
        } else {
            m_buffer = reinterpret_cast<const char*>(array);
        }
    }

    template <typename T> explicit TypedDataArray(const T value) : m_type_index{typeid(T)}, m_size{1}, m_owning{true}
    {
        m_buffer = new char[sizeof(T)];
        std::memcpy(const_cast<char*>(m_buffer), reinterpret_cast<const char*>(&value), sizeof(T));
    }

    explicit TypedDataArray(const std::string& value)
        : m_type_index{typeid(const char)}, m_size{value.size() + 1}, m_shape{value.size() + 1}, m_owning{true}
    {
        m_buffer = new char[m_size * sizeof(char)];
        std::memcpy(const_cast<char*>(m_buffer), value.data(), m_size);
    }

    ~TypedDataArray()
    {
        if (m_owning) {
            delete[] m_buffer;
        }
    }

    [[nodiscard]] bool empty() const { return m_size == 0; }

    [[nodiscard]] size_t size() const { return m_size; }

    [[nodiscard]] size_t rank() const { return m_shape.size(); }

    [[nodiscard]] std::type_index type_index() const { return m_type_index; }

    [[nodiscard]] const std::vector<size_t>& shape() const { return m_shape; }

    [[nodiscard]] const char* buffer() const { return m_buffer; }

    // Moveable but not copyable
    TypedDataArray(const TypedDataArray&) = delete;
    TypedDataArray(TypedDataArray&&) = default;
    TypedDataArray& operator=(const TypedDataArray&) = delete;
    TypedDataArray& operator=(TypedDataArray&&) = default;

  private:
    const char* m_buffer = nullptr;
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
    UDA_TYPE datatype;
    int rank;

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const SignalType sig_type, const UDA_TYPE datatype,
                          const int rank)
        : entries{entries}, global_data{global_data}, sig_type{sig_type}, datatype{datatype}, rank{rank}
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

} // namespace json_mapping
