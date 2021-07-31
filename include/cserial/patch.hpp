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
      static void apply(self_type& value, patch& p) {
        std::string name = std::get<std::string>(p.key.at(p.keypos));
        serial<self_type>::iterate(
            &value, [&]<typename current_field>(self_type*, current_field*) constexpr {
              std::cout << current_field::name() << ": " << name << std::endl;
              if (current_field::name() == name) {
                p.keypos++;
                patch_value<typename current_field::value_type>::apply(value.*current_field::member_pointer(), p);
              }
            });
      }
    };
    template <typename self_type> struct patch_value_convert {
      using serial_type = typename serial<self_type>::serial_type;
      static void apply(self_type& value, patch& p) {
        serial_type v2;
        serial<self_type>::convert(value, v2);
        patch_value<serial_type>::apply(v2, p);
        serial<self_type>::unconvert(v2, value);
      }
    };
    template <typename self_type>
    struct patch_value : std::conditional_t<std::is_base_of_v<available_converter, serial<self_type>>, patch_value_convert<self_type>, patch_value_struct<self_type>> {};
    template <typename int_type> struct patch_value_simple {
      static void apply(int_type& value, patch& p) { value = p.value.get<int_type>(); }
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
    template <> struct patch_value<std::monostate> {
      static void apply(std::monostate& value, patch& p) {}
    };
    template <typename... types> struct patch_value<std::variant<types...>> {
      template <size_t i, typename current_type, typename... other> static void apply_cond(std::variant<types...>& value, patch& p, size_t name) {
        if (i == name) {
          value = std::variant<types...>(std::in_place_index<i>);
          patch_value<current_type>::apply(std::get<i>(value), p);
        } else if constexpr (sizeof...(other) > 0)
          apply_cond<i + 1, other...>(value, p, name);
      }
      static void apply(std::variant<types...>& value, patch& p) {
        size_t name = std::get<size_t>(p.key.at(p.keypos));
        p.keypos++;
        apply_cond<0, types...>(value, p, name);
      }
    };
    std::vector<patch> ensure_split(patch in) {
      if (in.value.is_object()) {
        std::vector<patch> ret;
        for (const auto& [key, value] : in.value.items()) {
          patch np = in;
          size_t result;
          if (auto [p, ec] = std::from_chars(key.data(), key.data() + key.size(), result); (ec == std::errc()) && (p[0] == '\0')) {
            np.key.push_back(result);
          } else
            np.key.push_back(key);
          np.value = value;
          for (auto sp : ensure_split(np))
            ret.push_back(sp);
        }
        return ret;
      } else {
        return std::vector<patch>{in};
      }
    }
    template <typename self_type> void apply(self_type& o, nlohmann::json object) {
      patch root_patch;
      root_patch.value = object;
      auto p = ensure_split(root_patch);
      for (auto& p : p)
        patch_value<self_type>::apply(o, p);
    }
  } // namespace patch
} // namespace cserial
