#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#pragma once

namespace cserial {
  using namespace std::literals;
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  template <int val> class numeric_string {
  private:
    constexpr static int abs_val(int x) { return x < 0 ? -x : x; }
    constexpr static int num_digits(int x) { return x < 0 ? 1 + num_digits(-x) : x < 10 ? 1 : 1 + num_digits(x / 10); }
    template <char... args> struct metastring {
      const char data[sizeof...(args) + 1] = {'f', args...};
    };
    template <int size, int x, char... args> struct numeric_builder { typedef typename numeric_builder<size - 1, x / 10, '0' + abs_val(x) % 10, args...>::type type; };
    template <int x, char... args> struct numeric_builder<2, x, args...> { typedef metastring < x<0 ? '-' : '0' + x / 10, '0' + abs_val(x) % 10, args...> type; };
    template <int x, char... args> struct numeric_builder<1, x, args...> { typedef metastring<'0' + x, args...> type; };
    typedef typename numeric_builder<num_digits(val), val>::type type;
    static constexpr type value{};

  public:
    static constexpr inline std::string_view get() { return std::string_view(value.data, num_digits(val) + 1); }
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
#endif
  /**
   * \brief Default deserializer reading a std::string_view
   */
  class string_view_parser {
    const std::string_view& m_data;
    std::string_view::const_iterator m_current_pos;

  public:
    /**
     * \brief Construct a string_view_parser from a std::string_view
     * \param[in] data The string_view to operate on. Has to remain valid during the whole existence of the parser.
     */
    string_view_parser(const std::string_view& data) : m_data(data), m_current_pos(m_data.begin()) {}
    /**
     * \brief Read an unsigned integer from the stream
     * \return integer, normalized into 64 bit signed
     */
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
    /**
     * \brief Read a fixed sized chunk from the stream.
     * \param[in] len Length of the chunk in bytes
     * \return part of the string_view. Is only valid as long as the underlaying string_view has valid data
     */
    inline std::string_view fixed(const size_t len) {
      auto start = m_current_pos;
      m_current_pos += len;
      return std::string_view(start, m_current_pos - start);
    }
    /**
     * \brief Read a variable length string from the stream
     * \return part of the string_view. Is only valid as long as the underlaying string_view has valid data
     */
    inline std::string_view string() { return fixed(zig_zag()); }
  };
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#if defined(__clang__)
#if __has_feature(cxx_rtti)
#define RTTI_ENABLED
#endif
#elif defined(__GNUG__)
#if defined(__GXX_RTTI)
#define RTTI_ENABLED
#endif
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define RTTI_ENABLED
#endif
#endif
#if defined(RTTI_ENABLED)
  template <typename T, typename = void> constexpr bool is_defined = false;
  template <typename T> constexpr bool is_defined<T, decltype(typeid(T), void())> = true;
#else
  template <typename T> constexpr bool is_defined = true;
#endif
#endif
} // namespace cserial
