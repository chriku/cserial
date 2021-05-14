#include "avro.hpp"
#include "avro_variable.hpp"
#include "serialize.hpp"
#include <iostream>

using namespace std::literals;

struct val_t {
  int64_t f;
};
template <>
struct cserial::serial<val_t> : serializer<"val_t", //
                                           field<&val_t::f, "f", parameter<cserial::default_value, 5>>> {};

struct file_content {
  val_t z;
  int32_t x = 0x100;
};
template <>
struct cserial::serial<file_content> : serializer<"file_content",               //
                                                  field<&file_content::z, "z">, //
                                                  field<&file_content::x, "x">> {};
struct file_content2 {
  int64_t x;
  int64_t y;
};
template <>
struct cserial::serial<file_content2> : serializer<"file_content", //
                                                   field<&file_content2::x, "x", parameter<cserial::default_value, 5>>> {};

int main() {
  file_content a;
  file_content2 b;
  std::string t = cserial::avro::serialize(a);
  for (auto c : t)
    std::cout << std::setfill('0') << std::setw(2) << std::hex << ((uint8_t)c) + 0 << " ";
  std::cout << std::dec << std::endl;
  cserial::avro_variable::build_deserializer<file_content2>(cserial::avro::schema<file_content>())(b, t);
  std::cout << b.x << std::endl;
}
