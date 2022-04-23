


#include "luaobj.h"
#include "lovesdl.h"

#define CLASS_TYPE  LUAOBJ_TYPE_FONT
#define CLASS_NAME  "Font"

 #include "font_ttf.h"
int l_font_new(lua_State *L) {
  const char *filename;
  int ptsize = 8;
  if ( lua_isnoneornil(L, 2) )
  {
    filename = NULL;
    ptsize = luaL_optnumber(L, 1, ptsize);
  } else {
    filename = luaL_checkstring(L, 1);
    ptsize = luaL_optnumber(L, 2, ptsize);
  }
  font_t *self = luaobj_newudata(L, sizeof(*self));
  luaobj_setclass(L, CLASS_TYPE, CLASS_NAME);

  if (filename)
  {
    self->font=LoadFont(filename);
    self->isDefault=0;
    self->size =ptsize;
  } else
  {
      // self->font=LoadFont("font.ttf");
    self->font= LoadFontFromMemory("ttf", font_ttf,16272 , 32, NULL, 95);
    self->isDefault=0;
    self->size =ptsize;
  }

  return 1;
}

int l_font_default(lua_State *L)
{
  font_t *self = luaobj_newudata(L, sizeof(*self));
  luaobj_setclass(L, CLASS_TYPE, CLASS_NAME);
  LoadFontDefault();
  self->font=GetFontDefault();
  self->isDefault=1;
  return 1;
}


int l_font_gc(lua_State *L) {
  font_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
  if (self)
  {
   if (self->isDefault)
  {
    UnloadFontDefault();
  } else
  {
    UnloadFont(self->font);
  }
  }

 // font_deinit(self);
  return 0;
}


int l_font_getWidth(lua_State *L) {
  font_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
  const char *p = luaL_checkstring(L, 2);
  int width = 0;
  if(self)
  {

  Vector2 size =MeasureTextEx(self->font,p,10,0);
  width = (int)size.x;
     /*while (*p)
     {
        width += self->glyphs[(int) (*p++ & 127)].xadvance;
      }*/
  }

  lua_pushinteger(L, width);
  return 1;
}


int l_font_getHeight(lua_State *L) {
  font_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
  int w =0;
  if (self)
  {
  w = self->font.recs[0].height;
  }
  lua_pushinteger(L, w);
  return 1;
}



int luaopen_font(lua_State *L) {
  luaL_Reg reg[] = {
    { "new",            l_font_new            },
    { "__gc",           l_font_gc             },
    { "getWidth",       l_font_getWidth       },
    { "getHeight",      l_font_getHeight      },
    { 0, 0 },
  };

  luaobj_newclass(L, CLASS_NAME, NULL, l_font_new, reg);
  return 1;
}
