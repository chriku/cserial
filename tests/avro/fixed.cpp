#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "doctest.h"

namespace test_fixed {
  struct input_struct {
    std::array<char, 4> x;
    std::array<char, 4> y;
  };
  struct output_struct {
    std::array<char, 4> y;
  };
} // namespace test_fixed
template <>
struct cserial::serial<test_fixed::input_struct> : serializer<"struct",                                 //
                                                              field<&test_fixed::input_struct::x, "x">, //
                                                              field<&test_fixed::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_fixed::output_struct> : serializer<"struct", //
                                                               field<&test_fixed::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::array<char,4> 1") {
  test_fixed::input_struct a;
  a.x = {'A', 'B', 'C', 'D'};
  a.y = {'X', 'Y', 'Z', '0'};
  test_fixed::output_struct b;
  cserial::avro_variable::build_deserializer<test_fixed::output_struct>(cserial::avro::schema<test_fixed::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 2") {
  test_fixed::input_struct a;
  a.x = {'A', 'B', 'C', 'D'};
  a.y = {'A', 'B', 'C', '0'};
  test_fixed::output_struct b;
  cserial::avro_variable::build_deserializer<test_fixed::output_struct>(cserial::avro::schema<test_fixed::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 3") {
  test_fixed::input_struct a;
  a.x = {'G', 'H', 'I', 'J'};
  a.y = {'X', 'Y', 'Z', '0'};
  test_fixed::output_struct b;
  cserial::avro_variable::build_deserializer<test_fixed::output_struct>(cserial::avro::schema<test_fixed::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 4") {
  test_fixed::input_struct a;
  a.x = {'G', 'H', 'I', 'J'};
  a.y = {'A', 'B', 'C', '0'};
  test_fixed::output_struct b;
  cserial::avro_variable::build_deserializer<test_fixed::output_struct>(cserial::avro::schema<test_fixed::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
