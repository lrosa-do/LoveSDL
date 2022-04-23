
#include "lovesdl.h"
#include "luaobj.h"


//const Uint8 *key_state;


extern SDL_Window* gWindow ;
extern SDL_Renderer* gRenderer;

#define MAX_TOUCH_POINTS              10
#define MAX_KEY_PRESSED_QUEUE         16
#define MAX_CHAR_PRESSED_QUEUE        16

typedef struct { int x; int y; } Point;
typedef struct { float x; float y; } Pointf;
typedef struct { int  width; int height; } Size;


enum {
  EVENT_NULL,
  EVENT_QUIT,
  EVENT_WINDOW_RESIZE,
  EVENT_KEYBOARD_PRESSED,
  EVENT_KEYBOARD_RELEASED,
  EVENT_KEYBOARD_TEXTINPUT,
  EVENT_MOUSE_MOVED,
  EVENT_MOUSE_PRESSED,
  EVENT_MOUSE_RELEASED,

};

typedef union {
  int type;

  struct {
    int type;
    int status;
  } quit;


  struct {
  int w;
  int h;
  } window;

  struct {
    int type;
    int x, y;
    int dx, dy;
    int button;
  } mouse;

  struct {
    int type;
     char *key;
    char text[64];
    int scancode;
    int isrepeat;
  } keyboard;


} event_t;


typedef struct InputData {


   SDL_Window* win;
   Size size;
        struct {
            int exitKey;                    // Default exit key
            char currentKeyState[512];      // Registers current frame key state
            char previousKeyState[512];     // Registers previous frame key state
            int keyPressedQueue[MAX_KEY_PRESSED_QUEUE];     // Input keys queue
            int keyPressedQueueCount;       // Input keys queue count
            int charPressedQueue[MAX_CHAR_PRESSED_QUEUE];   // Input characters queue
            int charPressedQueueCount;      // Input characters queue count


        } Keyboard;
        struct {
            Point position;               // Mouse position on screen
            Point offset;                 // Mouse offset
            Point scale;                  // Mouse scaling

            char currentButtonState[3];     // Registers current mouse button state
            char previousButtonState[3];    // Registers previous mouse button state
            float currentWheelMove;         // Registers current mouse wheel variation
            float previousWheelMove;        // Registers previous mouse wheel variation

        } Mouse;
        struct {
            Point position[MAX_TOUCH_POINTS];         // Touch position on screen
            char currentTouchState[MAX_TOUCH_POINTS];   // Registers current touch state
            char previousTouchState[MAX_TOUCH_POINTS];  // Registers previous touch state
        } Touch;



} InputData;




static InputData Input = { 0 };


//----------------------------------------------------------------------------------
// Module Functions Definition - Input (Keyboard, Mouse, Gamepad) Functions
//----------------------------------------------------------------------------------

// Detect if a key has been pressed once
bool IsKeyPressed(int key)
{
    bool pressed = false;

    if ((Input.Keyboard.previousKeyState[key] == 0) && (Input.Keyboard.currentKeyState[key] == 1)) pressed = true;
    else pressed = false;

    return pressed;
}
bool IsAnyKeyPressed()
{
    bool keyPressed = false;
    int key = GetKeyPressed();

    //if ((key >= 32) && (key <= 126)) keyPressed = true;

    return (key != 0);
}

// Detect if a key is being pressed (key held down)
bool IsKeyDown(int key)
{
    if (Input.Keyboard.currentKeyState[key] == 1) return true;
    else return false;
}

// Detect if a key has been released once
bool IsKeyReleased(int key)
{
    bool released = false;

    if ((Input.Keyboard.previousKeyState[key] == 1) && (Input.Keyboard.currentKeyState[key] == 0)) released = true;
    else released = false;

    return released;
}

// Detect if a key is NOT being pressed (key not held down)
bool IsKeyUp(int key)
{
    if (Input.Keyboard.currentKeyState[key] == 0) return true;
    else return false;
}

