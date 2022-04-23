

#include <string.h>
#include <stdlib.h>

#include "dmt.h"
#include "lovesdl.h"
#include "luaobj.h"

extern SDL_Window* gWindow ;
extern SDL_Renderer* gRenderer;

font_t   *graphics_defaultFont;
font_t   *graphics_font;
Color   graphics_backgroundColor;
int       graphics_backgroundColor_rgb[4];
Color   graphics_color;
int       graphics_color_rgb[4];
int       graphics_blendMode;
int       graphics_alpha;
SDL_BlendMode currentBlend;


static Color getColorFromArgs(lua_State *L, int *rgb, const int *def)
{
  int r, g, b,a;
  if ( lua_isnoneornil(L, 1) )
  {
    r = def[0];
    g = def[1];
    b = def[2];
    a = 255;
  } else
  {
    r = luaL_checknumber(L, 1);
    g = luaL_checknumber(L, 2);
    b = luaL_checknumber(L, 3);
    a = 255;
  }

  if (rgb)
  {
    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;
    rgb[3] = 255;
  }
  return (Color){r,g,b,a};
}


static int pushColor(lua_State *L, int *rgb)
{
  lua_pushinteger(L, rgb[0]);
  lua_pushinteger(L, rgb[1]);
  lua_pushinteger(L, rgb[2]);
  return 3;
}


int l_graphics_getDimensions(lua_State *L)
 {
  int w,h;
  SDL_GetWindowSize(gWindow,&w,&h);
  lua_pushinteger(L, w);
  lua_pushinteger(L, h);
  return 2;
}


int l_graphics_getWidth(lua_State *L) {
int w,h;
  SDL_GetWindowSize(gWindow,&w,&h);

 lua_pushinteger(L, w);
  return 1;
}


int l_graphics_getHeight(lua_State *L) {
int w,h;
  SDL_GetWindowSize(gWindow,&w,&h);

 lua_pushinteger(L, h);

  return 1;
}


int l_graphics_getBackgroundColor(lua_State *L) {
  return pushColor(L, graphics_backgroundColor_rgb);
}


int l_graphics_setBackgroundColor(lua_State *L)
{
  static const int def[] = { 0, 0, 0 };
  graphics_backgroundColor_rgb[0]=RAYWHITE.r;
  graphics_backgroundColor_rgb[1]=RAYWHITE.g;
  graphics_backgroundColor_rgb[2]=RAYWHITE.b;
  graphics_backgroundColor_rgb[3]=RAYWHITE.r;
  Color idx = getColorFromArgs(L, graphics_backgroundColor_rgb, def);
  graphics_backgroundColor = idx;
  return 0;
}


int l_graphics_getColor(lua_State *L)
{
  return pushColor(L, graphics_color_rgb);
}


int l_graphics_setColor(lua_State *L)
{
  static const int def[] = { 0xff, 0xff, 0xff };
  graphics_color = getColorFromArgs(L, graphics_color_rgb, def);
  SDL_SetRenderDrawColor(gRenderer, graphics_color.r,graphics_color.g,graphics_color.b,graphics_alpha);
  return 0;
}

int  l_graphics_setAlpha(lua_State *L)
{
  graphics_alpha = luaL_checknumber(L, 1);
  return 0;
}




int l_graphics_getBlendMode(lua_State *L)
{
  switch (graphics_blendMode)
  {
    default:
    case NONE : lua_pushstring(L, "none");  break;
    case BLEND   : lua_pushstring(L, "blend");    break;
    case ADD    : lua_pushstring(L, "add");     break;
    case MOD     : lua_pushstring(L, "mod");      break;

  }
  return 1;
}


int l_graphics_setBlendMode(lua_State *L)
{
  //const char *str = lua_isnoneornil(L, 1);
  const char *str = luaL_checkstring(L, 1);

  if (str)
  {


  if (!strcmp(str,"none"))
  {
//  SDL_BlendMode(SDL_BLENDMODE_NONE);
  currentBlend = SDL_BLENDMODE_NONE;
  } else
  if (!strcmp(str,"blend"))
  {
 // SDL_BlendMode(SDL_BLENDMODE_BLEND);
  currentBlend = SDL_BLENDMODE_BLEND;
  } else
  if (!strcmp(str,"add"))
  {
  //SDL_BlendMode(SDL_BLENDMODE_ADD);
  currentBlend = SDL_BLENDMODE_ADD;
  } else
  if (!strcmp(str,"mod"))
  {
  //SDL_BlendMode(SDL_BLENDMODE_MOD);
  currentBlend = SDL_BLENDMODE_MOD;
  } else
  {
   luaL_argerror(L, 1, "Bad blend mode");
  }

}
  return 0;
}


