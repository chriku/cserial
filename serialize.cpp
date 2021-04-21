#include "serialize.hpp"
#include "avro.hpp"

template <typename base_type> struct serialize_single_field;
template <> struct serialize_single_field<int> {
  static void serialize(int& target, std::string_view name) {
    /*std::cout << "Serialize int " << name << " with value " << target << std::endl;*/
    target++;
  }
};

template <typename class_type> void debug_serializer(class_type* complete_value) {
  decltype(complete_value->serializer_info)::iterate(complete_value, [&]<typename current_field>(class_type* s, current_field*) {
    serialize_single_field<typename current_field::value_type>::serialize(s->*current_field::member_pointer(), current_field::name());
  });
}

struct Z {
  int x;
  int y;
  serializer<serializable_field<&Z::x, "x">, serializable_field<&Z::y, "y">> serializer_info;
};
struct Y {
  int b;
  int c;
  int d;
  std::unordered_map<std::string, int> z;
  serializer<serializable_field<&Y::b, "b">, serializable_field<&Y::c, "c">, serializable_field<&Y::d, "d">, serializable_field<&Y::z, "z">> serializer_info;
};

int main() {
  Y a;
  a.b = 0;
  a.c = 0;
  a.d = 0;
  a.z["foo"] = 3;
  avro::schema<Y>();
  avro::serialize(a);
}
