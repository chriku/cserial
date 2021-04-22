#pragma once

#include "avro.hpp"
#include <bounded/integer.hpp>

namespace cserial {
  namespace nienary {
    template <auto min, auto max> struct serialize_value<bounded::integer<min, max>> {
      using int_type = bounded::integer<min, max>;
      static std::string_view name() { return "bounded"sv; }
      static nlohmann::json schema() { return nlohmann::json{{"type", name()}, {"min", min}, {"max", max}}; }
      static void binary(std::stringstream& ss, const bounded::integer<min, max>& value) {
        if constexpr (min >= 0)
          varint(ss, value.value() - min);
        else
          zig_zag(ss, value.value());
      }
      static void unbinary(string_view_parser& svp, bounded::integer<min, max>& value) {
        if constexpr (min >= 0)
          value = bounded::check_in_range<int_type>(bounded::integer(svp.varint() + min));
        else
          value = bounded::check_in_range<int_type>(bounded::integer(svp.zig_zag()));
      }
    };
  } // namespace nienary
} // namespace cserial
