

#if _WIN32
  #include <windows.h>
#elif __linux__
  #include <unistd.h>
#elif __APPLE__
  #include <mach-o/dyld.h>
#endif
#include "luaobj.h"
#include "lovesdl.h"




static const char *buttonStr(int id) {
  switch (id) {
    case 1  : return "left";
    case 2  : return "middle";
    case 3  : return "right";
    case 4  : return "wheelup";
    case 5  : return "wheeldown";
    default : return "?";
  }
}

static char keyChar(char c) {
  return (c == '\r') ? '\n' : c;
}


int l_system_getOS(lua_State *L) {
  #if _WIN32
    lua_pushstring(L, "windows");
#elif __linux__
    lua_pushstring(L, "linux");
#elif __FreeBSD__
    lua_pushstring(L, "bsd");
#elif __APPLE__
    lua_pushstring(L, "osx");
#else
    lua_pushstring(L, "?");
#endif
  return 1;
}

static char *dirname(char *str) {
  char *p = str + strlen(str);
  while (p != str) {
    if (*p == '/' || *p == '\\') {
      *p = '\0';
      break;
    }
    p--;
  }
  return str;
}

static int l_system_info(lua_State *L) {
  const char *str = luaL_checkstring(L, 1);
  if (!strcmp(str, "os")) {
#if _WIN32
    lua_pushstring(L, "windows");
#elif __linux__
    lua_pushstring(L, "linux");
#elif __FreeBSD__
    lua_pushstring(L, "bsd");
#elif __APPLE__
    lua_pushstring(L, "osx");
#else
    lua_pushstring(L, "?");
#endif

  } else if (!strcmp(str, "exedir")) {
    UNUSED(dirname);
#if _WIN32
    char buf[1024];
    int len = GetModuleFileName(NULL, buf, sizeof(buf) - 1);
    buf[len] = '\0';
    dirname(buf);
    lua_pushfstring(L, "%s", buf);
#elif __linux__
    char path[128];
    char buf[1024];
    sprintf(path, "/proc/%d/exe", getpid());
    int len = readlink(path, buf, sizeof(buf) - 1);
    ASSERT( len != -1 );
    buf[len] = '\0';
    dirname(buf);
    lua_pushfstring(L, "%s", buf);
#elif __FreeBSD__
    /* TODO : Implement this */
    lua_pushfstring(L, ".");
#elif __APPLE__
    char buf[1024];
    uint32_t size = sizeof(buf);
    ASSERT( _NSGetExecutablePath(buf, &size) == 0 );
    dirname(buf);
    lua_pushfstring(L, "%s", buf);
#else
    lua_pushfstring(L, ".");
#endif

  } else if (!strcmp(str, "appdata")) {
#if _WIN32
    lua_pushfstring(L, "%s", getenv("APPDATA"));
#elif __APPLE__
    lua_pushfstring(L, "%s/Library/Application Support", getenv("HOME"));
#else
    lua_pushfstring(L, "%s/.local/share", getenv("HOME"));
#endif
  } else {
    luaL_argerror(L, 1, "invalid string");
  }
  return 1;
}

int l_system_getMemUsage(lua_State *L) {
  lua_pushnumber(L, lua_gc(L, LUA_GCCOUNT, 0) + dmt_usage() / 1024);
 // lua_pushnumber(L, lua_gc(L, LUA_GCCOUNT, 0)  / 1024);
  return 1;
}



int luaopen_system(lua_State *L)
{
  luaL_Reg reg[] = {
    { "getInfo",     l_system_info     },
    { "getOS",        l_system_getOS        },
    { "getMemUsage",  l_system_getMemUsage  },
    { 0, 0 },
  };
  luaL_newlib(L, reg);
  return 1;
}

