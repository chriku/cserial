#pragma once

#include "serialize.hpp"
#include "utils.hpp"
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cserial {
  namespace avro {
    template <typename self_type> struct serialize_value;
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template <typename self_type> struct serialize_value_norm : serialize_value<std::remove_cv_t<std::remove_reference_t<self_type>>> {};
    template <typename self_type_raw> struct serialize_value {
      using self_type = typename std::remove_cv_t<std::remove_reference_t<self_type_raw>>;
      static_assert(is_defined<serial<self_type>>, "Missing serializer info");
      static inline constexpr std::string_view name() { return serial<self_type>::name(); }
      static inline nlohmann::json schema() {
        nlohmann::json fields = nlohmann::json::array();
        serial<self_type>::iterate(
            static_cast<self_type*>(nullptr), [&]<typename current_field>(self_type*, current_field*) constexpr {
              fields.push_back(nlohmann::json{{"name", current_field::name()}, {"type", serialize_value_norm<typename current_field::value_type>::schema()}});
            });
        return nlohmann::json{{"type", "record"}, {"name", name()}, {"fields", fields}};
      }
      static inline constexpr void binary(auto& ss, const self_type& value) {
        serial<self_type>::iterate(
            const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) constexpr {
              serialize_value_norm<typename current_field::value_type>::binary(ss, value.*current_field::member_pointer());
            });
      }
      static inline constexpr void unbinary(auto& svp, self_type& value) {
        serial<self_type>::iterate(
            const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) constexpr {
              serialize_value_norm<typename current_field::value_type>::unbinary(svp, value.*current_field::member_pointer());
            });
      }
      static inline nlohmann::json json(const self_type& value) {
        nlohmann::json ret;
        serial<self_type>::iterate(
            const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) constexpr {
              ret[std::string(current_field::name())] = serialize_value_norm<typename current_field::value_type>::json(value.*current_field::member_pointer());
            });
        return ret;
      }
      static inline void unjson(nlohmann::json object, self_type& value) {}
    };
    template <> struct serialize_value<bool> {
      static inline constexpr std::string_view name() { return "boolean"sv; }
      static inline nlohmann::json schema() { return name(); }
      static inline constexpr void binary(auto& ss, const bool& value) {
        if (value)
          ss("\1"sv);
        else
          ss("\0"sv);
      }
      static inline constexpr void unbinary(auto& svp, bool& value) { value = svp.fixed(1).at(0) != 0; }
      static inline nlohmann::json json(const bool& value) { return nlohmann::json(value); }
      static inline void unjson(nlohmann::json object, bool& value) { value = object.get<bool>(); }
    };
    template <typename integer_type> struct serialize_value_int {
      static inline constexpr std::string_view name() { return "long"sv; }
      static inline nlohmann::json schema() { return name(); }
      static inline constexpr void binary(auto& ss, const integer_type& value) { zig_zag(ss, value); }
      static inline constexpr void unbinary(auto& svp, integer_type& value) { value = svp.zig_zag(); }
      static inline nlohmann::json json(const integer_type& value) { return nlohmann::json(value); }
      static inline void unjson(nlohmann::json object, integer_type& value) { value = object.get<integer_type>(); }
    };
    template <> struct serialize_value<int64_t> : serialize_value_int<int64_t> {};
    template <> struct serialize_value<uint64_t> : serialize_value_int<uint64_t> {};
    template <> struct serialize_value<int32_t> : serialize_value_int<int32_t> {};
    template <> struct serialize_value<uint32_t> : serialize_value_int<uint32_t> {};
    template <> struct serialize_value<int16_t> : serialize_value_int<int16_t> {};
    template <> struct serialize_value<uint16_t> : serialize_value_int<uint16_t> {};
    template <> struct serialize_value<int8_t> : serialize_value_int<int8_t> {};
    template <> struct serialize_value<uint8_t> : serialize_value_int<uint8_t> {};
    template <typename base_type> struct serialize_value_float {
      static inline constexpr void binary(auto& ss, const base_type& value) { ss(std::string_view(reinterpret_cast<const char*>(&value), sizeof(base_type))); }
      static inline constexpr void unbinary(auto& svp, base_type& value) { value = *reinterpret_cast<const base_type*>(svp.fixed(sizeof(base_type)).data()); }
      static inline nlohmann::json json(const base_type& value) { return nlohmann::json(value); }
      static inline void unjson(nlohmann::json object, base_type& value) { value = object.get<base_type>(); }
    };
    template <> struct serialize_value<double> : serialize_value_float<double> {
      static_assert(sizeof(double) == 8);
      static inline constexpr std::string_view name() { return "double"sv; }
      static inline nlohmann::json schema() { return nlohmann::json(name()); }
    };
    template <> struct serialize_value<float> : serialize_value_float<float> {
      static_assert(sizeof(float) == 4);
      static inline constexpr std::string_view name() { return "float"sv; }
      static inline nlohmann::json schema() { return nlohmann::json(name()); }
    };
    template <> struct serialize_value<std::string> {
      static inline constexpr std::string_view name() { return "string"sv; }
      static inline nlohmann::json schema() { return nlohmann::json(name()); }
      static inline constexpr void binary(auto& ss, const std::string& value) { avro_string(ss, value); }
      static inline constexpr void unbinary(auto& svp, std::string& value) { value = svp.string(); }
      static inline nlohmann::json json(const std::string& value) { return nlohmann::json(value); }
      static inline void unjson(nlohmann::json object, std::string& value) { value = object.get<std::string>(); }
    };
    template <typename subtype> struct serialize_value<std::vector<subtype>> {
      static inline constexpr std::string_view name() { return "array"sv; }
      static inline nlohmann::json schema() { return nlohmann::json{{"type", "array"}, {"items", serialize_value_norm<subtype>::schema()}}; }
      static inline constexpr void binary(auto& ss, const std::vector<subtype>& value) {
        zig_zag(ss, value.size());
        for (const auto& element : value)
          serialize_value_norm<subtype>::binary(ss, element);
        if (value.size())
          zig_zag(ss, 0);
      }
      static inline constexpr void unbinary(auto& svp, std::vector<subtype>& value) {
        value.clear();
        int64_t current_length;
        while ((current_length = svp.zig_zag()) != 0) {
          if (current_length < 0) {
            svp.zig_zag();
            current_length = -current_length;
          }
          value.resize(value.size() + current_length);
          for (uint64_t i = 0; i < current_length; i++) {
            serialize_value_norm<subtype>::unbinary(svp, value.at(i));
          }
        }
      }
      static inline nlohmann::json json(const std::vector<subtype>& value) {
        nlohmann::json ret = nlohmann::json::array();
        for (const auto& o : value)
          ret.push_back(serialize_value_norm<subtype>::json(o));
        return ret;
      }
      static inline void unjson(nlohmann::json object, std::vector<subtype>& value) {
        value.clear();
        value.reserve(object.size());
        size_t i = 0;
        for (nlohmann::json so : object) {
          serialize_value_norm<subtype>::unjson(so, value.at(i));
          i++;
        }
      }
    };

    template <typename subtype> struct serialize_value<std::unordered_map<std::string, subtype>> {
      static inline constexpr std::string_view name() { return "map"sv; }
      static inline nlohmann::json schema() { return nlohmann::json{{"type", "map"}, {"values", serialize_value_norm<subtype>::schema()}}; }
      static inline constexpr void binary(auto& ss, const std::unordered_map<std::string, subtype>& value) {
        zig_zag(ss, value.size());
        for (const auto& [key, value] : value) {
          avro_string(ss, key.data());
          serialize_value_norm<decltype(value)>::binary(ss, value);
        }
        if (value.size())
          zig_zag(ss, 0);
      }
      static inline constexpr void unbinary(auto& svp, std::unordered_map<std::string, subtype>& value) {
        value.clear();
        int64_t current_length;
        while ((current_length = svp.zig_zag()) != 0) {
          if (current_length < 0) {
            svp.zig_zag();
            current_length = -current_length;
          }
          for (uint64_t i = 0; i < current_length; i++) {
            serialize_value_norm<subtype>::unbinary(svp, value[std::string(svp.string())]);
          }
        }
      }
      static inline nlohmann::json json(const std::unordered_map<std::string, subtype>& value) {
        nlohmann::json ret = nlohmann::json::object();
        for (const auto& [key, value] : value) {
          ret[key] = serialize_value_norm<subtype>::json(value);
        }
        return ret;
      }
      static inline void unjson(nlohmann::json object, std::unordered_map<std::string, subtype>& value) {
        value.clear();
        for (auto [key, v] : object.items()) {
          serialize_value_norm<subtype>::unjson(v, value[key]);
        }
      }
    };

    template <typename subtype> struct serialize_value<std::optional<subtype>> {
      static inline constexpr std::string_view name() { return "union"sv; }
      static inline nlohmann::json schema() { return nlohmann::json{"null", serialize_value_norm<subtype>::schema()}; }
      static inline constexpr void binary(auto& ss, const std::optional<subtype>& value) {
        if (value) {
          zig_zag(ss, 1);
          serialize_value_norm<subtype>::binary(ss, *value);
        } else
          zig_zag(ss, 0);
      }
      static inline constexpr void unbinary(auto& svp, std::optional<subtype>& value) {
        if (svp.zig_zag()) {
          value.emplace();
          serialize_value_norm<subtype>::unbinary(svp, *value);
        } else
          value.reset();
      }
      static inline nlohmann::json json(const std::optional<subtype>& value) {
        if (value)
          return nlohmann::json{{serialize_value_norm<subtype>::name(), serialize_value_norm<subtype>::json(*value)}};
        else
          return nlohmann::json();
      }
      static inline void unjson(nlohmann::json object, std::optional<subtype>& value) {
        if (object.is_object()) {
          value.emplace();
          serialize_value_norm<subtype>::unjson(object[serialize_value_norm<subtype>::name()], *value);
        } else
          value.reset();
      }
    };

    template <typename... subtype> struct serialize_value<std::variant<subtype...>> {
      static inline constexpr std::string_view name() { return "union"sv; }
      static inline nlohmann::json schema() { return nlohmann::json{serialize_value_norm<subtype>::schema()...}; }
      static inline constexpr void binary(auto& ss, const std::variant<subtype...>& value) {
        zig_zag(ss, value.index());
        std::visit(
            [&](auto&& arg) constexpr { serialize_value_norm<decltype(arg)>::binary(ss, arg); }, value);
      }
      template <size_t target_value, typename current_type, typename... other>
      static inline constexpr void unbinary_value(size_t index, auto& svp, std::variant<subtype...>& value) {
        if (index == target_value) {
          value = std::variant<subtype...>(std::in_place_index<target_value>);
          serialize_value_norm<current_type>::unbinary(svp, std::get<target_value>(value));
        }
        if constexpr (sizeof...(other) > 0) {
          unbinary_value<target_value + 1, other...>(index, svp, value);
        }
      }
      static inline constexpr void unbinary(auto& svp, std::variant<subtype...>& value) { unbinary_value<0, subtype...>(svp.zig_zag(), svp, value); }
      static inline nlohmann::json json(const std::variant<subtype...>& value) {
        nlohmann::json retval("null");
        std::visit(
            [&](auto&& arg) constexpr {
              retval = nlohmann::json{{serialize_value_norm<decltype(arg)>::name(), serialize_value_norm<decltype(arg)>::json(arg)}};
            },
            value);
        return retval;
      }
      static inline void unjson(nlohmann::json object, std::variant<subtype...>& value) {}
    };

    template <size_t len> struct serialize_value<std::array<char, len>> {
      static inline constexpr std::string_view name() { return numeric_string<len>::get(); }
      static inline nlohmann::json schema() { return nlohmann::json{{"type", "fixed"}, {"size", len}, {"name", name()}}; }
      static inline constexpr void binary(auto& ss, const std::array<char, len>& value) { ss(std::string_view(value.begin(), len)); }
      static inline constexpr void unbinary(auto& svp, std::array<char, len>& value) {
        auto data = svp.fixed(len);
        std::copy(data.begin(), data.end(), value.begin());
      }
      static inline nlohmann::json json(const std::array<char, len>& value) { return nlohmann::json(std::string_view(value.begin(), len)); }
      static inline void unjson(nlohmann::json object, std::array<char, len>& value) {
        std::string v = object.get<std::string>();
        std::copy(v.begin(), v.end(), value.begin());
      }
    };
