#include "lovesdl.h"
#include "luaobj.h"

extern SDL_Window* gWindow ;
extern SDL_Renderer* gRenderer;


#define CLASS_TYPE  LUAOBJ_TYPE_IMAGE
#define CLASS_NAME  "Image"


int l_image_new(lua_State *L)
{

  const char *filename = luaL_checkstring(L, 1);
  image_t *self = luaobj_newudata(L, sizeof(*self));
  luaobj_setclass(L, CLASS_TYPE, CLASS_NAME);

   const char *err=NULL;

    self->surface = LoadSurfaceFromFile(filename);
	if( self->surface == NULL )
	{
		err=TextFormat("Unable to load image %s! ",filename);
	}
	else
	{
        self->tex = SDL_CreateTextureFromSurface( gRenderer, self->surface );
		if( self->tex == NULL )
		{
		  err=TextFormat("Unable to texture texture from  %s! ",filename);
		}
		self->width = self->surface->w;
		self->height= self->surface->h;

	}
  if (err) luaL_error(L, err);
  return 1;
}


int l_image_newCanvas(lua_State *L)
{
  int width = VGA_WIDTH;
  int height = VGA_HEIGHT;
  if (lua_gettop(L) > 0)
  {
    width = luaL_checknumber(L, 1);
    height = luaL_checknumber(L, 2);
    if (width <= 0) luaL_argerror(L, 1, "width must be larger than 0");
    if (height <= 0) luaL_argerror(L, 2, "height must be larger than 0");
  }
  image_t *self = luaobj_newudata(L, sizeof(*self));
  luaobj_setclass(L, CLASS_TYPE, CLASS_NAME);

  return 1;
}


int l_image_gc(lua_State *L)
{
  image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
        if (self)
        {
        if (self->tex)
        {
                 SDL_DestroyTexture(self->tex);
                 SDL_FreeSurface(self->surface);
                 TRACELOGD("Image: Unloaded image data from RAM and VRAM");
        }
        }

  return 0;
}


int l_image_getDimensions(lua_State *L) {
  image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);

     if (self)
        {
        if (self->tex)
        {
                lua_pushinteger(L, self->width);
                lua_pushinteger(L, self->height);
        }
        }



  return 2;
}


int l_image_getWidth(lua_State *L) {
  image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
  if (self)
        {
        if (self->tex)
        {
                lua_pushinteger(L, self->width);

        }
        }
  return 1;
}


int l_image_getHeight(lua_State *L) {
  image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
  if (self)
        {
        if (self->tex)
        {

                lua_pushinteger(L, self->height);
        }
        }
  return 1;
}


int l_image_getPixel(lua_State *L)
{
image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);
if (self)
{
if (self->tex)
{
        lua_pushinteger(L, self->width);
        lua_pushinteger(L, self->height);
}
}
}


int l_image_setPixel(lua_State *L)
{
image_t *self = luaobj_checkudata(L, 1, CLASS_TYPE);

  return 0;
}


int luaopen_image(lua_State *L) {
  luaL_Reg reg[] = {
    { "new",            l_image_new           },
    { "__gc",           l_image_gc            },
    { "getDimensions",  l_image_getDimensions },
    { "getWidth",       l_image_getWidth      },
    { "getHeight",      l_image_getHeight     },
    { "getPixel",       l_image_getPixel      },
    { "setPixel",       l_image_setPixel      },
    { 0, 0 },
  };
  luaobj_newclass(L, CLASS_NAME, NULL, l_image_new, reg);
  return 1;
}
