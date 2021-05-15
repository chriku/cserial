#include "avro.hpp"
#include "avro_variable.hpp"
#include "doctest.h"

namespace test_bool {
  struct input_struct {
    bool x;
    bool y;
  };
  struct output_struct {
    bool y;
  };
} // namespace test_bool
template <>
struct cserial::serial<test_bool::input_struct> : serializer<"struct",                                //
                                                             field<&test_bool::input_struct::x, "x">, //
                                                             field<&test_bool::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_bool::output_struct> : serializer<"struct", //
                                                              field<&test_bool::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip bool 1") {
  test_bool::input_struct a;
  a.x = true;
  a.y = true;
  test_bool::output_struct b;
  cserial::avro_variable::build_deserializer<test_bool::output_struct>(cserial::avro::schema<test_bool::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip bool 2") {
  test_bool::input_struct a;
  a.x = true;
  a.y = false;
  test_bool::output_struct b;
  cserial::avro_variable::build_deserializer<test_bool::output_struct>(cserial::avro::schema<test_bool::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip bool 3") {
  test_bool::input_struct a;
  a.x = false;
  a.y = true;
  test_bool::output_struct b;
  cserial::avro_variable::build_deserializer<test_bool::output_struct>(cserial::avro::schema<test_bool::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip bool 4") {
  test_bool::input_struct a;
  a.x = false;
  a.y = false;
  test_bool::output_struct b;
  cserial::avro_variable::build_deserializer<test_bool::output_struct>(cserial::avro::schema<test_bool::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
