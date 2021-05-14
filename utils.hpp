#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#pragma once

namespace cserial {
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
    static constexpr inline std::string_view get() { return std::string_view(value.data, num_digits(x) + 1); }
  };
  template <int x> constexpr typename numeric_string<x>::type numeric_string<x>::value;

  template <class... T> struct always_false : std::false_type {};
  inline void zig_zag(auto& ss, int64_t value) {
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
      char w = b;
      ss(std::string_view(&w, 1));
    } while (positive > 0);
  }
  inline void avro_string(auto& ss, std::string value) {
    zig_zag(ss, value.size());
    ss(value);
  }
  struct string_view_parser {
    string_view_parser(const std::string_view& data) : m_data(data), m_current_pos(m_data.begin()) {}
    const std::string_view& m_data;
    std::string_view::const_iterator m_current_pos;
    inline int64_t zig_zag() {
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
    inline std::string_view fixed(size_t len) {
      auto start = m_current_pos;
      m_current_pos += len;
      return std::string_view(start, m_current_pos - start);
    }
    inline std::string_view string() { return fixed(zig_zag()); }
  };
#if defined(RTTI_ENABLED)
  template <typename T, typename = void> constexpr bool is_defined = false;
  template <typename T> constexpr bool is_defined<T, decltype(typeid(T), void())> = true;
#else
  template <typename T> constexpr bool is_defined = true;
#endif
} // namespace cserial