int l_graphics_getFont(lua_State *L) {
  lua_pushlightuserdata(L, graphics_font);
  lua_gettable(L, LUA_REGISTRYINDEX);
  return 1;
}


int l_graphics_setFont(lua_State *L)
 {
font_t *oldFont = graphics_font;
  if (lua_isnoneornil(L, 1)) {
    /* If no arguments are given we use the default embedded font, grab it
     * from the registry and set it as the first argument */
    lua_pushlightuserdata(L, &graphics_defaultFont);
    lua_gettable(L, LUA_REGISTRYINDEX);
//    lua_insert(L, 1);
  }
  graphics_font = luaobj_checkudata(L, 1, LUAOBJ_TYPE_FONT);
  /* Remove old font from registry. This is done after we know the args are
   * okay so that the font remains unchanged if an error occurs */
  if (oldFont) {
    lua_pushlightuserdata(L, oldFont);
    lua_pushnil(L);
    lua_settable(L, LUA_REGISTRYINDEX);
  }
  /* Add new font to registry */
  lua_pushlightuserdata(L, graphics_font);
  lua_pushvalue(L, 1);
  lua_settable(L, LUA_REGISTRYINDEX);
  return 0;
}


int l_graphics_getCanvas(lua_State *L) {
  //lua_pushlightuserdata(L, graphics_canvas);
//  lua_gettable(L, LUA_REGISTRYINDEX);
  return 1;
}


int l_graphics_setCanvas(lua_State *L)
{

  return 0;
}


int l_graphics_reset(lua_State *L)
 {
  int (*funcs[])(lua_State*) = {
    l_graphics_setBackgroundColor,
    l_graphics_setColor,
  //  l_graphics_setBlendMode,
    l_graphics_setFont,
    l_graphics_setCanvas,
    NULL,
  };
  int i;
  for (i = 0; funcs[i]; i++) {
    lua_pushcfunction(L, funcs[i]);
    lua_call(L, 0, 0);
  }
  return 0;
}


int l_graphics_clear(lua_State *L)
{
if (!gRenderer) return 0;
Color idx = getColorFromArgs(L, NULL, graphics_backgroundColor_rgb);
SDL_SetRenderDrawColor( gRenderer, idx.r,idx.g, idx.b, 0xFF );
SDL_RenderClear( gRenderer );
 return 0;
}



int l_graphics_present(lua_State *L)
{
 if (!gRenderer) return 0;
  SDL_RenderPresent( gRenderer );
  return 0;
}

/*

Texture texture
A Texture (Image or Canvas) to texture the Quad with.
Quad quad
The Quad to draw on screen.
number x
The position to draw the object (x-axis).
number y
The position to draw the object (y-axis).
number r (0)
Orientation (radians).
number sx (1)
Scale factor (x-axis).
number sy (sx)
Scale factor (y-axis).
number ox (0)
Origin offset (x-axis).
number oy (0)
Origin offset (y-axis).

*/