// Get the last key pressed
int GetKeyPressed(void)
{
    int value = 0;

    if (Input.Keyboard.keyPressedQueueCount > 0)
    {
        // Get character from the queue head
        value = Input.Keyboard.keyPressedQueue[0];

        // Shift elements 1 step toward the head.
        for (int i = 0; i < (Input.Keyboard.keyPressedQueueCount - 1); i++)
            Input.Keyboard.keyPressedQueue[i] = Input.Keyboard.keyPressedQueue[i + 1];

        // Reset last character in the queue
        Input.Keyboard.keyPressedQueue[Input.Keyboard.keyPressedQueueCount] = 0;
        Input.Keyboard.keyPressedQueueCount--;
    }

    return value;
}

// Get the last char pressed
int GetCharPressed(void)
{
    int value = 0;

    if (Input.Keyboard.charPressedQueueCount > 0)
    {
        // Get character from the queue head
        value = Input.Keyboard.charPressedQueue[0];

        // Shift elements 1 step toward the head.
        for (int i = 0; i < (Input.Keyboard.charPressedQueueCount - 1); i++)
            Input.Keyboard.charPressedQueue[i] = Input.Keyboard.charPressedQueue[i + 1];

        // Reset last character in the queue
        Input.Keyboard.charPressedQueue[Input.Keyboard.charPressedQueueCount] = 0;
        Input.Keyboard.charPressedQueueCount--;
    }

    return value;
}




// Detect if a mouse button has been pressed once
bool IsMouseButtonPressed(int button)
{
    bool pressed = false;

    if ((Input.Mouse.currentButtonState[button] == 1) && (Input.Mouse.previousButtonState[button] == 0)) pressed = true;

    // Map touches to mouse buttons checking
    if ((Input.Touch.currentTouchState[button] == 1) && (Input.Touch.previousTouchState[button] == 0)) pressed = true;

    return pressed;
}

// Detect if a mouse button is being pressed
bool IsMouseButtonDown(int button)
{
    bool down = false;

    if (Input.Mouse.currentButtonState[button] == 1) down = true;

    // Map touches to mouse buttons checking
    if (Input.Touch.currentTouchState[button] == 1) down = true;

    return down;
}

// Detect if a mouse button has been released once
bool IsMouseButtonReleased(int button)
{
    bool released = false;

    if ((Input.Mouse.currentButtonState[button] == 0) && (Input.Mouse.previousButtonState[button] == 1)) released = true;

    // Map touches to mouse buttons checking
    if ((Input.Touch.currentTouchState[button] == 0) && (Input.Touch.previousTouchState[button] == 1)) released = true;

    return released;
}

// Detect if a mouse button is NOT being pressed
bool IsMouseButtonUp(int button)
{
    return !IsMouseButtonDown(button);
}

// Returns mouse position X
int GetMouseX(void)
{
#if defined(PLATFORM_ANDROID)
    return (int)Input.Touch.position[0].x;
#else
    return (int)((Input.Mouse.position.x + Input.Mouse.offset.x)*Input.Mouse.scale.x);
#endif
}

// Returns mouse position Y
int GetMouseY(void)
{
#if defined(PLATFORM_ANDROID)
    return (int)Input.Touch.position[0].y;
#else
    return (int)((Input.Mouse.position.y + Input.Mouse.offset.y)*Input.Mouse.scale.y);
#endif
}

// Returns mouse position XY
Vector2 GetMousePosition(void)
{
     Vector2 position;
     position.x=0;
     position.y=0;

#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    position = GetTouchPosition(0);
#else
    position.x = (Input.Mouse.position.x + Input.Mouse.offset.x)*Input.Mouse.scale.x;
    position.y = (Input.Mouse.position.y + Input.Mouse.offset.y)*Input.Mouse.scale.y;
#endif

    return position;
}

