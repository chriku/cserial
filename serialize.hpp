#pragma once

#include <algorithm>
#include <iostream>
#include <string_view>

namespace cserial {
  template <typename T> struct member_pointer_class;
  template <typename Class, typename Value> struct member_pointer_class<Value Class::*> { typedef Class type; };
  template <typename T> struct member_pointer_value;
  template <typename Class, typename Value> struct member_pointer_value<Value Class::*> { typedef Value type; };

  template <typename key_t, auto val_t> struct parameter {
    using key = key_t;
    constexpr static auto value = val_t;
  };
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

  template <auto member_pointer_t, string_literal name_t, typename... parameters_t> struct field {
    using value_type = typename member_pointer_value<decltype(member_pointer_t)>::type;
    constexpr static inline auto member_pointer() { return member_pointer_t; }
    constexpr static inline std::string_view name() { return name_t(); }
    template <typename chosen_t, typename current_parameter, typename... other_parameters> constexpr static inline auto access_field(auto default_value) {
      if constexpr (std::is_same_v<chosen_t, typename current_parameter::key>) {
        return current_parameter::value;
      } else if constexpr (sizeof...(other_parameters) > 0) {
        return access_field<chosen_t, other_parameters...>(default_value);
      } else
        return default_value;
    }
    template <typename chosen_t, typename argument_type> constexpr static inline argument_type parameter(argument_type default_value = argument_type()) {
      if constexpr (sizeof...(parameters_t) > 0) {
        return access_field<chosen_t, parameters_t...>(default_value);
      }
      return default_value;
    }
    template <typename chosen_t, typename current_parameter, typename... other_parameters> constexpr static inline bool has_access_field() {
      if constexpr (std::is_same_v<chosen_t, typename current_parameter::key>) {
        return true;
      } else if constexpr (sizeof...(other_parameters) > 0) {
        return has_access_field<chosen_t, other_parameters...>();
      } else
        return false;
    }
    template <typename chosen_t> constexpr static inline auto has_parameter() {
      if constexpr (sizeof...(parameters_t) > 0) {
        return has_access_field<chosen_t, parameters_t...>();
      }
      return false;
    }
  };

  template <typename self_type, typename executor_type, typename current_field, typename... other> struct field_iterator {
    static inline void execute(self_type* s, executor_type executor) {
      executor(s, static_cast<current_field*>(nullptr));
      if constexpr (sizeof...(other) > 0) {
        field_iterator<self_type, executor_type, other...>::execute(s, executor);
      }
    }
  };

  template <string_literal name_t, typename... args> struct serializer {
    template <typename self_type, typename executor_type> static inline void iterate(self_type* s, executor_type executor) {
      field_iterator<self_type, executor_type, args...>::execute(s, executor);
    }
    static inline std::string_view name() { return name_t(); }
  };

  struct default_value {};
} // namespace cserial
