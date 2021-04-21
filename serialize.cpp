#include "serialize.hpp"
#include "avro.hpp"

using namespace std::literals;

template <typename base_type> struct serialize_single_field;

struct Z {
  int x;
  int y;
};
template <> struct serial<Z> : serializer<serializable_field<&Z::x, "x">, serializable_field<&Z::y, "y">> {};
struct Y {
  int b;
  int c;
  Z d;
  std::vector<int> z;
  std::unordered_map<std::string, int> y;
  std::optional<int> x;
  std::variant<int, int> w;
};
template <>
struct serial<Y> : serializer<serializable_field<&Y::b, "b">, serializable_field<&Y::c, "c">, serializable_field<&Y::d, "d">, serializable_field<&Y::z, "z">,
                              serializable_field<&Y::y, "y">, serializable_field<&Y::x, "x">, serializable_field<&Y::w, "w">> {};

int main() {
  Y a;
  a.b = 42;
  a.c = 38;
  a.d.x = 7;
  a.d.y = 8;
  a.z.push_back(5);
  a.z.push_back(6);
  a.z.push_back(7);
  a.y["a"] = 3;
  a.y["b"] = 4;
  a.x = 3;
  a.w = std::variant<int, int>(std::in_place_index<1>, 8);
  avro::schema<Y>();
  std::string ser_str = avro::serialize(a);
  std::cout << "\"";
  for (uint8_t c : ser_str) {
    std::cout << "\\x" << std::setfill('0') << std::setw(2) << std::hex << c + 0;
  }
  std::cout << "\"s" << std::dec << std::endl;
  Y b;
  avro::deserialize(b, ser_str);
  std::cout << b.b << std::endl;
  std::cout << b.c << std::endl;
  std::cout << b.d.x << std::endl;
  std::cout << b.d.y << std::endl;
  std::cout << b.z.size() << std::endl;
  std::cout << b.z.at(0) << std::endl;
  std::cout << b.z.at(1) << std::endl;
  std::cout << b.z.at(2) << std::endl;
  std::cout << b.y.at("a") << std::endl;
  std::cout << b.y.at("b") << std::endl;
  std::cout << std::boolalpha << b.x.has_value() << std::endl;
  std::cout << b.w.index() << std::endl;
  std::cout << std::get<1>(b.w) << std::endl;
}
