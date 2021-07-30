#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace cserial {
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
  /**
   * \brief A parameter for a field
   */
  template <typename key_t, auto val_t> struct parameter {
    using key = key_t;
    constexpr static auto value = val_t;
  };
  /**
   * \brief Describes the layout of a serializable struct
   */
  template <typename base_class> struct serial;

  /**
   * \brief Descripe a field inside a serializationd definition
   * \tparam member_pointer_t Pointer to the member of the class where data should be loaded/stored
   * \tparam name_t Name of the field in the struct
   * \tparam parameters_t list of 0..n parameters
   */
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

  struct available_converter {};

  /**
   * \brief Default builder for simple structs
   * \tparam name_t Name of the struct to be used in serialization
   * \tparam args List of fields
   */
  template <typename base_type_t, typename serial_type_t> struct converter : available_converter {
    using base_type = base_type_t;
    using serial_type = serial_type_t;
  };

  /**
   * \brief Default builder for simple structs
   * \tparam name_t Name of the struct to be used in serialization
   * \tparam args List of fields
   */
  template <string_literal name_t, typename... args> struct serializer {
    template <typename self_type, typename executor_type> static inline void iterate(self_type* s, executor_type executor) {
      if constexpr (sizeof...(args) > 0)
        field_iterator<self_type, executor_type, args...>::execute(s, executor);
    }
    static inline std::string_view name() { return name_t(); }
  };
  template <class T1, class T2> struct serial<std::pair<T1, T2>> : serializer<"pair", field<&std::pair<T1, T2>::first, "first">, field<&std::pair<T1, T2>::second, "second">> {};

  template <typename clock, typename duration> struct serial<std::chrono::time_point<clock, duration>> : converter<std::chrono::time_point<clock, duration>, duration> {
    static void convert(const std::chrono::time_point<clock, duration>& a, duration& b) { b = a.time_since_epoch(); }
    static void unconvert(const duration& a, std::chrono::time_point<clock, duration>& b) { b = std::chrono::time_point<clock, duration>(a); }
  };

  template <typename Rep, typename Period> struct serial<std::chrono::duration<Rep, Period>> : converter<std::chrono::duration<Rep, Period>, Rep> {
    static void convert(const std::chrono::duration<Rep, Period>& a, Rep& b) { b = a.count(); }
    static void unconvert(const Rep& a, std::chrono::duration<Rep, Period>& b) { b = std::chrono::duration<Rep, Period>(a); }
  };

  template <typename subtype> struct serial<std::optional<subtype>> : converter<std::optional<subtype>, std::variant<std::monostate, subtype>> {
    static void convert(const std::optional<subtype>& a, std::variant<std::monostate, subtype>& b) {
      if (a)
        b = *a;
      else
        b = std::monostate();
    }
    static void unconvert(const std::variant<std::monostate, subtype>& a, std::optional<subtype>& b) {
      if (std::holds_alternative<subtype>)
        b = std::get<subtype>(a);
      else
        b.reset();
    }
  };

  /**
   * \brief Default key for the default_value parameter.
   * Should be used to fill fields, when they are missing from the schema.
   */
  struct default_value {};
} // namespace cserial