int l_graphics_draw(lua_State *L)
{
	const int top = lua_gettop(L);



  image_t *img = luaobj_checkudata(L, 1, LUAOBJ_TYPE_IMAGE);
  quad_l *quad = NULL;
  int x, y, flip;
  double angle = 0;
  double sx = 1;
  double sy = 1;
  int w;
  int h;
  SDL_Point center ={0,0};





  if (!lua_isnone(L, 2) && lua_type(L, 2) != LUA_TNUMBER)
  {
    quad = luaobj_checkudata(L, 2, LUAOBJ_TYPE_QUAD);
    x = luaL_optnumber(L, 3, 0);
    y = luaL_optnumber(L, 4, 0);
    flip = !lua_isnone(L, 5) && lua_toboolean(L, 5);

    if (!lua_isnone(L, 6) && luaL_optnumber(L, 6,0))
    {
      angle = luaL_optnumber(L, 6,0);
    }

     if (!lua_isnone(L, 7) && luaL_optnumber(L, 7,0) && !lua_isnone(L, 8) && luaL_optnumber(L, 8,0))
    {
      sx = luaL_optnumber(L, 7,0);
      sy = luaL_optnumber(L, 8,0);
    }


  } else
  {
    x = luaL_optnumber(L, 2, 0);
    y = luaL_optnumber(L, 3, 0);
    flip = !lua_isnone(L, 4) && lua_toboolean(L, 4);

    if (!lua_isnone(L, 5) && luaL_optnumber(L, 5,0))
    {
      angle = luaL_optnumber(L, 5,0);
    }

         if (!lua_isnone(L, 6) && luaL_optnumber(L, 6,0) && !lua_isnone(L, 7) && luaL_optnumber(L, 7,0))
        {
          sx = luaL_optnumber(L, 6,0);
          sy = luaL_optnumber(L, 7,0);
        }



  }

  SDL_RendererFlip flipType = SDL_FLIP_NONE;
  if (flip)
  {
  flipType = SDL_FLIP_HORIZONTAL;
  }

  if (quad)
  {

      if (img)
    {
      if (img->tex)
      {


        SDL_SetTextureBlendMode(img->tex,currentBlend);
        SDL_SetTextureAlphaMod(img->tex,graphics_alpha);
        SDL_SetTextureColorMod(img->tex,graphics_color.r,graphics_color.g,graphics_color.b);

          SDL_Rect clip = { quad->x,quad->y,quad->width,quad->height };

           w = clip.w  * sx;
           h = clip.h  * sy;

          SDL_Rect renderQuad = { x, y, w,h };

          center.x = w/2;
          center.y = h/2;


             if (!lua_isnone(L, 9) && luaL_optnumber(L, 9,0) && !lua_isnone(L, 10) && luaL_optnumber(L, 10,0))
            {
              center.x = luaL_optnumber(L, 9 ,0);
              center.x = luaL_optnumber(L, 10,0);
            }

	      SDL_RenderCopyEx( gRenderer, img->tex, &clip, &renderQuad, angle, &center, flipType );
      }

    }

  } else
  {
    if (img)
    {
      if (img->tex)
      {

           w = img->width  * sx;
           h = img->height * sy;

          SDL_Rect renderQuad = { x, y, w,h };



          center.x = renderQuad.w/2;
          center.y = renderQuad.h/2;


         if (!lua_isnone(L, 8) && luaL_optnumber(L, 8,0) && !lua_isnone(L, 9) && luaL_optnumber(L, 9,0))
        {
          center.x = luaL_optnumber(L, 8,0);
          center.x = luaL_optnumber(L, 9,0);
        }



    SDL_SetTextureBlendMode(img->tex,currentBlend);
    SDL_SetTextureAlphaMod(img->tex,graphics_alpha);
    SDL_SetTextureColorMod(img->tex,graphics_color.r,graphics_color.g,graphics_color.b);
	SDL_RenderCopyEx( gRenderer, img->tex, NULL, &renderQuad, angle, &center, flipType );

      }

    }
  }
  return 0;
}


int l_graphics_point(lua_State *L) {
  int x = luaL_checknumber(L, 1);
  int y = luaL_checknumber(L, 2);
  DrawPoint(x,y,graphics_color);
  return 0;
}


int l_graphics_line(lua_State *L) {
  int argc = lua_gettop(L);
  int x = luaL_checknumber(L, 1);
  int y = luaL_checknumber(L, 2);
  int x2 = luaL_checknumber(L, 3) ;
  int y2 = luaL_checknumber(L, 4) ;
  DrawLine(x,y,x2,y2,graphics_color);
  return 0;
}


int l_graphics_rectangle(lua_State *L)
{
  const char *mode = luaL_checkstring(L, 1);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4) ;
  int y2 = luaL_checknumber(L, 5) ;

  int fill = 0;
  if (!strcmp(mode, "fill"))
  {
    fill = 1;
  } else if (!strcmp(mode, "line"))
  {
    fill = 0;
  } else
  {
    luaL_error(L, "bad mode");
  }


  if (fill==1)
     DrawRectangle(x,y,x2,y2,graphics_color);
     else
     DrawRectangleLines(x,y,x2,y2,graphics_color);

  return 0;
}

int l_graphics_arc(lua_State *L) {

  int x = luaL_checknumber(L, 1);
  int y = luaL_checknumber(L, 2);
  int radius = luaL_checknumber(L, 3);
  int start = luaL_checknumber(L, 4);
  int end = luaL_checknumber(L, 5);
  arcRGBA(gRenderer,x,y,radius,start,end,0,0,0,0);
 return 0;
}