// Set mouse position XY
void SetMousePosition(int x, int y)
{
    Input.Mouse.position.x =x;
    Input.Mouse.position.y =y;


}

// Set mouse offset
// NOTE: Useful when rendering to different size targets
void SetMouseOffset(int offsetX, int offsetY)
{
    Input.Mouse.offset.x=offsetX;
    Input.Mouse.offset.y=offsetY;

}

// Set mouse scaling
// NOTE: Useful when rendering to different size targets
void SetMouseScale(float scaleX, float scaleY)
{
    Input.Mouse.scale.x =  scaleX;
    Input.Mouse.scale.y =  scaleY;

}

// Returns mouse wheel movement Y
float GetMouseWheelMove(void)
{
#if defined(PLATFORM_ANDROID)
    return 0.0f;
#elif defined(PLATFORM_WEB)
    return Input.Mouse.previousWheelMove/100.0f;
#else
    return Input.Mouse.previousWheelMove;
#endif
}


// Returns touch position X for touch point 0 (relative to screen size)
int GetTouchX(void)
{
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    return (int)Input.Touch.position[0].x;
#else   // PLATFORM_DESKTOP, PLATFORM_RPI, PLATFORM_DRM
    return GetMouseX();
#endif
}

// Returns touch position Y for touch point 0 (relative to screen size)
int GetTouchY(void)
{
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    return (int)Input.Touch.position[0].y;
#else   // PLATFORM_DESKTOP, PLATFORM_RPI, PLATFORM_DRM
    return GetMouseY();
#endif
}

Vector2 GetTouchPosition(int index)
{
    Vector2 position;
    position.x=-1;
    position.y=-1;

    if (index == 0) position = GetMousePosition();

    return position;
}



 void KeyCallback( int key, int scancode, int action, int mods)
{


  //  SDL_Log( "Key Callback: KEY:%i(%c) SDL KEY:%i(%c) - SCANCODE:%i (STATE:%i)", key, key,index,index, scancode, action);
      // int keycode =get_sdl_to_glfw(event->key.keysym.scancode);
       //if(core.key)  core.key(core.window,keycode,event->key.keysym.sym,GLFW_PRESS,event->key.keysym.mod);


        // WARNING: GLFW could return GLFW_REPEAT, we need to consider it as 1
        // to work properly with our implementation (IsKeyDown/IsKeyUp checks)
        if (action == 0) Input.Keyboard.currentKeyState[key] = 0;
        else Input.Keyboard.currentKeyState[key] = 1;


        // Check if there is space available in the key queue
        if ((Input.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE) && (action == 0))
        {
            // Add character to the queue
            Input.Keyboard.keyPressedQueue[Input.Keyboard.keyPressedQueueCount] = scancode;
            Input.Keyboard.keyPressedQueueCount++;
        }


}


 void CharCallback( unsigned int key)
{
    //SDL_Log( "Char Callback: KEY:%i(%c)", key, key);
    if (Input.Keyboard.charPressedQueueCount < MAX_KEY_PRESSED_QUEUE)
    {
        Input.Keyboard.charPressedQueue[Input.Keyboard.charPressedQueueCount] = key;
        Input.Keyboard.charPressedQueueCount++;
    }
}




 void MouseButtonCallback( int button, int action, int mods)
{

    Input.Mouse.currentButtonState[button] = action;

}


 void MouseCursorPosCallback( double x, double y)
{
    Input.Mouse.position.x = (float)x;
    Input.Mouse.position.y = (float)y;
    Input.Touch.position[0] = Input.Mouse.position;


}


#define BUFFER_SIZE 256
#define BUFFER_MASK (BUFFER_SIZE - 1)

static struct {
  event_t buffer[BUFFER_SIZE];
  unsigned writei, readi;
} events;



void event_push(event_t *e) {
  events.buffer[events.writei++ & BUFFER_MASK] = *e;
}

char* toLower(const char* s)
{
  for(char *p=s; *p; p++) *p=tolower(*p);
  return s;
}
char* toUpper(const char* s) {
  for(char *p=s; *p; p++) *p=toupper(*p);
  return s;
}

