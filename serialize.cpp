#include "serialize.hpp"
#include "avro.hpp"
#include "nienary.hpp"
#include "nienary_bounded.hpp"
#include <iostream>

using namespace std::literals;
using namespace bounded::literal;

struct file_content {
  bounded::integer<9900, 10100> x = 10000_bi;
  /*std::optional<bounded::integer<5, 10>> y;
  std::variant<std::string, int32_t> z;*/
};
template <>
struct cserial::serial<file_content>
    : serializer<"file_content", serializable_field<&file_content::x, "x"> /*, serializable_field<&file_content::y, "y">, serializable_field<&file_content::z, "z">*/> {};
struct file_content2 {
  uint32_t x;
  /*std::optional<bounded::integer<5, 10>> y;
  std::variant<std::string, int32_t> z;*/
};
template <> struct cserial::serial<file_content2> : serializer<"file_content", serializable_field<&file_content2::x, "x">> {};

int main() {
  file_content a;
  file_content2 b;
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
