#pragma once

#include "serialize.hpp"
#include "utils.hpp"
#include <charconv>
#include <nlohmann/json.hpp>

namespace cserial {
  namespace patch {
    struct patch {
      std::vector<std::variant<std::string, size_t>> key;
      size_t keypos = 0;
      nlohmann::json value;
    };
    template <typename self_type> struct patch_value;
    template <typename self_type> struct patch_value_struct {
      static void apply(self_type& value, nlohmann::json p) {
        serial<self_type>::iterate(
            &value, [&]<typename current_field>(self_type*, current_field*) constexpr {
              if (!p[std::string(current_field::name())].is_null()) {
                patch_value<typename current_field::value_type>::apply(value.*current_field::member_pointer(), p[std::string(current_field::name())]);
              }
            });
      }
    };
    template <typename self_type> struct patch_value_convert {
      using serial_type = typename serial<self_type>::serial_type;
      static void apply(self_type& value, nlohmann::json p) {
        serial_type v2;
        serial<self_type>::convert(value, v2);
        patch_value<serial_type>::apply(v2, p);
        serial<self_type>::unconvert(v2, value);
      }
    };
    template <typename self_type>
    struct patch_value : std::conditional_t<std::is_base_of_v<available_converter, serial<self_type>>, patch_value_convert<self_type>, patch_value_struct<self_type>> {};
    template <typename int_type> struct patch_value_simple {
      static void apply(int_type& value, nlohmann::json p) { value = p.get<int_type>(); }
    };
    template <> struct patch_value<uint64_t> : patch_value_simple<uint64_t> {};
    template <> struct patch_value<int64_t> : patch_value_simple<int64_t> {};
    template <> struct patch_value<uint32_t> : patch_value_simple<uint32_t> {};
    template <> struct patch_value<int32_t> : patch_value_simple<int32_t> {};
    template <> struct patch_value<uint16_t> : patch_value_simple<uint16_t> {};
    template <> struct patch_value<int16_t> : patch_value_simple<int16_t> {};
    template <> struct patch_value<uint8_t> : patch_value_simple<uint8_t> {};
    template <> struct patch_value<int8_t> : patch_value_simple<int8_t> {};
    template <> struct patch_value<float> : patch_value_simple<float> {};
    template <> struct patch_value<double> : patch_value_simple<double> {};
    template <> struct patch_value<bool> : patch_value_simple<bool> {};
    template <> struct patch_value<std::string> : patch_value_simple<std::string> {};
    template <> struct patch_value<std::monostate> {
      static void apply(std::monostate& value, nlohmann::json p) {}
    };
    template <typename... types> struct patch_value<std::variant<types...>> {
      template <size_t i, typename current_type, typename... other> static void apply_cond(std::variant<types...>& value, nlohmann::json p) {
        if (p.contains(std::to_string(i))) {
          value = std::variant<types...>(std::in_place_index<i>);
          patch_value<current_type>::apply(std::get<i>(value), p[std::to_string(i)]);
        } else if constexpr (sizeof...(other) > 0)
          apply_cond<i + 1, other...>(value, p);
      }
      static void apply(std::variant<types...>& value, nlohmann::json p) { apply_cond<0, types...>(value, p); }
    };
    template <typename subtype> struct patch_value<std::unordered_map<std::string, subtype>> {
      static void apply(std::unordered_map<std::string, subtype>& value, nlohmann::json p) {
        for (const auto& [key, val] : p.items()) {
          if (value.is_null()) {
            value.erase(key);
          } else {
            patch_value<subtype>::apply(value[key], val);
          }
        }
      }
    };
    template <typename subtype> struct patch_value<std::vector<subtype>> {
      static void apply(std::vector<subtype>& value, nlohmann::json p) {
        for (const auto& [key, val] : p.items()) {
          size_t name;
          if (auto [p, ec] = std::from_chars(key.data(), key.data() + key.size(), name); (ec == std::errc()) && (p[0] == '\0')) {
            if (value.is_null()) {
              value.resize(name);
            } else {
              if (value.size() <= name)
                value.resize(name + 1);
              patch_value<subtype>::apply(value.at(name), p);
            }
          }
        }
      }
    };
    template <typename self_type> void apply(self_type& o, nlohmann::json object) {
      if (object.is_object()) {
        patch_value<self_type>::apply(o, object);
      } else
        for (const auto& so : object)
          apply<self_type>(o, so);
    }
  } // namespace patch
} // namespace cserial