int event_poll(event_t *e)
{
  if (events.readi != events.writei)
  {
    *e = events.buffer[events.readi++ & BUFFER_MASK];
    return 1;
  }
  return 0;
}

int mouse_inited;
int mouse_x, mouse_y;
int mouse_lastX, mouse_lastY;

void LuaExecuteFile(const char *filename);

int l_event_pump(lua_State *L)
{

    Input.Keyboard.keyPressedQueueCount = 0;
    Input.Keyboard.charPressedQueueCount = 0;
    for (int i = 0; i < 512; i++) Input.Keyboard.previousKeyState[i] = Input.Keyboard.currentKeyState[i];
    for (int i = 0; i < 3; i++) Input.Mouse.previousButtonState[i] = Input.Mouse.currentButtonState[i];
    Input.Mouse.previousWheelMove = Input.Mouse.currentWheelMove;
    Input.Mouse.currentWheelMove = 0.0f;
    for (int i = 0; i < MAX_TOUCH_POINTS; i++) Input.Touch.previousTouchState[i] = Input.Touch.currentTouchState[i];


  SDL_Event e;
  while  (SDL_PollEvent( &e ) )
  {
    switch (e.type)
    {
         case  SDL_QUIT:
            {
                                 event_t me;
                                  me.type = EVENT_QUIT;
                                  me.quit.status = 1;
                                  event_push(&me);


                                     break;
         }

        case SDL_WINDOWEVENT:
                {
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                    {

                                   event_t me;
                                   me.type = EVENT_WINDOW_RESIZE;
                                   me.window.w=e.window.data1;
                                   me.window.h=e.window.data2;
                                   SDL_Log("main resize ");
                                   event_push(&me);
                  //  break;
                    }

                break;
                }
	case SDL_DROPFILE:
        {       char* dropped_filedir;
				dropped_filedir = e.drop.file;


				if (!strcmp("lua",GetFileExtension(dropped_filedir)))
				{
				// SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Lua file  dropped on window", dropped_filedir, gWindow);
				// LuaExecuteFile(dropped_filedir);
				}

				SDL_free(dropped_filedir);
				break;
				}
			case SDL_DROPTEXT:
			{
                char* dropped_filedir;
				dropped_filedir = e.drop.file;
				//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Text dropped on window", dropped_filedir, gWindow);
				SDL_free(dropped_filedir);
				break;
            }


              case SDL_KEYDOWN:
                       {

                                 KeyCallback(e.key.keysym.scancode,e.key.keysym.sym,1,e.key.keysym.mod);

                                  event_t me;
                                  me.type = EVENT_KEYBOARD_PRESSED;
                                  me.keyboard.key = SDL_GetScancodeName(e.key.keysym.scancode);
                                  me.keyboard.scancode =e.key.keysym.scancode;
                                  event_push(&me);

                          break;
                       }
              case SDL_KEYUP:
                       {


                                KeyCallback(e.key.keysym.scancode,e.key.keysym.sym,0,e.key.keysym.mod);
                                 event_t me;
                                  me.type = EVENT_KEYBOARD_RELEASED;
                                  me.keyboard.key = SDL_GetScancodeName(e.key.keysym.scancode);
                                  me.keyboard.scancode =e.key.keysym.scancode;
                                  event_push(&me);

                          break;
                       }
              case SDL_MOUSEBUTTONDOWN:
                {

                         MouseButtonCallback(e.button.button,1,0);
                         MouseCursorPosCallback(e.button.x,e.button.y);
                    event_t me;
                    me.type =  EVENT_MOUSE_PRESSED ;
                    me.mouse.x = e.button.x;
                    me.mouse.y = e.button.y;
                    me.mouse.button = e.button.button;
                    event_push(&me);

               break;
              }
              case SDL_MOUSEBUTTONUP:
              {

                         MouseButtonCallback(e.button.button,0,0);
                         MouseCursorPosCallback(e.button.x,e.button.y);

                    event_t me;
                    me.type =   EVENT_MOUSE_RELEASED;
                    me.mouse.x = e.button.x;
                    me.mouse.y = e.button.y;
                    me.mouse.button = e.button.button;
                    event_push(&me);
                    break;
              }
              case SDL_MOUSEMOTION:
                        {



                            MouseCursorPosCallback(e.button.x,e.button.y);


                            mouse_x = e.button.x;
                            mouse_y = e.button.y;
                            event_t me;
                            me.type =   EVENT_MOUSE_MOVED;
                            me.mouse.button = e.button.button;
                            me.mouse.x = mouse_x;
                            me.mouse.y = mouse_y;
                            me.mouse.dx = mouse_x - mouse_lastX;
                            me.mouse.dy = mouse_y - mouse_lastY;
                            event_push(&me);
                            mouse_lastX = mouse_x;
                            mouse_lastY = mouse_y;

                                    break;

                        }
    }
}
}




