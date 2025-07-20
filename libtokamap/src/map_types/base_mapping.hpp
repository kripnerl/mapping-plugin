#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

#include "map_arguments.hpp"

namespace libtokamap
{

enum class MappingType : uint8_t { UNKNOWN, VALUE, DATA_SOURCE, SLICE, EXPR, CUSTOM, DIM };

NLOHMANN_JSON_SERIALIZE_ENUM(MappingType, {{MappingType::UNKNOWN, ""}, // will default to this on no match
                                           {MappingType::VALUE, "VALUE"},
                                           {MappingType::DATA_SOURCE, "DATA_SOURCE"},
                                           {MappingType::SLICE, "SLICE"},
                                           {MappingType::EXPR, "EXPR"},
                                           {MappingType::CUSTOM, "CUSTOM"},
                                           {MappingType::DIM, "DIMENSION"}})

class Mapping
{
  public:
    Mapping() = default;
    virtual ~Mapping() = default;
    Mapping(Mapping&& other) = default;
    Mapping(const Mapping& other) = default;
    Mapping& operator=(Mapping&& other) = default;
    Mapping& operator=(const Mapping& other) = default;

    [[nodiscard]] virtual TypedDataArray map(const MapArguments& arguments) const = 0;
};

} // namespace libtokamap
