#include "avro.hpp"
#include "avro_variable.hpp"
#include "doctest.h"

namespace test_optional {
  struct input_struct {
    std::optional<int64_t> x;
    std::optional<int64_t> y;
  };
  struct output_struct {
    std::optional<int64_t> y;
  };
} // namespace test_optional
template <>
struct cserial::serial<test_optional::input_struct> : serializer<"struct",                                    //
                                                                 field<&test_optional::input_struct::x, "x">, //
                                                                 field<&test_optional::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_optional::output_struct> : serializer<"struct", //
                                                                  field<&test_optional::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::optional 1") {
  test_optional::input_struct a;
  a.x = 42;
  a.y = 38;
  test_optional::output_struct b;
  cserial::avro_variable::build_deserializer<test_optional::output_struct>(cserial::avro::schema<test_optional::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::optional 2") {
  test_optional::input_struct a;
  a.x = 0;
  a.y = 38;
  test_optional::output_struct b;
  cserial::avro_variable::build_deserializer<test_optional::output_struct>(cserial::avro::schema<test_optional::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::optional 3") {
  test_optional::input_struct a;
  a.x = 42;
  a.y = 0;
  test_optional::output_struct b;
  cserial::avro_variable::build_deserializer<test_optional::output_struct>(cserial::avro::schema<test_optional::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::optional 4") {
  test_optional::input_struct a;
  a.x = 0;
  a.y = 0;
  test_optional::output_struct b;
  cserial::avro_variable::build_deserializer<test_optional::output_struct>(cserial::avro::schema<test_optional::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