const char* event_typestr(int type) {
  switch (type)
  {
    case EVENT_QUIT               : return "quit";
    case EVENT_WINDOW_RESIZE      : return "windowresize";
    case EVENT_KEYBOARD_PRESSED   : return "keypressed";
    case EVENT_KEYBOARD_RELEASED  : return "keyreleased";
    case EVENT_KEYBOARD_TEXTINPUT : return "textinput";
    case EVENT_MOUSE_MOVED        : return "mousemoved";
    case EVENT_MOUSE_PRESSED      : return "mousepressed";
    case EVENT_MOUSE_RELEASED     : return "mousereleased";
  }
  return "none";
}

int l_event_poll(lua_State *L)
{


  event_t e;
  if (event_poll(&e))
  {
    lua_pushstring(L, event_typestr(e.type));


    switch(e.type)
    {
      case EVENT_QUIT:
        lua_pushnumber(L, 1);
        return 2;

       case EVENT_WINDOW_RESIZE:
        lua_pushnumber(L, e.window.w);
        lua_pushnumber(L, e.window.h);

        return 3;
      case EVENT_KEYBOARD_PRESSED:
      case EVENT_KEYBOARD_RELEASED:
        lua_pushstring(L, e.keyboard.key);
        lua_pushnumber(L, e.keyboard.scancode);
        return 3;

      case EVENT_KEYBOARD_TEXTINPUT:
        lua_pushstring(L, e.keyboard.text);
        return 2;

      case EVENT_MOUSE_MOVED:
        lua_pushnumber(L, e.mouse.x);
        lua_pushnumber(L, e.mouse.y);
        lua_pushnumber(L, e.mouse.dx);
        lua_pushnumber(L, e.mouse.dy);
        return 5;

      case EVENT_MOUSE_PRESSED:
      case EVENT_MOUSE_RELEASED:
        lua_pushnumber(L, e.mouse.x);
        lua_pushnumber(L, e.mouse.y);
        lua_pushnumber(L, e.mouse.button);
        return 4;

    }

    return 1;
  }
  return 0;
}


int l_event_quit(lua_State *L)
{
  int status = luaL_optnumber(L, 1, 0);
  event_t e;
  e.type = EVENT_QUIT;
  e.quit.status = status;
  event_push(&e);
  return 0;
}



int luaopen_event(lua_State *L) {
  luaL_Reg reg[] = {
    { "pump",  l_event_pump  },
    { "poll",  l_event_poll  },
    { "quit",  l_event_quit  },
    { 0, 0 },
  };
  luaL_newlib(L, reg);


    Input.Mouse.scale.x=1;
    Input.Mouse.scale.y=1;

    Input.Mouse.offset.x=1;
    Input.Mouse.offset.y=1;

    Input.Mouse.position.x = 0;
    Input.Mouse.position.y = 0;


  return 1;
}
