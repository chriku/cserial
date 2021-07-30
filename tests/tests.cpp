#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "doctest.h"
#include <string>

struct file_content {
  double x;
};
template <> struct cserial::serial<file_content> : serializer<"file_content", field<&file_content::x, "x">> {};

TEST_CASE("avro double 0") {
  file_content a;
  file_content b;
  a.x = 0;
  cserial::avro::deserialize_sv(b, cserial::avro::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("avro double 1") {
  file_content a;
  file_content b;
  a.x = 1;
  cserial::avro::deserialize_sv(b, cserial::avro::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("avro double -1") {
  file_content a;
  file_content b;
  a.x = -1;
  cserial::avro::deserialize_sv(b, cserial::avro::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("avro double 0.0000001") {
  file_content a;
  file_content b;
  a.x = 0.0000001;
  cserial::avro::deserialize_sv(b, cserial::avro::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}
