#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "doctest.h"

namespace test_vector {
  struct input_struct {
    std::vector<int32_t> x;
    std::vector<int32_t> y;
  };
  struct output_struct {
    std::vector<int32_t> y;
  };
} // namespace test_vector
template <>
struct cserial::serial<test_vector::input_struct> : serializer<"struct",                                  //
                                                               field<&test_vector::input_struct::x, "x">, //
                                                               field<&test_vector::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_vector::output_struct> : serializer<"struct", //
                                                                field<&test_vector::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::array<char,4> 1") {
  test_vector::input_struct a;
  a.x = {'A', 'B', 'C', 'D'};
  a.y = {'X', 'Y', 'Z', '0'};
  test_vector::output_struct b;
  cserial::avro_variable::build_deserializer<test_vector::output_struct>(cserial::avro::schema<test_vector::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 2") {
  test_vector::input_struct a;
  a.x = {'A', 'B', 'C', 'D'};
  a.y = {'A', 'B', 'C', '0'};
  test_vector::output_struct b;
  cserial::avro_variable::build_deserializer<test_vector::output_struct>(cserial::avro::schema<test_vector::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 3") {
  test_vector::input_struct a;
  a.x = {'G', 'H', 'I', 'J'};
  a.y = {'X', 'Y', 'Z', '0'};
  test_vector::output_struct b;
  cserial::avro_variable::build_deserializer<test_vector::output_struct>(cserial::avro::schema<test_vector::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::array<char,4> 4") {
  test_vector::input_struct a;
  a.x = {'G', 'H', 'I', 'J'};
  a.y = {'A', 'B', 'C', '0'};
  test_vector::output_struct b;
  cserial::avro_variable::build_deserializer<test_vector::output_struct>(cserial::avro::schema<test_vector::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
