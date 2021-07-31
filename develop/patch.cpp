#include "cserial/patch.hpp"
#include "cserial/serialize.hpp"
#include <iostream>

using namespace std::literals;

struct file_content3 {
  bool a = false;
};
template <>
struct cserial::serial<file_content3> : serializer<"file_content3", //
                                                   field<&file_content3::a, "a">> {};

struct file_content2 {
  bool x = false;
};
template <> struct cserial::serial<file_content2> : converter<file_content2, file_content3> {
  static void convert(const file_content2& a, file_content3& b) { b.a = a.x; }
  static void unconvert(const file_content3& a, file_content2& b) { b.x = a.a; }
};
struct file_content {
  file_content2 x;
  std::optional<bool> y = true;
};
template <>
struct cserial::serial<file_content> : serializer<"file_content", //
                                                  field<&file_content::x, "x">, field<&file_content::y, "y">> {};

int main() {
  file_content fc;
  cserial::patch::patch p;
  std::cout << std::boolalpha << fc.x.x << std::endl;
  cserial::patch::apply<file_content>(fc, R"(
    {"x":{"a": true},"y":{"1": true}}
  )"_json);
  std::cout << std::boolalpha << fc.x.x << std::endl;
}
