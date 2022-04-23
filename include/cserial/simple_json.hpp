#pragma once
#include <concepts>

#include "serialize.hpp"
#include "utils.hpp"
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cserial {
  struct simple_json {
    struct all {};
  };
} // namespace cserial

namespace nlohmann {
  template <typename self_type> struct adl_serializer<self_type, std::enable_if_t<std::is_base_of_v<cserial::simple_json, self_type>>> {
    static_assert(!std::is_base_of_v<cserial::available_converter, cserial::serial<self_type>>);
    static void to_json(json& j, const self_type& value) {
      j = json::object();
      cserial::serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
        if constexpr (!current_field::template has_parameter<cserial::simple_json::all>())
          j[std::string(current_field::name())] = json(value.*current_field::member_pointer());
      });
    }

    static void from_json(const json& j, self_type& value) {
      cserial::serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
        try {
          if constexpr (!current_field::template has_parameter<cserial::simple_json::all>()) {
            if (j.contains(std::string(current_field::name())))
              value.*current_field::member_pointer() = j[std::string(current_field::name())].get<typename current_field::value_type>();
            else
              value.*current_field::member_pointer() = nlohmann::json().get<typename current_field::value_type>();
          } else {
            value.*current_field::member_pointer() = j.get<typename current_field::value_type>();
          }
        } catch (std::exception& e) {
          throw std::runtime_error(std::string(fmt::format("error parsing field \"{}\": {}", current_field::name(), e.what())));
        }
      });
    }
  };
  template <typename T> struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
      if (opt) {
        j = *opt;
      } else {
        j = nullptr;
      }
    }

    static void from_json(const json& j, std::optional<T>& opt) {
      if (j.is_null()) {
        opt.reset();
      } else {
        opt = j.get<T>();
      }
    }
  };
  template <> struct adl_serializer<nlohmann::json> {
    static void to_json(json& j, const json& opt) { j = opt; }
    static void from_json(const json& j, json& opt) { opt = j; }
  };
} // namespace nlohmann
