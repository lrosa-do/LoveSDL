
 #include "luaobj.h"
 #include "lovesdl.h"




int l_mouse_getPosition(lua_State *L)
{
  lua_pushinteger(L, GetMouseX());
  lua_pushinteger(L, GetMouseY());
  return 2;
}


int l_mouse_getX(lua_State *L) {
  lua_pushinteger(L, GetMouseX());
  return 1;
}


int l_mouse_getY(lua_State *L) {
  lua_pushinteger(L, GetMouseY());
  return 1;
}

int l_mouse_setVisible(lua_State *L)
{
int n = lua_gettop(L);
bool value =	luax_checkboolean(L, 1) ;
SDL_ShowCursor(value);
 return 0;
}

int l_mouse_isDown(lua_State *L) {
  int n = lua_gettop(L);
  int res = 0;
  int i;
  int idx = luaL_checknumber(L, 0);

  res = IsMouseButtonPressed(idx);
  lua_pushboolean(L, res);
  return 1;
}



int luaopen_mouse(lua_State *L) {
  luaL_Reg reg[] = {
    { "getPosition",  l_mouse_getPosition   },
    { "setVisible",  l_mouse_setVisible   },
    { "getX",         l_mouse_getX          },
    { "getY",         l_mouse_getY          },
    { "isDown",       l_mouse_isDown        },
    { 0, 0 },
  };
  luaL_newlib(L, reg);
  return 1;
}
