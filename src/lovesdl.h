#ifndef LOVESDL_H
#define LOVESDL_H

#include "dmt.h"
#include "luaobj.h"
#include <SDL2/SDL.h>
#include "stb_truetype.h"
typedef unsigned char pixel_t;


#define TRACELOG(level, ...) SDL_Log(__VA_ARGS__)
#define TRACELOGD(...) SDL_Log(__VA_ARGS__)



#define DMTMALLOC(sz)       dmt_malloc(sz)
#define DMTCALLOC(n,sz)     dmt_calloc(n,sz)
#define DMTREALLOC(ptr,sz)  dmt_realloc(ptr,sz)
#define DMTFREE(ptr)        dmt_free(ptr)

#define RL_MALLOC(sz)       SDL_malloc(sz)
#define RL_CALLOC(n,sz)     SDL_calloc(n,sz)
#define RL_REALLOC(n,sz)    SDL_realloc(n,sz)
#define RL_FREE(p)          SDL_free(p)


#if defined(__STDC__) && __STDC_VERSION__ >= 199901L
    #include <stdbool.h>
#elif !defined(__cplusplus) && !defined(bool)
    typedef enum { false, true } bool;
#endif

#define ASSERT(x)\
  do {\
    if (!(x)) {\
      SDL_Log("%s:%d: %s(): assertion '%s' failed\n",\
              __FILE__, __LINE__, __func__, #x);\
      abort();\
    }\
  } while (0)

#define UNUSED(x)       ((void) (x))
#define MIN(a, b)       ((b) < (a) ? (b) : (a))
#define MAX(a, b)       ((b) > (a) ? (b) : (a))
#define CLAMP(x, a, b)  (MAX(a, MIN(x, b)))
#define LERP(a, b, p)   ((a) + ((b) - (a)) * (p))


// Rectangle type
typedef struct Rectangle {
    float x;
    float y;
    float width;
    float height;
} Rectangle;

typedef struct Vector2 {
    float x;
    float y;

} Vector2;


typedef struct Vector3 {
    float x;
    float y;
    float z;

} Vector3;



typedef struct Vector4 {
    float x;
    float y;
    float z;
    float w;

} Vector4;


    // Texture formats (support depends on OpenGL version)
    typedef enum {
        PIXELFORMAT_GRAYSCALE = 1,     // 8 bit per pixel (no alpha)
        PIXELFORMAT_GRAY_ALPHA,
        PIXELFORMAT_R5G6B5,            // 16 bpp
        PIXELFORMAT_R8G8B8,            // 24 bpp
        PIXELFORMAT_R5G5B5A1,          // 16 bpp (1 bit alpha)
        PIXELFORMAT_R4G4B4A4,          // 16 bpp (4 bit alpha)
        PIXELFORMAT_R8G8B8A8,          // 32 bpp
        PIXELFORMAT_R32,               // 32 bpp (1 channel - float)
        PIXELFORMAT_R32G32B32,         // 32*3 bpp (3 channels - float)
        PIXELFORMAT_R32G32B32A32,      // 32*4 bpp (4 channels - float)
    } PixelFormat;

 typedef struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;
    } Color;
