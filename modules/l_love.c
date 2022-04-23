
#include "lovesdl.h"
#include "luaobj.h"
#define LOVE_VERSION "0.0.1"



bool luax_toboolean(lua_State *L, int idx)
{
	return (lua_toboolean(L, idx) != 0);
}

bool luax_checkboolean(lua_State *L, int idx)
{
	luaL_checktype(L, idx, LUA_TBOOLEAN);
	return luax_toboolean(L, idx);
}

void luax_pushboolean(lua_State *L, bool b)
{
	lua_pushboolean(L, b ? 1 : 0);
}

int l_love_getVersion(lua_State *L) {
  lua_pushstring(L, LOVE_VERSION);
  return 1;
}




int luaopen_image(lua_State *L);
int luaopen_quad(lua_State *L);
int luaopen_font(lua_State *L);
int luaopen_source(lua_State *L);
int luaopen_system(lua_State *L);
int luaopen_event(lua_State *L);
int luaopen_filesystem(lua_State *L);
int luaopen_graphics(lua_State *L);
int luaopen_audio(lua_State *L);
int luaopen_timer(lua_State *L);
int luaopen_keyboard(lua_State *L);
int luaopen_mouse(lua_State *L);

int luaopen_love(lua_State *L)
{
  int i;
  int (*classes[])(lua_State *L) =
  {
    luaopen_image,
    luaopen_quad,
     luaopen_font,
    luaopen_source,
    NULL,
  };
  for (i = 0; classes[i]; i++)
  {
    classes[i](L);
    lua_pop(L, 1);
  }

  /* Init love module */
  luaL_Reg reg[] =
  {
    { "getVersion",   l_love_getVersion },
    { 0, 0 },
  };
  luaL_newlib(L, reg);





  /* Init submodules */
  struct { char *name; int (*fn)(lua_State *L); } mods[] = {
    { "graphics",   luaopen_graphics    },
    { "audio",      luaopen_audio       },
    { "event",      luaopen_event       },
    { "timer",      luaopen_timer       },
    { "keyboard",   luaopen_keyboard    },
    { "mouse",      luaopen_mouse       },
    { "system",     luaopen_system      },
    { "filesystem", luaopen_filesystem  },
    { 0 },
  };
  for (i = 0; mods[i].name; i++)
  {
    mods[i].fn(L);
    lua_setfield(L, -2, mods[i].name);
  }
  return 1;
}