int l_graphics_pie(lua_State *L)
{
  const char *mode = luaL_checkstring(L, 1);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  int radius = luaL_checknumber(L, 4);
  int start = luaL_checknumber(L, 5);
  int end = luaL_checknumber(L, 6);

  int fill = 0;
  if (!strcmp(mode, "fill")) {
    fill = 1;
  } else if (!strcmp(mode, "line")) {
    fill = 0;
  } else {
    luaL_error(L, "bad mode");
  }



  if (fill)
     filledPieRGBA(gRenderer,x,y,radius,start,end,0,0,0,0);
     else
     pieRGBA(gRenderer,x,y,radius,start,end,0,0,0,0);



  return 0;
}


int l_graphics_box(lua_State *L)
{

  int x1 = luaL_checknumber(L, 1);
  int y1 = luaL_checknumber(L, 2);
  int x2 = luaL_checknumber(L, 3);
  int y2 = luaL_checknumber(L, 4);

  boxRGBA(gRenderer,x1,y1,x2,y2,0,0,0,0);


  return 0;
}

int l_graphics_rounded(lua_State *L)
{
  const char *mode = luaL_checkstring(L, 1);
  int x1 = luaL_checknumber(L, 2);
  int y1 = luaL_checknumber(L, 3);
  int x2 = luaL_checknumber(L, 4);
  int y2 = luaL_checknumber(L, 5);
  int rad = luaL_checknumber(L, 6);


  int fill = 0;
  if (!strcmp(mode, "fill")) {
    fill = 1;
  } else if (!strcmp(mode, "line")) {
    fill = 0;
  } else {
    luaL_error(L, "bad mode");
  }
  if (fill)
     roundedRectangleRGBA(gRenderer,x1,y1,x2,y2,rad,0,0,0,0);
     else
     roundedBoxRGBA(gRenderer,x1,y1,x1,y1,rad,0,0,0,0);


  return 0;
}



int l_graphics_ellipse(lua_State *L) {
  const char *mode = luaL_checkstring(L, 1);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  int rx = luaL_checknumber(L, 4);
  int ry = luaL_checknumber(L, 5);

  int fill = 0;
  if (!strcmp(mode, "fill")) {
    fill = 1;
  } else if (!strcmp(mode, "line")) {
    fill = 0;
  } else {
    luaL_error(L, "bad mode");
  }
  if (fill)
     filledEllipseRGBA(gRenderer,x,y,rx,ry,0,0,0,0);
     else
     ellipseRGBA(gRenderer,x,y,rx,ry,0,0,0,0);
  return 0;
}



int l_graphics_circle(lua_State *L) {
  const char *mode = luaL_checkstring(L, 1);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  int radius = luaL_checknumber(L, 4);
  int fill = 0;
  if (!strcmp(mode, "fill")) {
    fill = 1;
  } else if (!strcmp(mode, "line")) {
    fill = 0;
  } else {
    luaL_error(L, "bad mode");
  }



  if (fill)
     DrawFilledCircle(x,y,radius,graphics_color);
     else
     DrawCircle(x,y,radius,graphics_color);



  return 0;
}