// Image type, bpp always RGBA (32bit)
// NOTE: Data stored in CPU memory (RAM)
typedef struct Image {
    void *data;             // Image raw data
    int width;              // Image base width
    int height;             // Image base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Image;

typedef struct Texture {
    SDL_Texture* id;        // OpenGL texture id
    int width;              // Texture base width
    int height;             // Texture base height
    int mipmaps;            // Mipmap levels, 1 by default
    int format;             // Data format (PixelFormat type)
} Texture;

typedef Texture Texture2D;

// Font character info
typedef struct CharInfo {
    int value;              // Character value (Unicode)
    int offsetX;            // Character offset X when drawing
    int offsetY;            // Character offset Y when drawing
    int advanceX;           // Character advance position X
    Image image;            // Character image data
} CharInfo;

// Font type, includes texture and charSet array data
typedef struct Font {
    int baseSize;           // Base size (default chars height)
    int charsCount;         // Number of characters
    int charsPadding;       // Padding around the chars
    Texture texture;      // Characters texture atlas
    Rectangle *recs;        // Characters rectangles in texture
    CharInfo *chars;        // Characters info data
} Font;

// Font type, defines generation method
typedef enum {
    FONT_DEFAULT = 0,       // Default font generation, anti-aliased
    FONT_BITMAP,            // Bitmap font generation, no anti-aliasing
    FONT_SDF                // SDF font generation, requires external shader
} FontType;


#define MAX_TEXT_BUFFER_LENGTH              1024        // Size of internal static buffers used on some functions:
#define MAX_TEXT_UNICODE_CHARS               512        // Maximum number of unicode codepoints: GetCodepoints()
#define MAX_TEXTSPLIT_COUNT                  128        // Maximum number of substrings to split: TextSplit()


#if defined(__cplusplus)
    #define CLITERAL(type)      type
#else
    #define CLITERAL(type)      (type)
#endif

// Some Basic Colors
// NOTE: Custom raylib color palette for amazing visuals on WHITE background
#define LIGHTGRAY  CLITERAL(Color){ 200, 200, 200, 255 }   // Light Gray
#define GRAY       CLITERAL(Color){ 130, 130, 130, 255 }   // Gray
#define DARKGRAY   CLITERAL(Color){ 80, 80, 80, 255 }      // Dark Gray
#define YELLOW     CLITERAL(Color){ 253, 249, 0, 255 }     // Yellow
#define GOLD       CLITERAL(Color){ 255, 203, 0, 255 }     // Gold
#define ORANGE     CLITERAL(Color){ 255, 161, 0, 255 }     // Orange
#define PINK       CLITERAL(Color){ 255, 109, 194, 255 }   // Pink
#define RED        CLITERAL(Color){ 230, 41, 55, 255 }     // Red
#define MAROON     CLITERAL(Color){ 190, 33, 55, 255 }     // Maroon
#define GREEN      CLITERAL(Color){ 0, 228, 48, 255 }      // Green
#define LIME       CLITERAL(Color){ 0, 158, 47, 255 }      // Lime
#define DARKGREEN  CLITERAL(Color){ 0, 117, 44, 255 }      // Dark Green
#define SKYBLUE    CLITERAL(Color){ 102, 191, 255, 255 }   // Sky Blue
#define BLUE       CLITERAL(Color){ 0, 121, 241, 255 }     // Blue
#define DARKBLUE   CLITERAL(Color){ 0, 82, 172, 255 }      // Dark Blue
#define PURPLE     CLITERAL(Color){ 200, 122, 255, 255 }   // Purple
#define VIOLET     CLITERAL(Color){ 135, 60, 190, 255 }    // Violet
#define DARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }    // Dark Purple
#define BEIGE      CLITERAL(Color){ 211, 176, 131, 255 }   // Beige
#define BROWN      CLITERAL(Color){ 127, 106, 79, 255 }    // Brown
#define DARKBROWN  CLITERAL(Color){ 76, 63, 47, 255 }      // Dark Brown

#define WHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
#define BLACK      CLITERAL(Color){ 0, 0, 0, 255 }         // Black
#define BLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define MAGENTA    CLITERAL(Color){ 255, 0, 255, 255 }     // Magenta
#define RAYWHITE   CLITERAL(Color){ 245, 245, 245, 255 }   // My own White (raylib logo)



enum {
  NONE,
  BLEND,
  ADD,
  MOD,
} BLEND_MODE;


typedef struct {
  SDL_Texture* tex;
  SDL_Surface* surface;
  int width, height;
} image_t;

typedef struct {
  Font font;
  int isDefault;
  int size;
} font_t;

typedef struct {
  lua_Number x, y;
  lua_Number width, height;
} quad_l;


int VGA_HEIGHT;
int VGA_WIDTH;



 int GetRandomValue(int min, int max);                       // Returns a random value between min and max (both included)

// Files management functions
 unsigned char *LoadFileData(const char *fileName, unsigned int *bytesRead);     // Load file data as byte array (read)
 void UnloadFileData(unsigned char *data);                   // Unload file data allocated by LoadFileData()
 bool SaveFileData(const char *fileName, void *data, unsigned int bytesToWrite); // Save data to file from byte array (write), returns true on success
 char *LoadFileText(const char *fileName);                   // Load text data from file (read), returns a '\0' terminated string
 void UnloadFileText(unsigned char *text);                   // Unload file text data allocated by LoadFileText()
 bool SaveFileText(const char *fileName, char *text);        // Save text data to file (write), string must be '\0' terminated, returns true on success
 bool FileExists(const char *fileName);                      // Check if file exists
 bool DirectoryExists(const char *dirPath);                  // Check if a directory path exists
 bool IsFileExtension(const char *fileName, const char *ext);// Check file extension (including point: .png, .wav)
 const char *GetFileExtension(const char *fileName);         // Get pointer to extension for a filename string (including point: ".png")
 const char *GetFileName(const char *filePath);              // Get pointer to filename for a path string
 const char *GetFileNameWithoutExt(const char *filePath);    // Get filename string without extension (uses static string)
 const char *GetDirectoryPath(const char *filePath);         // Get full path for a given fileName with path (uses static string)
 const char *GetPrevDirectoryPath(const char *dirPath);      // Get previous directory path for a given path (uses static string)
 const char *GetApplicationDirectory(void);
 const char *GetWorkingDirectory(void);                      // Get current working directory (uses static string)
 char **GetDirectoryFiles(const char *dirPath, int *count);  // Get filenames in a directory path (memory should be freed)
 void ClearDirectoryFiles(void);                             // Clear directory files paths buffers (free memory)
 bool ChangeDirectory(const char *dir);                      // Change working directory, return true on success
 long GetFileModTime(const char *fileName);                  // Get file modification time (last write time)
long GetFileSize(const char *fileName);

// Persistent storage management
 bool SaveStorageValue(unsigned int position, int value);    // Save integer value to storage file (to defined position), returns true on success
 int LoadStorageValue(unsigned int position);                // Load integer value from storage file (from defined position)

 void OpenURL(const char *url);                              // Open URL with default system browser (if available)

 bool IsAnyKeyPressed();
 bool IsKeyPressed(int key);                             // Detect if a key has been pressed once
 bool IsKeyDown(int key);                                // Detect if a key is being pressed
 bool IsKeyReleased(int key);                            // Detect if a key has been released once
 bool IsKeyUp(int key);                                  // Detect if a key is NOT being pressed
 int GetKeyPressed(void);                                // Get key pressed (keycode), call it multiple times for keys queued
 int GetCharPressed(void);                               // Get char pressed (unicode), call it multiple times for chars queued
bool IsAnyKeyPressed();

 bool IsMouseButtonPressed(int button);                  // Detect if a mouse button has been pressed once
 bool IsMouseButtonDown(int button);                     // Detect if a mouse button is being pressed
 bool IsMouseButtonReleased(int button);                 // Detect if a mouse button has been released once
 bool IsMouseButtonUp(int button);                       // Detect if a mouse button is NOT being pressed
 int GetMouseX(void);                                    // Returns mouse position X
 int GetMouseY(void);                                    // Returns mouse position Y
 Vector2 GetMousePosition(void);                         // Returns mouse position XY
 void SetMousePosition(int x, int y);                    // Set mouse position XY
 void SetMouseOffset(int offsetX, int offsetY);          // Set mouse offset
 void SetMouseScale(float scaleX, float scaleY);         // Set mouse scaling
 float GetMouseWheelMove(void);                          // Returns mouse wheel movement Y
 int GetMouseCursor(void);                               // Returns mouse cursor if (MouseCursor enum)
 void SetMouseCursor(int cursor);                        // Set mouse cursor


// Text strings management functions (no utf8 strings, only byte chars)
// NOTE: Some strings allocate memory internally for returned strings, just be careful!
 int TextCopy(char *dst, const char *src);                                             // Copy one string to another, returns bytes copied
 bool TextIsEqual(const char *text1, const char *text2);                               // Check if two text string are equal
 unsigned int TextLength(const char *text);                                            // Get text length, checks for '\0' ending
 const char *TextFormat(const char *text, ...);                                        // Text formatting with variables (sprintf style)
 const char *TextSubtext(const char *text, int position, int length);                  // Get a piece of a text string
 char *TextReplace(char *text, const char *replace, const char *by);                   // Replace text string (memory must be freed!)
 char *TextInsert(const char *text, const char *insert, int position);                 // Insert text in a position (memory must be freed!)
 const char *TextJoin(const char **textList, int count, const char *delimiter);        // Join text strings with delimiter
 const char **TextSplit(const char *text, char delimiter, int *count);                 // Split text into multiple strings
 void TextAppend(char *text, const char *append, int *position);                       // Append text at specific position and move cursor!
 int TextFindIndex(const char *text, const char *find);                                // Find first text occurrence within a string
 const char *TextToUpper(const char *text);                      // Get upper case version of provided string
 const char *TextToLower(const char *text);                      // Get lower case version of provided string
 const char *TextToPascal(const char *text);                     // Get Pascal case notation version of provided string
 int TextToInteger(const char *text);                            // Get integer value from text (negative values not supported)
char *TextToUtf8(int *codepoints, int length);                  // Encode text codepoint into utf8 text (memory must be freed!)

// UTF8 text strings management functions
 int *GetCodepoints(const char *text, int *count);               // Get all codepoints in a string, codepoints count returned by parameters
 int GetCodepointsCount(const char *text);                       // Get total number of characters (codepoints) in a UTF8 encoded string
 int GetNextCodepoint(const char *text, int *bytesProcessed);    // Returns next codepoint in a UTF8 encoded string; 0x3f('?') is returned on failure
 const char *CodepointToUtf8(int codepoint, int *byteLength);    // Encode codepoint into utf8 text (char array length returned as parameter)
 const char *CodepointToUTF8(int codepoint, int *byteSize);
int GetCodepoint(const char *text, int *bytesProcessed);

void DrawRectangle(int x, int y, int w, int h, Color c);
void DrawRectangleGradientV(int posX, int posY, int width, int height, Color color1, Color color2);
void DrawRectangleGradientEx(Rectangle rec, Color col1, Color col2, Color col3, Color col4);
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color c);
void DrawRectangleLines(int x, int y, int w, int h, Color c);
void DrawRectangleLinesEx(Rectangle rec, int lineThick, Color color);
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);
void DrawLine(int x, int y, int x2,int y2, Color c);
void DrawPoint(int x, int y, Color c);
int DrawCircle( Sint16 x, Sint16 y, Sint16 rad, Color color);
int DrawFilledCircle(Sint16 x, Sint16 y, Sint16 rad, Color color);

