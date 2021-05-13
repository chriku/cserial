#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"
#include "nienary.hpp"
#include <string>

struct file_content {
  double x;
};
template <> struct cserial::serial<file_content> : serializer<"file_content", field<&file_content::x, "x">> {};

TEST_CASE("nienary double 0") {
  file_content a;
  file_content b;
  a.x = 0;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double 1") {
  file_content a;
  file_content b;
  a.x = 1;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double -1") {
  file_content a;
  file_content b;
  a.x = -1;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double 0.0000001") {
  file_content a;
  file_content b;
  a.x = 0.0000001;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double -0.0000001") {
  file_content a;
  file_content b;
  a.x = -0.0000001;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double 1000000000") {
  file_content a;
  file_content b;
  a.x = 1000000000;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double -1000000000") {
  file_content a;
  file_content b;
  a.x = -1000000000;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double 0.1") {
  file_content a;
  file_content b;
  a.x = 0.1;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double -0.1") {
  file_content a;
  file_content b;
  a.x = -0.1;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == doctest::Approx(a.x));
}

TEST_CASE("nienary double nan") {
  file_content a;
  file_content b;
  a.x = 0.0 / 0;
  cserial::nienary::deserialize(b, cserial::nienary::serialize(a));
  CHECK(b.x == 0);
}