int l_graphics_polygon(lua_State *L)
{

    int args = lua_gettop(L) -1;
    const char *mode = luaL_checkstring(L, 1);
	bool is_table = false;
	if (lua_istable(L, 2))
	{
		args = (int) luax_objlen(L, 2);
		is_table = true;
	}





	if (args % 2 != 0)
		return luaL_error(L, "Number of points components must be a multiple of two");
	else if (args < 6)
		return luaL_error(L, "Need at least 3 vertices to draw a line");

  int fill = 0;
  if (!strcmp(mode, "fill")) {
    fill = 1;
  } else if (!strcmp(mode, "line")) {
    fill = 0;
  } else {
    luaL_error(L, "bad mode");
  }

    int numvertices = args / 2;

    Sint16* x_coords =(Sint16 *) dmt_malloc((numvertices + 1)*sizeof(Sint16));
	Sint16* y_coords =(Sint16 *) dmt_malloc((numvertices + 1)*sizeof(Sint16));

	if (is_table)
	{
		for (int i = 0; i < numvertices; ++i)
		{
			lua_rawgeti(L, 2, (i * 2) + 1);
			lua_rawgeti(L, 2, (i * 2) + 2);
			x_coords[i] = (Sint16)luaL_checknumber(L, -2);
			y_coords[i] = (Sint16)luaL_checknumber(L, -1);

			//DrawFilledCircle(x_coords[i],y_coords[i],5,RED);
			lua_pop(L, 2);
		}
	}
	else
	{
		for (int i = 0; i < numvertices; ++i)
		{
			x_coords[i] = (Sint16)luaL_checknumber(L, (i * 2) + 2);
			y_coords[i] = (Sint16)luaL_checknumber(L, (i * 2) + 3);
		}
	}

 //  x_coords[numvertices] = x_coords[0];
 //  y_coords[numvertices] = y_coords[0];

 if (fill)
 filledPolygonRGBA(gRenderer,x_coords,y_coords,numvertices);
else
	polygon(gRenderer,x_coords,y_coords,numvertices);

	dmt_free(x_coords);
	dmt_free(y_coords);
/*
	int numvertices = args / 2;
//	SDL_Log("%d %d %d",args,is_table,numvertices);

	SDL_Point* coords =(SDL_Point *) dmt_malloc((numvertices + 1)*sizeof(SDL_Point));


	if (is_table)
	{
		for (int i = 0; i < numvertices; ++i)
		{
			lua_rawgeti(L, 2, (i * 2) + 1);
			lua_rawgeti(L, 2, (i * 2) + 2);
			coords[i].x = (int)luaL_checknumber(L, -2);
			coords[i].y = (int)luaL_checknumber(L, -1);
			lua_pop(L, 2);
		}
	}
	else
	{
		for (int i = 0; i < numvertices; ++i)
		{
			coords[i].x = (int)luaL_checknumber(L, (i * 2) + 2);
			coords[i].y = (int)luaL_checknumber(L, (i * 2) + 3);
		}
	}

 //  coords[numvertices].x = coords[0].x;
 //  coords[numvertices].y = coords[0].y;


  if (fill)
     SDL_RenderDrawLines(gRenderer, coords, numvertices);
     else
     SDL_RenderDrawLines(gRenderer, coords, numvertices);
     	dmt_free(coords);
*/




  return 0;
}

int l_graphics_bezier(lua_State *L)
{

    int args = lua_gettop(L) -1;
    int steps = (int)luaL_checknumber(L, 1);
	bool is_table = false;
	if (lua_istable(L, 2))
	{
		args = (int) luax_objlen(L, 2);
		is_table = true;
	}





	if (args % 2 != 0)
		return luaL_error(L, "Number of points components must be a multiple of two");
	else if (args < 6)
		return luaL_error(L, "Need at least four vertices to draw a line");


	int numvertices = args / 2;
//	SDL_Log("%d %d %d",args,is_table,numvertices);

	double* x_coords =(double *) dmt_malloc((numvertices + 1)*sizeof(double));
	double* y_coords =(double *) dmt_malloc((numvertices + 1)*sizeof(double));

	if (is_table)
	{
		for (int i = 0; i < numvertices; ++i)
		{
			lua_rawgeti(L, 2, (i * 2) + 1);
			lua_rawgeti(L, 2, (i * 2) + 2);
			x_coords[i] = (int)luaL_checknumber(L, -2);
			y_coords[i] = (int)luaL_checknumber(L, -1);
		//	DrawFilledCircle(x_coords[i],y_coords[i],2,RED);
			lua_pop(L, 2);
		}
	}
	else
	{
		for (int i = 0; i < numvertices; ++i)
		{
			x_coords[i] = (int)luaL_checknumber(L, (i * 2) + 2);
			y_coords[i] = (int)luaL_checknumber(L, (i * 2) + 3);
		}
	}

   x_coords[numvertices] = x_coords[0];
   y_coords[numvertices] = y_coords[0];

	bezierRGBA(gRenderer,x_coords,y_coords,numvertices,steps);

	dmt_free(x_coords);
	dmt_free(y_coords);


  return 0;
}




void PrintText(Font f,const char *text, int posX, int posY, int fontSize, Color color)
{



    // Check if default font has been loaded
    if (f.texture.id != 0)
    {
        Vector2 position = { (float)posX, (float)posY };

        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize/defaultFontSize;

        DrawTextEx(f, text, position, (float)fontSize, (float)spacing, color);
    }
}

int l_graphics_print(lua_State *L) {
  luaL_checkany(L, 3);
  int nargs = lua_gettop(L);
  const char *str = luaL_tolstring(L, 1, NULL);
  int x = luaL_checknumber(L, 2);
  int y = luaL_checknumber(L, 3);
  if (graphics_font)
  {
   int size = graphics_font->size;

	if (nargs == 4)
	{
		 size = luaL_checknumber(L, 4);
		return 0;
	}
  PrintText(graphics_font->font,str,x,y,graphics_font->size,graphics_color);
  }

  return 0;
}

