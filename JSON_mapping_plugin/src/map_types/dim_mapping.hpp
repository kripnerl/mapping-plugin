#pragma once

#include <string>
#include <utility>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

namespace json_mapping {

class DimMapping : public Mapping
{
  public:
    DimMapping() = delete;
    explicit DimMapping(std::string dim_probe) : m_dim_probe{std::move(dim_probe)} {};

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_dim_probe;
};

} // namespace json_mapping
