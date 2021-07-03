#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "doctest.h"

namespace test_variant {
  struct input_struct {
    std::variant<int64_t, std::string> x;
    std::variant<int64_t, std::string> y;
  };
  struct output_struct {
    std::variant<int64_t, std::string> y;
  };
} // namespace test_variant
template <>
struct cserial::serial<test_variant::input_struct> : serializer<"struct",                                   //
                                                                field<&test_variant::input_struct::x, "x">, //
                                                                field<&test_variant::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_variant::output_struct> : serializer<"struct", //
                                                                 field<&test_variant::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::variant<int64_t,std::string> 1") {
  test_variant::input_struct a;
  a.x = 42;
  a.y = 38;
  test_variant::output_struct b;
  cserial::avro_variable::build_deserializer<test_variant::output_struct>(cserial::avro::schema<test_variant::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::variant<int64_t,std::string> 2") {
  test_variant::input_struct a;
  a.x = 42;
  a.y = "ABC";
  test_variant::output_struct b;
  cserial::avro_variable::build_deserializer<test_variant::output_struct>(cserial::avro::schema<test_variant::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::variant<int64_t,std::string> 3") {
  test_variant::input_struct a;
  a.x = "ABC";
  a.y = 38;
  test_variant::output_struct b;
  cserial::avro_variable::build_deserializer<test_variant::output_struct>(cserial::avro::schema<test_variant::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::variant<int64_t,std::string> 4") {
  test_variant::input_struct a;
  a.x = "ABC";
  a.y = "ABC";
  test_variant::output_struct b;
  cserial::avro_variable::build_deserializer<test_variant::output_struct>(cserial::avro::schema<test_variant::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
