#include "avro.hpp"
#include "avro_variable.hpp"
#include "doctest.h"

namespace test_string {
  struct input_struct {
    std::string x;
    std::string y;
  };
  struct output_struct {
    std::string y;
  };
} // namespace test_string
template <>
struct cserial::serial<test_string::input_struct> : serializer<"struct",                                  //
                                                               field<&test_string::input_struct::x, "x">, //
                                                               field<&test_string::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_string::output_struct> : serializer<"struct", //
                                                                field<&test_string::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::string 1") {
  test_string::input_struct a;
  a.x = "ABCDEF";
  a.y = "XYZ";
  test_string::output_struct b;
  cserial::avro_variable::build_deserializer<test_string::output_struct>(cserial::avro::schema<test_string::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::string 2") {
  test_string::input_struct a;
  a.x = "ABCDEF";
  a.y = "";
  test_string::output_struct b;
  cserial::avro_variable::build_deserializer<test_string::output_struct>(cserial::avro::schema<test_string::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::string 3") {
  test_string::input_struct a;
  a.x = "";
  a.y = "XYZ";
  test_string::output_struct b;
  cserial::avro_variable::build_deserializer<test_string::output_struct>(cserial::avro::schema<test_string::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::string 4") {
  test_string::input_struct a;
  a.x = "";
  a.y = "";
  test_string::output_struct b;
  cserial::avro_variable::build_deserializer<test_string::output_struct>(cserial::avro::schema<test_string::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
