#pragma once

#include "avro.hpp"
#include <bounded/integer.hpp>

namespace cserial {
  namespace avro {
    template <auto min, auto max> struct serialize_value<bounded::integer<min, max>> {
      using int_type = bounded::integer<min, max>;
      static std::string_view name() { return "long"sv; }
      static nlohmann::json schema() { return nlohmann::json(name()); }
      static void binary(std::stringstream& ss, const bounded::integer<min, max>& value) { zig_zag(ss, value.value()); }
      static void unbinary(string_view_parser& svp, bounded::integer<min, max>& value) { value = bounded::check_in_range<int_type>(svp.zig_zag()); }
      static nlohmann::json json(const bounded::integer<min, max>& value) { return nlohmann::json(value.value()); }
      static void unjson(nlohmann::json object, bounded::integer<min, max>& value) { value = bounded::check_in_range<int_type>(object.get<int64_t>()); }
    };
  } // namespace avro
} // namespace cserial
