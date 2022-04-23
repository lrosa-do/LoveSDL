#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
#endif
#include <time.h>
#include "luaobj.h"
#include "lovesdl.h"



Uint32 timer_lastStep;
double timer_lastDt;

double  timer_avgLastDt;
double  timer_avgAcc = 1;
int     timer_avgCount;
double  timer_avgTimer;



static int l_time_getNow(lua_State *L) {
  double t;
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  t = (ft.dwHighDateTime * 4294967296.0 / 1e7) + ft.dwLowDateTime / 1e7;
  t -= 11644473600.0;
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = tv.tv_sec + tv.tv_usec / 1e6;
#endif
  lua_pushnumber(L, t);
  return 1;
}

int l_timer_step(lua_State *L)
{

  Uint32 now;
  do
  {
    now = SDL_GetTicks();
    timer_lastDt = (now - timer_lastStep) /(double) 1000;//CLOCKS_PER_SEC;
  } while (timer_lastDt < 0);
  timer_lastStep = now;
  timer_avgAcc += timer_lastDt;
  timer_avgCount++;
  timer_avgTimer -= timer_lastDt;
  if (timer_avgTimer <= 0)
  {
    timer_avgLastDt = (timer_avgAcc / timer_avgCount);
    timer_avgTimer += 1;
    timer_avgAcc = 0;
    timer_avgCount = 0;
  }

 // SDL_Log("%f  %f",timer_lastDt,timer_avgLastDt);


  return 0;
}


int l_timer_sleep(lua_State *L) {
  //SDL_Delay(luaL_checknumber(L, 1) / 1000.0);
  SDL_Delay(luaL_checknumber(L, 1) * 1000.0);
  return 1;
}


int l_timer_getDelta(lua_State *L) {
  lua_pushnumber(L, timer_lastDt);
  return 1;
}


int l_timer_getAverageDelta(lua_State *L) {
  lua_pushnumber(L, timer_avgLastDt);
  return 1;
}


int l_timer_getFPS(lua_State *L) {
  lua_pushnumber(L, (int)(1. / timer_avgLastDt));
  return 1;
}


int l_timer_getTime(lua_State *L) {
  lua_pushnumber(L, SDL_GetTicks() /  1000.);
  return 1;
}


int luaopen_timer(lua_State *L) {
  luaL_Reg reg[] = {
    { "getNow",   l_time_getNow   },
    { "step",             l_timer_step              },
    { "sleep",            l_timer_sleep             },
    { "getDelta",         l_timer_getDelta          },
    { "getAverageDelta",  l_timer_getAverageDelta   },
    { "getFPS",           l_timer_getFPS            },
    { "getTime",          l_timer_getTime           },
    { 0, 0 },
  };
  luaL_newlib(L, reg);
  l_timer_step(L);
  return 1;
}