SDL_Surface* LoadSurfaceFromFile(const char* filename);
SDL_Texture* LoadTextureFromSurface(SDL_Surface* surfe,bool discarSurface);
bool ExportSurface(const SDL_Surface* image, const char *fileName);
void SurfaceDrawPixel(SDL_Surface *dst, int x, int y, Color color);
void SurfaceClearBackground(SDL_Surface *dst, Color color);
Color *LoadSurfaceColors(const SDL_Surface* image);
void SurfaceColorReplace(SDL_Surface *image, Color color, Color replace);
void SurfaceAlphaClear(SDL_Surface *image, Color color, float threshold);
//SDL_Surface* SurfaceFromSurface(const SDL_Surface* image, Rectangle rec);
int GetSurfacePixelDataSize(int width, int height, int channels);


Image LoadImage(const char *fileName);
Image LoadImageFromFile(const char* filename);
Color *LoadImageColors(Image image);
Image LoadImageFromMemory( const unsigned char *fileData, int dataSize);
Image ImageFromImage(Image image, Rectangle rec);
void UnloadImage(Image image);
void ImageColorReplace(Image *image, Color color, Color replace);
void ImageAlphaMask(Image *image, Image alphaMask);
void ImageAlphaClear(Image *image, Color color, float threshold);
Image ImageCopy(Image image);
void ImageFormat(Image *image, int newFormat);
bool ExportImage(Image image, const char *fileName);   ;
void ImageClearBackground(Image *dst, Color color);
void ImageDrawPixel(Image *dst, int x, int y, Color color);
void ResizeSurfaceCanvas(SDL_Surface *image, int newWidth, int newHeight, int offsetX, int offsetY, Color fill);

