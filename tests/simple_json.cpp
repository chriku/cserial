#include "doctest.h"
#include <cserial/simple_json.hpp>

using nlohmann::json;

struct tc1 : cserial::simple_json {
  double a;
};

template <> struct cserial::serial<tc1> : serializer<"TC1", field<&tc1::a, "a">> {};

TEST_CASE("simple parse") {
  json j = R"+({"a": 5})+"_json;
  auto r = j.get<tc1>();
  CHECK(r.a == 5);
  json k(r);
  CHECK(k == R"+({"a": 5})+"_json);
}

struct tc2 : cserial::simple_json {
  std::optional<double> a;
};

template <> struct cserial::serial<tc2> : serializer<"TC2", field<&tc2::a, "a">> {};

TEST_CASE("parse optional") {
  {
    json j = R"+({"a": 5})+"_json;
    auto r = j.get<tc2>();
    CHECK(r.a.value_or(0) == 5);
    json k(r);
    CHECK(k == R"+({"a": 5})+"_json);
  }
  {
    json j = R"+({"a": null})+"_json;
    auto r = j.get<tc2>();
    CHECK(!r.a.has_value());
    json k(r);
    CHECK(k == R"+({"a": null})+"_json);
  }
}
