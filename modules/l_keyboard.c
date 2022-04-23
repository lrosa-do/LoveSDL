#include "luaobj.h"
#include "lovesdl.h"

#include <string.h>
#include <ctype.h>
#define KEY_MAX 128
#define BUFFER_SIZE 32
#define BUFFER_MASK (BUFFER_SIZE - 1)


int l_keyboard_setKeyRepeat(lua_State *L) {
 // keyboard_setKeyRepeat( lua_toboolean(L, 1) );
  return 0;
}



int l_keyboard_isDown(lua_State *L)
{
  int n = lua_gettop(L);
  const char *key = luaL_checkstring(L, 1);
  int res = 0;
  int scanCode=SDL_GetScancodeFromName(key);
  if (IsKeyDown(scanCode))
  {
    res=1;
  }
  lua_pushboolean(L, res);
  return 1;
}


int luaopen_keyboard(lua_State *L) {
  luaL_Reg reg[] = {
    { "setKeyRepeat", l_keyboard_setKeyRepeat },
    { "isDown",       l_keyboard_isDown       },
    { 0, 0 },
  };
  luaL_newlib(L, reg);
  return 1;
}
