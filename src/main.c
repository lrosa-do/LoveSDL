

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "lovesdl.h"
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"

static lua_State *L;
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;


static void deinit(void)
{
          SDL_LogInfo(0,"Close lib");

          SDL_Log("Current memory usage: %u bytes\n", dmt_usage());
      dmt_dump();


      lua_close(L);

}


static int onLuaPanic(lua_State *L) {


  const char *err = lua_tostring(L, -1);
  SDL_LogError(0,"Lua Panic :%s",err);
  return 0;
}




 void LuaExecuteFile(const char *filename)
{


    int result = luaL_dofile(L, filename);

    switch (result)
    {
        case LUA_OK: break;
        case LUA_ERRRUN: SDL_LogError(0, "Lua Runtime Error: %s", lua_tostring(L, -1));
        case LUA_ERRMEM: SDL_LogError(0, "Lua Memory Error: %s", lua_tostring(L, -1));
        default: SDL_LogError(0, "Lua Error: %s", lua_tostring(L, -1));
    }
}


int luaopen_love(lua_State *L);


int love_Loader(lua_State *L)
{


    const char *name = luaL_checkstring(L, 1);

    char* fileName=TextFormat("%s%s",name,".lua");

    if (FileExists(fileName))
    {
        int size;
        const char* data= (const char*)LoadFileData(fileName,&size);
        int status = luaL_loadbuffer(L, data, size, name);
        RL_FREE(data);
        /*
      switch (status)
        {
        case LUA_ERRMEM:
            return luaL_error(L, "Memory allocation error: %s\n", lua_tostring(L, -1));
        case LUA_ERRSYNTAX:
            return luaL_error(L, "Syntax error: %s\n", lua_tostring(L, -1));
        default:
           {
            lua_pop(L, 1);
            SDL_Log( "LOVE: [%s] Compiled .", fileName);
            return 1;
            }*/
                  lua_pop(L, 1);
            SDL_Log( "LOVE: [%s] Compiled .", fileName);
            return 1;


 } else
 {
   SDL_LogError(0, "FILEIO: [%s] Failed to open file", fileName);
 }

    return 0;
}

void love_SetLoader(lua_State* L)
{
    lua_register(L, "loveLoader", love_Loader);
    const char*  str= "table.insert(package.searchers, 2, loveLoader)";
    //const char*  str= "package.searchers[#package.searchers+1] =loveLoader";
    luaL_dostring(L, str);
}

int main(int argc, char **argv) {

int liveCode = 1;
double liveCodeInterval = 1.5;
int liveCodeReloadAssets = 0;

VGA_WIDTH =640;
VGA_HEIGHT=480;
int VSYNC=0;
char*  TITLE="LoveSDL";
int FPS= 60;

  atexit(deinit);



        if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
        {
                SDL_LogError(0, "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        return 1;
        }



    //Set texture filtering to linear
    if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
    {
        SDL_LogWarn(0, "Warning: Linear texture filtering not enabled!" );
    }

  filesystem_mount(GetApplicationDirectory());
  L = luaL_newstate();

  lua_atpanic(L, onLuaPanic);



  lua_pushboolean(L, liveCode);
  lua_setglobal(L, "liveCode");
  lua_pushnumber(L, liveCodeInterval);
  lua_setglobal(L, "liveCodeInterval");
  lua_pushboolean(L, liveCodeReloadAssets);
  lua_setglobal(L, "liveCodeReloadAssets");

  if (FileExists("conf.lua"))
  {

        LuaExecuteFile("conf.lua");
        lua_getglobal(L, "config_table");
        if (lua_istable(L, -1))
        {

            lua_getfield(L, -1, "window_width");
            VGA_WIDTH  = (int)lua_tonumber(L, -1);
            lua_getfield(L, -2, "window_height");
            VGA_HEIGHT = (int)lua_tonumber(L, -1);
            lua_getfield(L, -3, "vsync");
            VSYNC = (int)lua_tonumber(L, -1);
            lua_getfield(L, -4, "fps");
            FPS = (int)lua_tonumber(L, -1);
             lua_getfield(L, -5, "title");
             TITLE =(char*)lua_tostring(L, -1);
         }
    }

    //Create window
    gWindow = SDL_CreateWindow( TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, VGA_WIDTH, VGA_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if( gWindow == NULL )
    {
        SDL_LogError(0, "Window could not be created! SDL Error: %s\n", SDL_GetError() );

    }
    else
    {
        //Create vsynced renderer for window
        gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
        if( gRenderer == NULL )
        {
            SDL_LogError(0, "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );

        }
        else
        {
            //Initialize renderer color
            SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

        }
    }


  luaL_openlibs(L);
  luaL_requiref(L, "love", luaopen_love, 1);
  lua_getglobal(L, "love");
  love_SetLoader(L);

 // luaL_dostring(L, "package.path = package.path .. ';?.lua'");
  //luaL_dostring(L, "package.path = package.path .. ';?.lua;/sdcard/lua?.lua;/sdcard/lua/assets/?.lua'");



  if (!lua_isnil(L, -1))
  {
    lua_newtable(L);
    int i;
    for (i = 0; i < argc; i++)
    {
      lua_pushstring(L, argv[i]);
      lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "argv");

  }
  lua_pop(L, 1);



    /* Init embedded scripts */
/*
  #include "nogame_lua.h"
  #include "boot_lua.h"
  struct {
    const char *name, *data; int size;
  } items[] = {
    { "nogame.lua",   nogame_lua,   sizeof(nogame_lua)  },
    { "boot.lua",     boot_lua,     sizeof(boot_lua)    },
    { NULL, NULL, 0 }
  };
  int i;
  for (i = 0; items[i].name; i++)
  {
    int err = luaL_loadbuffer(L, items[i].data, items[i].size, items[i].name);
    if (err || lua_pcall(L, 0, 0, 0) != 0)
    {
      const char *str = lua_tostring(L, -1);
      log_error( "Error: %s\n", str);
      abort();
    }
  }

*/

  {
    int size;
    const char* data= (const char*)LoadFileData("nogame.lua",&size);
     int err = luaL_loadbuffer(L, data, size, "nogame");
    if (err || lua_pcall(L, 0, 0, 0) != 0)
    {
      const char *str = lua_tostring(L, -1);
      SDL_LogError(0, "%s\n", str);
      abort();
    }
  }

    {
    int size;
    const char* data= (const char*)LoadFileData("boot.lua",&size);
     int err = luaL_loadbuffer(L, data, size, "boot");
    if (err || lua_pcall(L, 0, 0, 0) != 0)
    {
      const char *str = lua_tostring(L, -1);
      SDL_LogError(0,"%s", str);
      abort();
    }
  }


SDL_DestroyRenderer( gRenderer );
SDL_DestroyWindow( gWindow );
gWindow = NULL;
gRenderer = NULL;



SDL_Quit();

  return 0;
}
