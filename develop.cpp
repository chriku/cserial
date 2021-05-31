#include "avro.hpp"
#include "avro_variable.hpp"
#include "serialize.hpp"
#include <iostream>

using namespace std::literals;

struct file_content {
  int32_t x;
  int32_t y;
  // std::unordered_map<std::string, int32_t> y;
};
template <>
struct cserial::serial<file_content> : serializer<"file_content",               //
                                                  field<&file_content::x, "x">, //
                                                  field<&file_content::y, "y">> {};
/*struct file_content2 {
  std::unordered_map<std::string, int32_t> y;
};
template <>
struct cserial::serial<file_content2> : serializer<"file_content", //
                                                   field<&file_content2::y, "y">> {};*/

void w(void (*o)(std::string_view), file_content& a) { cserial::avro::serialize(a, o); }
/*int main() {
  file_content a;
  file_content2 b;
  a.y.emplace("A", 4);
  a.y.emplace("B", 5);
  a.y.emplace("C", 6);
  std::string t = cserial::avro::serialize(a);
  for (auto c : t)
    std::cout << std::setfill('0') << std::setw(2) << std::hex << ((uint8_t)c) + 0 << " ";
  std::cout << std::dec << std::endl;
  std::cout << cserial::avro::schema<file_content>() << std::endl;
  cserial::avro_variable::build_deserializer<file_content2>(cserial::avro::schema<file_content>())(b, t);
  std::cout << b.y.size() << std::endl;
}*/