Color GetColor(int hexValue);


// Color/pixel related functions
 Color Fade(Color color, float alpha);                                 // Returns color with alpha applied, alpha goes from 0.0f to 1.0f
 int ColorToInt(Color color);                                          // Returns hexadecimal value for a Color
 Vector4 ColorNormalize(Color color);                                  // Returns Color normalized as float [0..1]
 Color ColorFromNormalized(Vector4 normalized);                        // Returns Color from normalized values [0..1]
 Vector3 ColorToHSV(Color color);                                      // Returns HSV values for a Color
 Color ColorFromHSV(float hue, float saturation, float value);         // Returns a Color from HSV values
 Color ColorAlpha(Color color, float alpha);                           // Returns color with alpha applied, alpha goes from 0.0f to 1.0f
 Color ColorAlphaBlend(Color dst, Color src, Color tint);              // Returns src alpha-blended into dst color with tint
 Color GetColor(int hexValue);                                         // Get Color structure from hexadecimal value
 Color GetPixelColor(void *srcPtr, int format);                        // Get Color from a source pixel pointer of certain format

int GetPixelDataSize(int width, int height, int format);
void SetPixelColor(void *dstPtr, Color color, int format);


bool ExportImageAsCode(Image image, const char *fileName);
Texture2D LoadTextureFromImage(Image image);
void UnloadTexture(Texture2D texture);

