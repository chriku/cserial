#pragma once

#include "json.hpp"
#include "serialize.hpp"
#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cserial {
  namespace nienary {
    using namespace std::literals;
    constexpr int abs_val(int x) { return x < 0 ? -x : x; }
    constexpr int num_digits(int x) { return x < 0 ? 1 + num_digits(-x) : x < 10 ? 1 : 1 + num_digits(x / 10); }
    template <char... args> struct metastring {
      const char data[sizeof...(args) + 1] = {'f', args...};
    };
    template <int size, int x, char... args> struct numeric_builder { typedef typename numeric_builder<size - 1, x / 10, '0' + abs_val(x) % 10, args...>::type type; };
    template <int x, char... args> struct numeric_builder<2, x, args...> { typedef metastring < x<0 ? '-' : '0' + x / 10, '0' + abs_val(x) % 10, args...> type; };
    template <int x, char... args> struct numeric_builder<1, x, args...> { typedef metastring<'0' + x, args...> type; };
    template <int x> class numeric_string {
    private:
      typedef typename numeric_builder<num_digits(x), x>::type type;
      static constexpr type value{};

    public:
      static constexpr std::string_view get() { return std::string_view(value.data, num_digits(x) + 1); }
    };
    template <int x> constexpr typename numeric_string<x>::type numeric_string<x>::value;
    template <class... T> struct always_false : std::false_type {};

    void varint(std::stringstream& ss, uint64_t positive) {
      do {
        uint8_t b = positive % 0x80;
        positive >>= 7;
        if (positive > 0)
          b |= 0x80;
        ss << ((char)b);
      } while (positive > 0);
    }
    void zig_zag(std::stringstream& ss, int64_t value) {
      uint64_t positive;
      if (value >= 0)
        positive = value * 2;
      else
        positive = (-value * 2) - 1;
      varint(ss, positive);
    }
    void nienary_string(std::stringstream& ss, std::string value) {
      varint(ss, value.size());
      ss << value;
    }
    struct string_view_parser {
      string_view_parser(const std::string_view& data) : m_data(data), m_current_pos(m_data.begin()) {}
      const std::string_view& m_data;
      std::string_view::const_iterator m_current_pos;
      uint64_t varint() {
        uint64_t val = 0;
        uint8_t current_byte;
        uint8_t off = 0;
        do {
          current_byte = *m_current_pos;
          ++m_current_pos;
          val |= static_cast<uint64_t>(current_byte & 0x7f) << off;
          off += 7;
        } while (current_byte & 0x80);
        return val;
      }
      int64_t zig_zag() {
        int64_t val = varint();
        if (val % 2) {
          return -(val + 1) / 2;
        } else {
          return val / 2;
        }
      }
      std::string_view fixed(size_t len) {
        auto start = m_current_pos;
        m_current_pos += len;
        return std::string_view(start, m_current_pos - start);
      }
      std::string_view string() { return fixed(varint()); }
    };
    template <typename T, typename = void> constexpr bool is_defined = false;
    template <typename T> constexpr bool is_defined<T, decltype(typeid(T), void())> = true;
    template <typename self_type> struct serialize_value;
    template <typename self_type> struct serialize_value_norm : serialize_value<std::remove_cv_t<std::remove_reference_t<self_type>>> {};
    template <typename self_type_raw> struct serialize_value {
      using self_type = typename std::remove_cv_t<std::remove_reference_t<self_type_raw>>;
      static_assert(is_defined<serial<self_type>>, "Missing serializer info");
      static std::string_view name() { return serial<self_type>::name(); }
      static nlohmann::json schema() {
        nlohmann::json fields = nlohmann::json::array();
        serial<self_type>::iterate(static_cast<self_type*>(nullptr), [&]<typename current_field>(self_type*, current_field*) {
          fields.push_back(nlohmann::json{{"name", current_field::name()}, {"type", serialize_value_norm<typename current_field::value_type>::schema()}});
        });
        return nlohmann::json{{"type", "record"}, {"name", name()}, {"fields", fields}};
      }
      static void binary(std::stringstream& ss, const self_type& value) {
        serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
          serialize_value_norm<typename current_field::value_type>::binary(ss, value.*current_field::member_pointer());
        });
      }
      static void unbinary(string_view_parser& svp, self_type& value) {
        serial<self_type>::iterate(const_cast<self_type*>(&value), [&]<typename current_field>(self_type*, current_field*) {
          serialize_value_norm<typename current_field::value_type>::unbinary(svp, value.*current_field::member_pointer());
        });
      }
    };
    template <typename base_type> struct serialize_integer {
      static std::string_view name() {
        if constexpr (std::is_signed_v<base_type>)
          return "long"sv;
        else
          return "unsigned long"sv;
      }
      static nlohmann::json schema() { return nlohmann::json(name()); }
      static void binary(std::stringstream& ss, const base_type& value) {
        if constexpr (std::is_signed_v<base_type>)
          zig_zag(ss, value);
        else
          varint(ss, value);
      }
      static void unbinary(string_view_parser& svp, base_type& value) {
        if constexpr (std::is_signed_v<base_type>)
          value = svp.zig_zag();
        else
          value = svp.varint();
      }
    };
    template <> struct serialize_value<int64_t> : serialize_integer<int64_t> {};
    template <> struct serialize_value<uint64_t> : serialize_integer<uint64_t> {};
    template <> struct serialize_value<int32_t> : serialize_integer<int32_t> {};
    template <> struct serialize_value<uint32_t> : serialize_integer<uint32_t> {};
    template <> struct serialize_value<int16_t> : serialize_integer<int16_t> {};
    template <> struct serialize_value<uint16_t> : serialize_integer<uint16_t> {};
    template <> struct serialize_value<int8_t> : serialize_integer<int8_t> {};
    template <> struct serialize_value<uint8_t> : serialize_integer<uint8_t> {};
    template <> struct serialize_value<double> {
      static_assert(sizeof(double) == 8);
      static std::string_view name() { return "double"sv; }
      static nlohmann::json schema() { return nlohmann::json(name()); }
      static constexpr double multiplicator = (1LL << 52);
      static void binary(std::stringstream& ss, const double& value) {
        int exp;
        double fp = frexp(value, &exp);
        int64_t val = 0;
        if (fp > 0) {
          val = (fp - 0.5) * multiplicator;
          val++;
        } else if (fp < 0) {
          fp = -fp;
          val = (fp - 0.5) * multiplicator;
          val++;
          val = -val;
        }
        zig_zag(ss, val);
        zig_zag(ss, exp);
      }
      static void unbinary(string_view_parser& svp, double& value) {
        int64_t val = svp.zig_zag();
        int64_t exp = svp.zig_zag();
        double fp = 0;
        if (val >= 1) {
          val--;
          fp = double(val) / multiplicator;
          fp += 0.5;
          value = ldexp(fp, exp);
        } else if (val <= -1) {
          val = -val;
          val--;
          fp = double(val) / multiplicator;
          fp += 0.5;
          fp = -fp;
          value = ldexp(fp, exp);
        } else
          value = 0;
      }
    };
    template <> struct serialize_value<float> : serialize_value<double> {};
    template <> struct serialize_value<std::string> {
      static std::string_view name() { return "string"sv; }
      static nlohmann::json schema() { return nlohmann::json(name()); }
      static void binary(std::stringstream& ss, const std::string& value) { nienary_string(ss, value); }
      static void unbinary(string_view_parser& svp, std::string& value) { value = svp.string(); }
      static nlohmann::json json(const std::string& value) { return nlohmann::json(value); }
      static void unjson(nlohmann::json object, std::string& value) { value = object.get<std::string>(); }
    };

    template <typename subtype> struct serialize_value<std::vector<subtype>> {
      static std::string_view name() { return "array"sv; }
      static nlohmann::json schema() { return nlohmann::json{{"type", "array"}, {"items", serialize_value_norm<subtype>::schema()}}; }
      static void binary(std::stringstream& ss, const std::vector<subtype>& value) {
        varint(ss, value.size());
        for (const auto& element : value)
          serialize_value_norm<subtype>::binary(ss, element);
      }
      static void unbinary(string_view_parser& svp, std::vector<subtype>& value) {
        value.clear();
        uint64_t current_length = svp.varint();
        value.resize(current_length);
        for (uint64_t i = 0; i < current_length; i++) {
          serialize_value_norm<subtype>::unbinary(svp, value.at(i));
        }
      }
    }; // namespace nienary

    template <typename subtype> struct serialize_value<std::unordered_map<std::string, subtype>> {
      static std::string_view name() { return "map"sv; }
      static nlohmann::json schema() { return nlohmann::json{{"type", "map"}, {"values", serialize_value_norm<subtype>::schema()}}; }
      static void binary(std::stringstream& ss, const std::unordered_map<std::string, subtype>& value) {
        varint(ss, value.size());
        for (const auto& [key, value] : value) {
          nienary_string(ss, key.data());
          serialize_value_norm<decltype(value)>::binary(ss, value);
        }
      }
      static void unbinary(string_view_parser& svp, std::unordered_map<std::string, subtype>& value) {
        value.clear();
        int64_t current_length = svp.varint();
        for (uint64_t i = 0; i < current_length; i++) {
          serialize_value_norm<subtype>::unbinary(svp, value[std::string(svp.string())]);
        }
      }
    };

    template <typename subtype> struct serialize_value<std::optional<subtype>> {
      static std::string_view name() { return "union"sv; }
      static nlohmann::json schema() { return nlohmann::json{"null", serialize_value_norm<subtype>::schema()}; }
      static void binary(std::stringstream& ss, const std::optional<subtype>& value) {
        if (value) {
          varint(ss, 1);
          serialize_value_norm<subtype>::binary(ss, *value);
        } else
          varint(ss, 0);
      }
      static void unbinary(string_view_parser& svp, std::optional<subtype>& value) {
        if (svp.varint()) {
          value.emplace();
          serialize_value_norm<subtype>::unbinary(svp, *value);
        } else
          value.reset();
      }
    };

    template <typename... subtype> struct serialize_value<std::variant<subtype...>> {
      static std::string_view name() { return "union"sv; }
      static nlohmann::json schema() { return nlohmann::json{serialize_value_norm<subtype>::schema()...}; }
      static void binary(std::stringstream& ss, const std::variant<subtype...>& value) {
        varint(ss, value.index());
        std::visit([&](auto&& arg) { serialize_value_norm<decltype(arg)>::binary(ss, arg); }, value);
      }
      template <size_t target_value, typename current_type, typename... other> static void unbinary_value(size_t index, string_view_parser& svp, std::variant<subtype...>& value) {
        if (index == target_value) {
          value = std::variant<subtype...>(std::in_place_index<target_value>);
          serialize_value_norm<current_type>::unbinary(svp, std::get<target_value>(value));
        }
        if constexpr (sizeof...(other) > 0) {
          unbinary_value<target_value + 1, other...>(index, svp, value);
        }
      }
      static void unbinary(string_view_parser& svp, std::variant<subtype...>& value) { unbinary_value<0, subtype...>(svp.varint(), svp, value); }
    };

    template <size_t len> struct serialize_value<std::array<char, len>> {
      static std::string_view name() { return numeric_string<len>::get(); }
      static nlohmann::json schema() { return nlohmann::json{{"type", "fixed"}, {"size", len}, {"name", name()}}; }
      static void binary(std::stringstream& ss, const std::array<char, len>& value) { ss << std::string_view(value.begin(), len); }
      static void unbinary(string_view_parser& svp, std::array<char, len>& value) {
        auto data = svp.fixed(len);
        std::copy(data.begin(), data.end(), value.begin());
      }
    };

    template <typename self_type> nlohmann::json schema() { return serialize_value_norm<self_type>::schema(); }
    template <typename self_type> std::string serialize(const self_type& value) {
      std::stringstream ss;
      serialize_value_norm<self_type>::binary(ss, value);
      return ss.str();
    }
    template <typename self_type> void deserialize(self_type& value, const std::string_view& text) {
      string_view_parser p(text);
      serialize_value_norm<self_type>::unbinary(p, value);
    }
  } // namespace nienary
} // namespace cserial
