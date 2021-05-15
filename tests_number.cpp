#include "avro.hpp"
#include "avro_variable.hpp"
#include "doctest.h"

namespace test_number {
  struct input_struct {
    int64_t x;
    int64_t y;
  };
  struct output_struct {
    int64_t y;
  };
} // namespace test_number
template <>
struct cserial::serial<test_number::input_struct> : serializer<"struct",                                  //
                                                               field<&test_number::input_struct::x, "x">, //
                                                               field<&test_number::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_number::output_struct> : serializer<"struct", //
                                                                field<&test_number::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip int64_t 1") {
  test_number::input_struct a;
  a.x = 42;
  a.y = 38;
  test_number::output_struct b;
  cserial::avro_variable::build_deserializer<test_number::output_struct>(cserial::avro::schema<test_number::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip int64_t 2") {
  test_number::input_struct a;
  a.x = 42;
  a.y = 0;
  test_number::output_struct b;
  cserial::avro_variable::build_deserializer<test_number::output_struct>(cserial::avro::schema<test_number::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip int64_t 3") {
  test_number::input_struct a;
  a.x = 0;
  a.y = 38;
  test_number::output_struct b;
  cserial::avro_variable::build_deserializer<test_number::output_struct>(cserial::avro::schema<test_number::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip int64_t 4") {
  test_number::input_struct a;
  a.x = 0;
  a.y = 0;
  test_number::output_struct b;
  cserial::avro_variable::build_deserializer<test_number::output_struct>(cserial::avro::schema<test_number::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