#endif
    /**
     * \brief Generate the schema for the given data type
     */
    template <typename self_type> inline nlohmann::json schema() { return serialize_value_norm<self_type>::schema(); }
    /**
     * \brief Serialize a value into a std::string
     */
    template <typename self_type> inline std::string serialize(const self_type& value) {
      std::stringstream ss;
      auto f = [&ss](std::string_view v) constexpr { ss << v; };
      serialize_value_norm<self_type>::binary(f, value);
      return ss.str();
    }
    /**
     * \brief Serialize a value into a writer
     */
    template <typename self_type> inline constexpr void serialize(const self_type& value, auto& o) { serialize_value_norm<self_type>::binary(o, value); }
    /**
     * \brief Deserialize a std::string_view
     */
    template <typename self_type> inline void deserialize(self_type& value, const std::string_view& text) {
      string_view_parser p(text);
      serialize_value_norm<self_type>::unbinary(p, value);
    }
    /**
     * \brief Deserialize a string_view_parser compatibel value
     */
    template <typename self_type> inline constexpr void deserialize(self_type& value, auto& p) { serialize_value_norm<self_type>::unbinary(p, value); }
    /**
     * \brief Serialize into an avro json object
     */
    template <typename self_type> inline nlohmann::json json(const self_type& value) { return serialize_value_norm<self_type>::json(value); }
    /**
     * \brief Deserialize from an avro json object
     */
    template <typename self_type> inline void dejson(self_type& value, nlohmann::json text) { serialize_value_norm<self_type>::unjson(text, value); }
  } // namespace avro
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  namespace avro {
    struct schema_file {
      std::array<char, 4> magic;
      std::unordered_map<std::string, std::string> meta;
      std::array<char, 16> sync;
      long count;
      long size;
    };
  } // namespace avro
  template <>
  struct serial<avro::schema_file> : serializer<"ContainerObject", field<&avro::schema_file::magic, "magic">, field<&avro::schema_file::meta, "meta">,
                                                field<&avro::schema_file::sync, "sync">, field<&avro::schema_file::count, "count">, field<&avro::schema_file::size, "size">> {};
  namespace avro {
    template <typename self_type> std::string serialize_object_container(const self_type& a) {
      std::string file_data = serialize(a);
      schema_file sf;
      sf.magic[0] = 'O';
      sf.magic[1] = 'b';
      sf.magic[2] = 'j';
      sf.magic[3] = 1;
      sf.meta["avro.schema"] = schema<self_type>().dump();
      sf.sync.fill('0');
      sf.count = 1;
      sf.size = file_data.size();
      return avro::serialize(sf) + file_data + std::string(16, '0');
    }
  } // namespace avro
#endif
} // namespace cserial
