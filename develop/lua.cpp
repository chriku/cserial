#include "cserial/lua.hpp"
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
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  cserial::lua::lua_value<file_content>::init(L);
  file_content fc;
  cserial::lua::lua_value<file_content>::push(L, &fc);
  lua_setglobal(L, "val");
  if (luaL_dostring(L, R"(
    do
      local vx=val.x
      vx.a = true
    end
  )") != LUA_OK)
    throw std::runtime_error(lua_tostring(L, -1));
  std::cout << std::boolalpha << fc.x.x << std::endl;
  lua_close(L);
}
