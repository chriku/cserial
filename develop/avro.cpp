#include "cserial/avro.hpp"
#include "cserial/avro_variable.hpp"
#include "cserial/serialize.hpp"
#include <iostream>

using namespace std::literals;

struct file_content {
  bool x = true;
};
template <>
struct cserial::serial<file_content> : serializer<"file_content", //
                                                  field<&file_content::x, "x">> {};

struct file_content2 {
  bool x;
};
template <> struct cserial::serial<file_content2> : converter<file_content2, std::optional<long>> {
  static void convert(const file_content2& a, std::optional<long>& b) {
    if (a.x)
      b = 5;
  }
  static void unconvert(const std::optional<long>& a, file_content2& b) { b.x = a.has_value(); }
};

int main() {
  {
    file_content2 a;
    a.x = true;
    std::string t = cserial::avro::serialize(a);
    for (auto c : t)
      std::cout << std::setfill('0') << std::setw(2) << std::hex << ((uint8_t)c) + 0 << " ";
    std::cout << std::dec << std::endl;
  }
  {
    file_content2 a;
    a.x = false;
    std::string t = cserial::avro::serialize(a);
    for (auto c : t)
      std::cout << std::setfill('0') << std::setw(2) << std::hex << ((uint8_t)c) + 0 << " ";
    std::cout << std::dec << std::endl;
  }
  std::cout << cserial::avro::schema<file_content2>() << std::endl;
}
