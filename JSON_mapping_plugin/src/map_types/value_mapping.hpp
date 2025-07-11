#pragma once

#include <nlohmann/json.hpp>
#include <utility>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"

class ValueMapping : public Mapping
{
  public:
    ValueMapping() = delete;
    ~ValueMapping() override = default;
    ValueMapping(ValueMapping&&) = default;
    ValueMapping(const ValueMapping&) = default;
    ValueMapping& operator=(ValueMapping&&) = default;
    ValueMapping& operator=(const ValueMapping&) = default;

    explicit ValueMapping(nlohmann::json value) : m_value(std::move(value)) {};
    [[nodiscard]] int map(const MapArguments& arguments) const override;

  private:
    nlohmann::json m_value;
};
