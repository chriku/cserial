#pragma once

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}
#include "serialize.hpp"
#include "utils.hpp"

namespace cserial {
  namespace lua {
    inline void do_refresh(lua_State* L) {
      size_t round = 0;
      size_t cnt = 1;
      while (cnt > round) {
        round++;
        cnt = 0;
        lua_getfield(L, LUA_REGISTRYINDEX, "cserial_refresh");
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
          cnt++;
          lua_call(L, 0, 0);
        }
        lua_pop(L, 1);
      }
    }
    template <typename self_type> struct lua_value;
    template <typename self_type> struct lua_value_struct {
      static inline constexpr void init(lua_State* L) {
        if (luaL_newmetatable(L, serial<self_type>::name().data())) {
          lua_pushcfunction(L, [](lua_State* L) {
            self_type* self = *static_cast<self_type**>(luaL_checkudata(L, 1, serial<self_type>::name().data()));
            size_t name_len;
            const char* name_data = luaL_checklstring(L, 2, &name_len);
            std::string name(name_data, name_len);
            if (name == "_type") {
              lua_pushlstring(L, serial<self_type>::name().data(), serial<self_type>::name().size());
              return 1;
            } else {
              int ret = 0;
              serial<self_type>::iterate(
                  self, [&]<typename current_field>(self_type * value, current_field*) constexpr {
                    if (current_field::name() == name) {
                      lua_value<typename current_field::value_type>::push(L, &(value->*current_field::member_pointer()));
                      ret = 1;
                    }
                  });
              return ret;
            }
            return 0;
          });
          lua_setfield(L, -2, "__index");
          lua_pushcfunction(L, [](lua_State* L) {
            self_type* self = *static_cast<self_type**>(luaL_checkudata(L, 1, serial<self_type>::name().data()));
            size_t name_len;
            const char* name_data = luaL_checklstring(L, 2, &name_len);
            std::string name(name_data, name_len);
            serial<self_type>::iterate(
                self, [&]<typename current_field>(self_type * value, current_field*) constexpr {
                  if (current_field::name() == name) {
                    lua_value<typename current_field::value_type>::pull(L, &(value->*current_field::member_pointer()), 3);
                    do_refresh(L);
                  }
                });
            return 0;
          });
          lua_setfield(L, -2, "__newindex");
          lua_pop(L, 1);

          lua_newtable(L);
          lua_newtable(L);
          lua_pushliteral(L, "k");
          lua_setfield(L, -2, "__mode");
          lua_setmetatable(L, -2);
          lua_setfield(L, LUA_REGISTRYINDEX, "cserial_refresh");

          serial<self_type>::iterate(
              static_cast<self_type*>(nullptr), [&]<typename current_field>(self_type*, current_field*) constexpr { lua_value<typename current_field::value_type>::init(L); });
        } else
          lua_pop(L, 1);
      }
      static void push(lua_State* L, self_type* value) {
        *static_cast<self_type**>(lua_newuserdatauv(L, sizeof(self_type*), 2)) = value;
        luaL_setmetatable(L, serial<self_type>::name().data());
      }
      static void pull(lua_State* L, self_type* value, int index) {
        self_type* self = *static_cast<self_type**>(luaL_checkudata(L, index, serial<self_type>::name().data()));
        *value = *self;
      }
    };
    template <typename self_type> struct lua_value_convert {
      using serial_type = typename serial<self_type>::serial_type;
      static inline constexpr void init(lua_State* L) {
        lua_value<serial_type>::init(L);
        if (luaL_newmetatable(L, typeid(self_type).name())) {
          lua_pushcfunction(L, [](lua_State* L) {
            std::string name = typeid(self_type).name();
            serial_type* self = static_cast<serial_type*>(luaL_checkudata(L, 1, name.data()));
            self->~serial_type();
            return 0;
          });
          lua_setfield(L, -2, "__gc");
        } else
          lua_pop(L, 1);
      }
      static void push(lua_State* L, self_type* value) {
        std::string name = typeid(self_type).name();
        void* pos = lua_newuserdatauv(L, sizeof(serial_type), 0);
        serial_type* v2 = new (pos) serial_type;
        luaL_setmetatable(L, name.data());
        serial<self_type>::convert(*value, *v2);
        lua_rotate(L, lua_gettop(L) - 1, 1);
        lua_setiuservalue(L, -2, 1);
        lua_value<serial_type>::push(L, v2);
        if (lua_isuserdata(L, -1)) {
          lua_getfield(L, LUA_REGISTRYINDEX, "cserial_refresh");
          lua_pushvalue(L, -2);
          lua_pushvalue(L, -3);
          lua_pushlightuserdata(L, value);
          lua_pushcclosure(
              L,
              [](lua_State* L) -> int {
                self_type* value = static_cast<self_type*>(lua_touserdata(L, lua_upvalueindex(2)));
                serial_type v2;
                lua_value<serial_type>::pull(L, &v2, lua_upvalueindex(1));
                serial<self_type>::unconvert(v2, *value);
                return 0;
              },
              2);
          lua_settable(L, -3);
          lua_pop(L, 1);
        }
      }
      static void pull(lua_State* L, self_type* value, int index) {
        serial_type v2;
        lua_value<serial_type>::pull(L, &v2, index);
        serial<self_type>::unconvert(v2, *value);
      }
    };
    template <typename self_type>
    struct lua_value : std::conditional_t<std::is_base_of_v<available_converter, serial<self_type>>, lua_value_convert<self_type>, lua_value_struct<self_type>> {};
    template <> struct lua_value<bool> {
      static inline constexpr void init(lua_State* L) {}
      static void push(lua_State* L, bool* value) { lua_pushboolean(L, *value); }
      static void pull(lua_State* L, bool* value, int index) { *value = lua_toboolean(L, index); }
    };
    template <typename int_type> struct lua_value_int {
      static inline constexpr void init(lua_State* L) {}
      static void push(lua_State* L, int_type* value) { lua_pushinteger(L, *value); }
      static void pull(lua_State* L, int_type* value, int index) { *value = luaL_checkinteger(L, index); }
    };
    template <> struct lua_value<uint64_t> : lua_value_int<uint64_t> {};
    template <> struct lua_value<int64_t> : lua_value_int<int64_t> {};
    template <> struct lua_value<uint32_t> : lua_value_int<uint32_t> {};
    template <> struct lua_value<int32_t> : lua_value_int<int32_t> {};
    template <> struct lua_value<uint16_t> : lua_value_int<uint16_t> {};
    template <> struct lua_value<int16_t> : lua_value_int<int16_t> {};
    template <> struct lua_value<uint8_t> : lua_value_int<uint8_t> {};
    template <> struct lua_value<int8_t> : lua_value_int<int8_t> {};
    template <typename... types> struct lua_value<std::variant<types...>> {
      static inline constexpr void init(lua_State* L) {}
      static void push(lua_State* L, std::variant<types...>* value) {}
      static void pull(lua_State* L, std::variant<types...>* value, int index) {}
    };
  } // namespace lua
} // namespace cserial
