#include "avro.hpp"
#include "avro_variable.hpp"
#include "doctest.h"

namespace test_map {
  struct input_struct {
    std::unordered_map<std::string, int32_t> x;
    std::unordered_map<std::string, int32_t> y;
  };
  struct output_struct {
    std::unordered_map<std::string, int32_t> y;
  };
} // namespace test_map
template <>
struct cserial::serial<test_map::input_struct> : serializer<"struct",                               //
                                                            field<&test_map::input_struct::x, "x">, //
                                                            field<&test_map::input_struct::y, "y">> {};
template <>
struct cserial::serial<test_map::output_struct> : serializer<"struct", //
                                                             field<&test_map::output_struct::y, "y">> {};
TEST_CASE("avro_variable skip std::unordered_map<std::string, int32_t> 1") {
  test_map::input_struct a;
  a.x = std::unordered_map<std::string, int32_t>{{"A", 1}, {"B", 2}, {"C", 3}};
  a.y = std::unordered_map<std::string, int32_t>{{"A", 1}, {"B", 2}, {"C", 3}};
  test_map::output_struct b;
  cserial::avro_variable::build_deserializer<test_map::output_struct>(cserial::avro::schema<test_map::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::unordered_map<std::string, int32_t> 2") {
  test_map::input_struct a;
  a.x = std::unordered_map<std::string, int32_t>{{"A", 1}, {"B", 2}, {"C", 3}};
  a.y = std::unordered_map<std::string, int32_t>{{"X", 1}, {"Y", 2}, {"Z", 3}};
  test_map::output_struct b;
  cserial::avro_variable::build_deserializer<test_map::output_struct>(cserial::avro::schema<test_map::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::unordered_map<std::string, int32_t> 3") {
  test_map::input_struct a;
  a.x = std::unordered_map<std::string, int32_t>{{"X", 1}, {"Y", 2}, {"Z", 3}};
  a.y = std::unordered_map<std::string, int32_t>{{"A", 1}, {"B", 2}, {"C", 3}};
  test_map::output_struct b;
  cserial::avro_variable::build_deserializer<test_map::output_struct>(cserial::avro::schema<test_map::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
TEST_CASE("avro_variable skip std::unordered_map<std::string, int32_t> 4") {
  test_map::input_struct a;
  a.x = std::unordered_map<std::string, int32_t>{{"X", 1}, {"Y", 2}, {"Z", 3}};
  a.y = std::unordered_map<std::string, int32_t>{{"X", 1}, {"Y", 2}, {"Z", 3}};
  test_map::output_struct b;
  cserial::avro_variable::build_deserializer<test_map::output_struct>(cserial::avro::schema<test_map::input_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
