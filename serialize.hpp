#pragma once

#include <algorithm>
#include <iostream>
#include <string_view>

namespace cserial {
  using namespace std::literals;
  template <typename T> struct member_pointer_class;
  template <typename Class, typename Value> struct member_pointer_class<Value Class::*> { typedef Class type; };
  template <typename T> struct member_pointer_value;
  template <typename Class, typename Value> struct member_pointer_value<Value Class::*> { typedef Value type; };
  template <size_t N> struct string_literal {
    constexpr string_literal(const char (&str)[N]) { std::copy_n(str, N, value); }
    char value[N];
    std::string_view operator()() const {
      if constexpr (N > 0)
        return std::string_view(value, N - 1);
      else
        return std::string_view();
    }
  };
  template <typename base_class> struct serial;

  template <auto member_pointer_t, string_literal name_t> struct serializable_field {
    using value_type = typename member_pointer_value<decltype(member_pointer_t)>::type;
    static auto member_pointer() { return member_pointer_t; }
    static std::string_view name() { return name_t(); }
  };

  template <typename self_type, typename executor_type, typename current_field, typename... other> struct field_iterator {
    static void execute(self_type* s, executor_type executor) {
      executor(s, static_cast<current_field*>(nullptr));
      if constexpr (sizeof...(other) > 0) {
        field_iterator<self_type, executor_type, other...>::execute(s, executor);
      }
    }
  };

  template <string_literal name_t, typename... args> struct serializer {
    template <typename self_type, typename executor_type> static void iterate(self_type* s, executor_type executor) {
      field_iterator<self_type, executor_type, args...>::execute(s, executor);
    }
    static std::string_view name() { return name_t(); }
  };
} // namespace cserial