extern double  timer_avgLastDt;

int l_graphics_printFPS(lua_State *L)
{
  luaL_checkany(L, 1);
  int x = luaL_checknumber(L, 1);
  int y = luaL_checknumber(L, 2);
  if (graphics_font)
  {

    Color color = WHITE; // good fps
    int fps = (int)(1. / timer_avgLastDt);

    if (fps < 30 && fps >= 15) color = ORANGE;  // warning FPS
    else if (fps < 15) color = RED;    // bad FPS

    PrintText(graphics_font->font,TextFormat("%2i FPS", fps), x, y, 10, color);
  }

  return 0;
}


int l_image_new(lua_State *L);
int l_image_newCanvas(lua_State *L);
int l_quad_new(lua_State *L);
int l_font_new(lua_State *L);
int l_font_default(lua_State *L);

int luaopen_graphics(lua_State *L) {
  luaL_Reg reg[] = {
    { "getDimensions",      l_graphics_getDimensions      },
    { "getWidth",           l_graphics_getWidth           },
    { "getHeight",          l_graphics_getHeight          },
    { "getBackgroundColor", l_graphics_getBackgroundColor },
    { "setBackgroundColor", l_graphics_setBackgroundColor },
    { "getColor",           l_graphics_getColor           },
    { "setColor",           l_graphics_setColor           },
    { "setAlpha",           l_graphics_setAlpha           },
    { "getBlendMode",       l_graphics_getBlendMode       },
    { "setBlendMode",       l_graphics_setBlendMode       },
    { "getFont",            l_graphics_getFont            },
    { "setFont",            l_graphics_setFont            },
    { "getCanvas",          l_graphics_getCanvas          },
    { "setCanvas",          l_graphics_setCanvas          },
    { "reset",              l_graphics_reset              },
    { "clear",              l_graphics_clear              },
    { "present",            l_graphics_present            },
    { "draw",               l_graphics_draw               },
    { "point",              l_graphics_point              },
    { "line",               l_graphics_line               },
    { "bezier",               l_graphics_bezier               },
    { "rectangle",          l_graphics_rectangle          },
    { "circle",             l_graphics_circle             },
    { "print",              l_graphics_print              },
    { "pie"  ,             l_graphics_pie                },
    { "ellipse",              l_graphics_ellipse                         },
    { "arc",              l_graphics_arc                          },
    { "box",              l_graphics_box                },
    { "rounded",                  l_graphics_rounded                },
    { "polygon",              l_graphics_polygon              },
    { "Fps",              l_graphics_printFPS              },
    { "newImage",           l_image_new                   },
    { "newCanvas",          l_image_newCanvas             },
    { "newQuad",            l_quad_new                    },
    { "newFont",            l_font_new                    },
    { 0, 0 },
  };
  luaL_newlib(L, reg);

  graphics_color= WHITE;
  graphics_backgroundColor=WHITE;
  graphics_alpha=255;
  currentBlend = SDL_BLENDMODE_BLEND;
/*
//  lua_pushcfunction(L, l_image_newCanvas);
  lua_pushinteger(L, VGA_WIDTH);
  lua_pushinteger(L, VGA_HEIGHT);
  lua_call(L, 2, 1);
//  graphics_screen = luaobj_checkudata(L, -1, LUAOBJ_TYPE_IMAGE);

 // lua_pushlightuserdata(L, &graphics_screen);
 // lua_pushvalue(L, -2);
 // lua_settable(L, LUA_REGISTRYINDEX);
 // lua_pop(L, 1);
*/



  //lua_pushcfunction(L, l_font_default);
  lua_pushcfunction(L, l_font_new);
  lua_call(L, 0, 1);
  graphics_defaultFont =luaobj_checkudata(L, -1, LUAOBJ_TYPE_FONT);
  graphics_font = graphics_defaultFont;

  lua_pushlightuserdata(L, &graphics_defaultFont);
  lua_pushvalue(L, -2);
  lua_settable(L, LUA_REGISTRYINDEX);
  lua_pop(L, 1); // Pop the Font objec

  lua_pushcfunction(L, l_graphics_reset);
  lua_call(L, 0, 0);

  return 1;
}

