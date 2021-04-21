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
  struct string_view_parser {
    string_view_parser(const std::string_view& data) : m_data(data), m_current_pos(m_data.begin()) {}
    const std::string_view& m_data;
    std::string_view::const_iterator m_current_pos;
    int64_t zig_zag() {
      int64_t val = 0;
      uint8_t current_byte;
      uint8_t off = 0;
      do {
        current_byte = *m_current_pos;
        ++m_current_pos;
        val |= static_cast<int64_t>(current_byte & 0x7f) << off;
        off += 7;
      } while (current_byte & 0x80);
      if (val % 2) {
        return -(val + 1) / 2;
      } else {
        return val / 2;
      }
    }
    std::string_view string() {
      uint64_t len = zig_zag();
      auto start = m_current_pos;
      m_current_pos += len;
      return std::string_view(start, m_current_pos);
    }
  };

  template <typename self_type> struct serialize_value {
    static nlohmann::json schema() {
      nlohmann::json fields = nlohmann::json::array();
      serial<self_type>::iterate(static_cast<self_type*>(nullptr), [&]<typename current_field>(self_type*, current_field*) {
        fields.push_back(nlohmann::json{{"name", current_field::name()}, {"type", serialize_value<typename current_field::value_type>::schema()}});
      });
      return nlohmann::json{{"type", "record"}, {"fields", fields}};
    }
    static void binary(std::stringstream& ss, const self_type& value) {
      serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
        serialize_value<typename current_field::value_type>::binary(ss, value.*current_field::member_pointer());
      });
    }
    static void unbinary(string_view_parser& svp, self_type& value) {
      serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
        serialize_value<typename current_field::value_type>::unbinary(svp, value.*current_field::member_pointer());
      });
    }
  };

  template <> struct serialize_value<int> {
    static nlohmann::json schema() { return nlohmann::json("int"); }
    static void binary(std::stringstream& ss, const int& value) { zig_zag(ss, value); }
    static void unbinary(string_view_parser& svp, int& value) { value = svp.zig_zag(); }
  };

  template <typename subtype> struct serialize_value<std::vector<subtype>> {
    static nlohmann::json schema() { return nlohmann::json{{"type", "array"}, {"items", serialize_value<subtype>::schema()}}; }
    static void binary(std::stringstream& ss, const std::vector<subtype>& value) {
      zig_zag(ss, value.size());
      for (auto& element : value)
        serialize_value<subtype>::binary(ss, element);
      if (value.size())
        zig_zag(ss, 0);
    }
    static void unbinary(string_view_parser& svp, std::vector<subtype>& value) {
      value.clear();
      int64_t current_length;
      while ((current_length = svp.zig_zag()) != 0) {
        if (current_length < 0) {
          svp.zig_zag();
          current_length = -current_length;
        }
        value.resize(value.size() + current_length);
        for (uint64_t i = 0; i < current_length; i++) {
          serialize_value<subtype>::unbinary(svp, value.at(i));
        }
      }
    }
  };

  template <typename subtype> struct serialize_value<std::unordered_map<std::string, subtype>> {
    static nlohmann::json schema() { return nlohmann::json{{"type", "map"}, {"values", serialize_value<subtype>::schema()}}; }
    static void binary(std::stringstream& ss, const std::unordered_map<std::string, subtype>& value) {
      zig_zag(ss, value.size());
      for (const auto& [key, value] : value) {
        avro_string(ss, key.data());
        serialize_value<std::remove_cv_t<std::remove_reference_t<decltype(value)>>>::binary(ss, value);
      }
      if (value.size())
        zig_zag(ss, 0);
    }
    static void unbinary(string_view_parser& svp, std::unordered_map<std::string, subtype>& value) {
      value.clear();
      int64_t current_length;
      while ((current_length = svp.zig_zag()) != 0) {
        if (current_length < 0) {
          svp.zig_zag();
          current_length = -current_length;
        }
        for (uint64_t i = 0; i < current_length; i++) {
          serialize_value<subtype>::unbinary(svp, value[std::string(svp.string())]);
        }
      }
    }
  };

  template <typename subtype> struct serialize_value<std::optional<subtype>> {
    static nlohmann::json schema() { return nlohmann::json{"null", serialize_value<subtype>::schema()}; }
    static void binary(std::stringstream& ss, const std::optional<subtype>& value) {
      if (value) {
        zig_zag(ss, 1);
        serialize_value<subtype>::binary(ss, *value);
      } else
        zig_zag(ss, 0);
    }
    static void unbinary(string_view_parser& svp, std::optional<subtype>& value) {
      if (svp.zig_zag()) {
        value.emplace();
        serialize_value<subtype>::unbinary(svp, *value);
      } else
        value.reset();
    }
  };

  template <typename... subtype> struct serialize_value<std::variant<subtype...>> {
    static nlohmann::json schema() { return nlohmann::json{serialize_value<subtype>::schema()...}; }
    static void binary(std::stringstream& ss, const std::variant<subtype...>& value) {
      zig_zag(ss, value.index());
      std::visit([&](auto&& arg) { serialize_value<std::remove_cv_t<std::remove_reference_t<decltype(arg)>>>::binary(ss, arg); }, value);
    }
    template <size_t target_value, typename current_type, typename... other> static void unbinary_value(size_t index, string_view_parser& svp, std::variant<subtype...>& value) {
      if (index == target_value) {
        value = std::variant<subtype...>(std::in_place_index<target_value>);
        serialize_value<current_type>::unbinary(svp, std::get<target_value>(value));
      }
      if constexpr (sizeof...(other) > 0) {
        unbinary_value<target_value + 1, other...>(index, svp, value);
      }
    }
    static void unbinary(string_view_parser& svp, std::variant<subtype...>& value) { unbinary_value<0, subtype...>(svp.zig_zag(), svp, value); }
  };

  template <typename self_type> void schema() { std::cout << serialize_value<self_type>::schema().dump(2) << std::endl; }
  template <typename self_type> std::string serialize(const self_type& value) {
    std::stringstream ss;
    serialize_value<self_type>::binary(ss, value);
    return ss.str();
  }
  template <typename self_type> void deserialize(self_type& value, const std::string_view& text) {
    string_view_parser p(text);
    serialize_value<self_type>::unbinary(p, value);
  }
} // namespace avro
