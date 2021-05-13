#include "serialize.hpp"
#include "avro.hpp"
#include "nienary.hpp"
#include <iostream>

using namespace std::literals;

struct file_content {
  uint32_t x = 10000;
};
template <> struct cserial::serial<file_content> : serializer<"file_content", field<&file_content::x, "x", parameter<cserial::default_value, 5>>> {};
struct file_content2 {
  uint32_t x;
  /*std::optional<bounded::integer<5, 10>> y;
  std::variant<std::string, int32_t> z;*/
};
template <> struct cserial::serial<file_content2> : serializer<"file_content", field<&file_content2::x, "x">> {};

int main() {
  file_content a;
  file_content b;
  a.x++;
  std::string t = cserial::nienary::serialize(a);
  cserial::nienary::deserialize(b, t);
  std::cout << t.size() << ":" << b.x << std::endl;
  // a.x = -42.3;
  /*a.y = -38;
  a.z = std::variant<std::string, int32_t>(std::in_place_index<1>, -38);
  std::cout << cserial::nienary::serialize(a);*/
  /*bounded::integer<3, 3> j = 3_bi;
  auto i = j + 3_bi;
  std::cout << std::numeric_limits<decltype(i)>::min() << ":" << std::numeric_limits<decltype(i)>::max() << std::endl;*/
}
