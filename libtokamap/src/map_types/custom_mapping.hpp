#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

namespace libtokamap {

enum class CustomMapType_t : uint8_t { MASTU_helloworld, DRAFT_helloworld, INVALID };

NLOHMANN_JSON_SERIALIZE_ENUM(CustomMapType_t, {{CustomMapType_t::INVALID, nullptr},
                                               {CustomMapType_t::MASTU_helloworld, "MASTU_helloworld"},
                                               {CustomMapType_t::DRAFT_helloworld, "DRAFT_helloworld"}})

/**
 * @class CustomMapping
 * @brief Mapping class CustomMapping to hold the CUSTOM MAP_TYPE from the JSON
 * mapping files. Custom functions are able to be executed using the
 * CustomMapType_t variable, any unrecognised string reverts to INVALID type and
 * returns 1
 *
 */
class CustomMapping : public Mapping
{
  public:
    CustomMapping() = delete;
    ~CustomMapping() override = default;
    explicit CustomMapping(CustomMapType_t custom_type) : m_custom_type(custom_type) {};
    CustomMapping(CustomMapping&& other) = default;
    CustomMapping(const CustomMapping& other) = default;
    CustomMapping& operator=(CustomMapping&& other) = default;
    CustomMapping& operator=(const CustomMapping& other) = default;

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    CustomMapType_t m_custom_type;

    static TypedDataArray MASTU_helloworld();
    static TypedDataArray DRAFT_helloworld();
};

} // namespace libtokamap