void LoadFontDefault();
void UnloadFontDefault(void);

Font GetFontDefault(void);                                                            // Get the default Font
Font LoadFont(const char *fileName);                                                  // Load font from file into GPU memory (VRAM)
Font LoadFontEx(const char *fileName, int fontSize, int *fontChars, int charsCount);  // Load font from file with extended parameters
Font LoadFontFromImage(Image image, Color key, int firstChar);                        // Load font from Image (XNA style)
Font LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int charsCount); // Load font from memory buffer, fileType refers to extension: i.e. "ttf"
CharInfo *LoadFontData(const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int charsCount, int type);      // Load font data for further use
Image GenImageFontAtlas(const CharInfo *chars, Rectangle **recs, int charsCount, int fontSize, int padding, int packMethod);      // Generate image font atlas using chars info
void UnloadFontData(CharInfo *chars, int charsCount);                                 // Unload font chars info data (RAM)
void UnloadFont(Font font);                                                           // Unload Font from GPU memory (VRAM)
Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing);

int GetGlyphIndex(Font font, int codepoint);
void DrawFPS(int posX, int posY);
void DrawTextCodepoint(Font font, int codepoint, Vector2 position, float fontSize, Color tint);
void DrawText(const char *text, int posX, int posY, int fontSize, Color color);       // Draw text (using default font)
 void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);                // Draw text using font and additional parameters
 void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);   // Draw text using font inside rectangle limits
 void DrawTextRecEx(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint,
                         int selectStart, int selectLength, Color selectTint, Color selectBackTint);    // Draw text using font inside rectangle limits with support for text selection


int pixelRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Horizontal line */


int hlineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Vertical line */


int vlineRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Rectangle */


int rectangleRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1,
Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Rounded-Corner Rectangle */


int roundedRectangleRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1,
Sint16 x2, Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled rectangle (Box) */


int boxRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2,
Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Rounded-Corner Filled rectangle (Box) */


int roundedBoxRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2,
Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Line */

int lineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1,
Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Line */

int aalineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1,
Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Thick Line */

int thickLineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2,
Uint8 width, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Circle */


int circleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Arc */


int arcRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end,
Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Circle */


int aacircleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y,
Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Circle */


int filledCircleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y,
Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Ellipse */


int ellipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y,
Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA Ellipse */


int aaellipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y,
Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Ellipse */

int filledEllipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y,
Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Pie */

int pieRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad,
Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Pie */


int filledPieRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad,
Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Trigon */


int trigonRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* AA-Trigon */


int aatrigonRGBA(SDL_Renderer * renderer,  Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Filled Trigon */


int filledTrigonRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
Uint8 r, Uint8 g, Uint8 b, Uint8 a);

/* Polygon */


int polygonRGBA(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy,int n);

/* AA-Polygon */


int aapolygonRGBA(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy,
int n);

/* Filled Polygon */


int filledPolygonRGBA(SDL_Renderer * renderer, const Sint16 * vx,
const Sint16 * vy, int n);

/* Textured Polygon */

int texturedPolygon(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy, int n, SDL_Surface * texture,int texture_dx,int texture_dy);

/* Bezier */

int bezierRGBA(SDL_Renderer * renderer, const double *x,const double *y, int n, int s);




int SDL_CollideTransparentPixel(SDL_Surface *surface , int u , int v);

int SDL_CollidePixel(SDL_Surface *as , int ax , int ay ,
                       SDL_Surface *bs , int bx , int by, int skip );

int SDL_CollideBoundingBox(SDL_Surface *sa , int ax , int ay ,
                             SDL_Surface *sb , int bx , int by);

int SDL_CollideBox(SDL_Rect a , SDL_Rect b);

int SDL_CollideCircle(int x1 , int y1 , int r1 ,
                                int x2 , int y2 , int r2 , int offset);

int SDL_CollideBoundingCircle(SDL_Surface *a , int x1 , int y1 ,
                                SDL_Surface *b , int x2 , int y2 ,
                                int offset);

int RenderScaleFlip(
	SDL_Renderer* renderer, SDL_Texture* t,
	int x, int y, int w, int h, double angle, double sx, double sy, SDL_RendererFlip flip);
#endif
