#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "doctest.h"

namespace test_convert {
  struct double_struct {
    double y;
  };
  struct int64_t_struct {
    int64_t y;
  };
  struct int32_t_struct {
    int32_t y;
  };
} // namespace test_convert
template <> struct cserial::serial<test_convert::double_struct> : serializer<"struct", field<&test_convert::double_struct::y, "y">> {};
template <> struct cserial::serial<test_convert::int64_t_struct> : serializer<"struct", field<&test_convert::int64_t_struct::y, "y">> {};
template <> struct cserial::serial<test_convert::int32_t_struct> : serializer<"struct", field<&test_convert::int32_t_struct::y, "y">> {};
TEST_CASE("avro_variable double to int64_t") {
  CHECK_THROWS(([] {
    test_convert::double_struct a;
    a.y = 42;
    test_convert::int64_t_struct b;
    cserial::avro_variable::build_deserializer<test_convert::int64_t_struct>(cserial::avro::schema<test_convert::double_struct>())(b, cserial::avro::serialize(a));
    CHECK(a.y == b.y);
  })());
}
TEST_CASE("avro_variable int32_t to int64_t") {
  test_convert::int32_t_struct a;
  a.y = 42;
  test_convert::int64_t_struct b;
  cserial::avro_variable::build_deserializer<test_convert::int64_t_struct>(cserial::avro::schema<test_convert::int32_t_struct>())(b, cserial::avro::serialize(a));
  CHECK(a.y == b.y);
}
