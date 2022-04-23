
#include "lovesdl.h"
#include "filesystem.h"
#include "luaobj.h"
#include <sys/stat.h>               // Required for: stat() [Used in GetFileModTime()]
#include <dirent.h>             // Required for: DIR, opendir(), closedir() [Used in GetDirectoryFiles()]


int l_filesystem_mount(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  int err = filesystem_mount(path);
  if (err) {
    lua_pushnil(L);
    lua_pushstring(L, filesystem_strerror(err));
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}


int l_filesystem_unmount(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int err = filesystem_unmount(path);
  if (err) {
    lua_pushnil(L);
    lua_pushstring(L, filesystem_strerror(err));
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}


int l_filesystem_exists(lua_State *L)
{
  const char *filename = luaL_checkstring(L, 1);
  int state = filesystem_exists(filename) ;
  lua_pushboolean( L, state);
  return 1;
}


int l_filesystem_isFile(lua_State *L)
{
   const char *filename = luaL_checkstring(L, 1);
   int state = filesystem_isFile(filename) ;
   lua_pushboolean( L, state);
  return 1;
}


int l_filesystem_isDirectory(lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  lua_pushboolean( L, filesystem_isDirectory(filename) );
  return 1;
}


int l_filesystem_read(lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  int size;
  void *data = filesystem_read(filename, &size);
  if (!data) {
    luaL_error(L, "could not read file");
  }
  lua_pushlstring(L, data, size);
  filesystem_free(data);
  return 1;
}


int l_filesystem_setWriteDir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int err = filesystem_setWriteDir(path);
  if (err) {
    lua_pushnil(L);
    lua_pushstring(L, filesystem_strerror(err));
    return 2;
  }
  lua_pushboolean(L, 1);
  return 1;
}


int l_filesystem_write(lua_State *L) {
  size_t sz;
  const char *filename = luaL_checkstring(L, 1);
  const char *data = luaL_tolstring(L, 2, &sz);
  int err = filesystem_write(filename, data, sz);
  if (err) {
    luaL_error(L, "%s", filesystem_strerror(err));
  }
  lua_pushboolean(L, 1);
  return 1;
}




int l_filesystem_file_mod_time(lua_State *L)
{
  const char *filename = luaL_checkstring(L, 1);
  lua_pushnumber(L,GetFileModTime(filename));
  return 1;
}

int l_filesystem_getSize(lua_State *L)
{
  const char *filename = luaL_checkstring(L, 1);
  lua_pushnumber(L,GetFileSize(filename));
  return 1;
}

int l_filesystem_getWorkingDirectory(lua_State *L)
{
  lua_pushstring(L,GetWorkingDirectory());
  return 1;
}
int l_filesystem_getInfo(lua_State *L)
{

const char *filename = luaL_checkstring(L, 1);

            lua_createtable(L, 0, 3);

            if (filesystem_isFile(filename))
            {
            lua_pushstring(L, "file");
            } else
            if (filesystem_isDirectory(filename))
            {
            lua_pushstring(L, "directory");
            } else
            {
            lua_pushstring(L, "other");
            }


                lua_setfield(L, -2, "type");
        lua_pushnumber(L, (lua_Number) GetFileSize(filename));
        lua_setfield(L, -2, "size");
        lua_pushnumber(L, (lua_Number) GetFileModTime(filename));
        lua_setfield(L, -2, "modtime");
        lua_pushstring(L, GetFileExtension(filename));
                lua_setfield(L, -2, "ext");


        return 1;
}

int l_filesystem_getDirectoryItems(lua_State *L)
{
        const char *dirPath = luaL_checkstring(L, 1);


        int count = 0;
        char** files;
        files = GetDirectoryFiles(dirPath, &count);
        lua_createtable(L, count , 0);
        for (int i = 0; i < (int)count; i++)
        {
            lua_pushstring(L, files[i]);
            lua_rawseti(L, -2, i + 1);
        }
    ClearDirectoryFiles();

        return 1;
}

int l_filesystem_load(lua_State *L)
{

    const char *filename = luaL_checkstring(L, 1);
    int size;
    const char* data= (const char*)LoadFileData(filename,&size);
    int status = luaL_loadbuffer(L, data, size, filename);
    RL_FREE(data);

  switch (status)
        {
        case LUA_ERRMEM:
                return luaL_error(L, "Memory allocation error: %s\n", lua_tostring(L, -1));
        case LUA_ERRSYNTAX:
                return luaL_error(L, "Syntax error: %s\n", lua_tostring(L, -1));
        default: // success
                return 1;
        }


/*      std::string filename = std::string(luaL_checkstring(L, 1));

        Data *data = nullptr;
        try
        {
                data = instance()->read(filename.c_str());
        }
        catch (love::Exception &e)
        {
                return luax_ioError(L, "%s", e.what());
        }

        int status = luaL_loadbuffer(L, (const char *)data->getData(), data->getSize(), ("@" + filename).c_str());

        data->release();

        // Load the chunk, but don't run it.
        switch (status)
        {
        case LUA_ERRMEM:
                return luaL_error(L, "Memory allocation error: %s\n", lua_tostring(L, -1));
        case LUA_ERRSYNTAX:
                return luaL_error(L, "Syntax error: %s\n", lua_tostring(L, -1));
        default: // success
                return 1;
        }*/
}

int luaopen_filesystem(lua_State *L) {
  luaL_Reg reg[] = {
    { "mount",        l_filesystem_mount        },
    { "unmount",      l_filesystem_unmount      },
    { "exists",       l_filesystem_exists       },
    { "isFile",       l_filesystem_isFile       },
    { "isDirectory",  l_filesystem_isDirectory  },
    { "read",         l_filesystem_read         },
    { "load",         l_filesystem_load         },
    { "setWriteDir",  l_filesystem_setWriteDir  },
    { "write",        l_filesystem_write        },
    { "getInfo",        l_filesystem_getInfo        },
    { "getSize",        l_filesystem_getSize        },
    {"enumerate",      l_filesystem_getDirectoryItems        },
    {"getDirectoryItems",      l_filesystem_getDirectoryItems        },
    {"getLastModified",      l_filesystem_file_mod_time        },
    {"getWorkingDirectory",      l_filesystem_getWorkingDirectory        },
    { 0, 0 },
  };
  luaL_newlib(L, reg);
  return 1;
}
