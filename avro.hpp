#pragma once

#include "json.hpp"
#include "serialize.hpp"
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace avro {
  template <class... T> struct always_false : std::false_type {};
  void zig_zag(std::stringstream& ss, int64_t value) {
    uint64_t positive;
    if (value >= 0)
      positive = value * 2;
    else
      positive = (-value * 2) - 1;
    do {
      uint8_t b = positive % 0x80;
      positive >>= 7;
      if (positive > 0)
        b |= 0x80;
      ss << ((char)b);
    } while (positive > 0);
  }
  void avro_string(std::stringstream& ss, std::string value) {
    zig_zag(ss, value.size());
    ss << value;
  }

  template <typename self_type> struct schema_name {
    static nlohmann::json json() {
      nlohmann::json fields = nlohmann::json::array();
      decltype(static_cast<self_type*>(nullptr)->serializer_info)::iterate(static_cast<self_type*>(nullptr), [&]<typename current_field>(self_type*, current_field*) {
        fields.push_back(nlohmann::json{{"name", current_field::name()}, {"type", schema_name<typename current_field::value_type>::json()}});
      });
      return nlohmann::json{{"type", "record"}, {"fields", fields}};
    }
  };
  template <typename self_type> struct serialize_value {
    static void binary(std::stringstream& ss, const self_type& value) {
      decltype(const_cast<self_type*>(&value)->serializer_info)::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
        serialize_value<typename current_field::value_type>::binary(ss, value.*current_field::member_pointer());
      });
    }
  };

  template <> struct schema_name<int> {
    static nlohmann::json json() { return nlohmann::json("int"); }
  };
  template <> struct serialize_value<int> {
    static void binary(std::stringstream& ss, const int& value) { zig_zag(ss, value); }
  };

  template <typename subtype> struct schema_name<std::vector<subtype>> {
    static nlohmann::json json() { return nlohmann::json{{"type", "array"}, {"items", schema_name<subtype>::json()}}; }
  };
  template <typename subtype> struct serialize_value<std::vector<subtype>> {
    static void binary(std::stringstream& ss, const std::vector<subtype>& value) {
      zig_zag(ss, value.size());
      for (auto& element : value)
        serialize_value<subtype>::binary(ss, element);
      if (value.size())
        zig_zag(ss, 0);
    }
  };

  template <typename subtype> struct schema_name<std::unordered_map<std::string, subtype>> {
    static nlohmann::json json() { return nlohmann::json{{"type", "map"}, {"values", schema_name<subtype>::json()}}; }
  };
  template <typename subtype> struct serialize_value<std::unordered_map<std::string, subtype>> {
    static void binary(std::stringstream& ss, const std::unordered_map<std::string, subtype>& value) {
      zig_zag(ss, value.size());
      for (const auto& [key, value] : value) {
        avro_string(ss, key.data());
        serialize_value<std::remove_cv_t<std::remove_reference_t<decltype(value)>>>::binary(ss, value);
      }
      if (value.size())
        zig_zag(ss, 0);
    }
  };

  template <typename subtype> struct schema_name<std::optional<subtype>> {
    static nlohmann::json json() { return nlohmann::json{"null", schema_name<subtype>::json()}; }
  };
  template <typename subtype> struct serialize_value<std::optional<subtype>> {
    static void binary(std::stringstream& ss, const std::optional<subtype>& value) {
      if (value) {
        zig_zag(ss, 1);
        serialize_value<subtype>::binary(ss, *value);
      } else
        zig_zag(ss, 0);
    }
  };

  template <typename... subtype> struct schema_name<std::variant<subtype...>> {
    static nlohmann::json json() { return nlohmann::json{schema_name<subtype>::json()...}; }
  };
  template <typename... subtype> struct serialize_value<std::variant<subtype...>> {
    static void binary(std::stringstream& ss, const std::variant<subtype...>& value) {
      zig_zag(ss, value.index());
      std::visit([&](auto&& arg) { serialize_value<std::remove_cv_t<std::remove_reference_t<decltype(arg)>>>::binary(ss, arg); }, value);
    }
  };

  template <typename self_type> void schema() { std::cout << schema_name<self_type>::json().dump(2) << std::endl; }
  template <typename self_type> void serialize(const self_type& value) {
    std::stringstream ss;
    serialize_value<self_type>::binary(ss, value);
    for (uint8_t c : ss.str()) {
      std::cout << std::setfill('0') << std::setw(2) << std::hex << c + 0 << " ";
    }
    std::cout << std::endl;
  }
} // namespace avro
