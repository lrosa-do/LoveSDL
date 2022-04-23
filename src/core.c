#include <stdlib.h>                 // Required for: srand(), rand(), atexit()
#include <stdio.h>                  // Required for: sprintf() [Used in OpenURL()]
#include <string.h>                 // Required for: strrchr(), strcmp(), strlen()
#include <time.h>                   // Required for: time() [Used in InitTimer()]
#include <math.h>                   // Required for: tan() [Used in BeginMode3D()]

#include <sys/stat.h>               // Required for: stat() [Used in GetFileModTime()]
#include "dmt.h"




#include <dirent.h>             // Required for: DIR, opendir(), closedir() [Used in GetDirectoryFiles()]






#if defined(_WIN32)
    #include <direct.h>             // Required for: _getch(), _chdir()
    #define GETCWD _getcwd          // NOTE: MSDN recommends not to use getcwd(), chdir()
    #define CHDIR _chdir
    #include <io.h>                 // Required for _access() [Used in FileExists()]
#else
    #include <unistd.h>             // Required for: getch(), chdir() (POSIX), access()
    #define GETCWD getcwd
    #define CHDIR chdir
#endif


#if (defined(__linux__) || defined(PLATFORM_WEB)) && _POSIX_C_SOURCE < 199309L
    #undef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 199309L // Required for CLOCK_MONOTONIC if compiled with c99 without gnu ext.
#endif

#if defined(__linux__)
    #define MAX_FILEPATH_LENGTH     4096        // Maximum length for filepaths (Linux PATH_MAX default value)
#else
    #define MAX_FILEPATH_LENGTH      512        // Maximum length supported for filepaths
#endif

    #define MAX_TEXT_BUFFER_LENGTH              1024        // Size of internal static buffers used on some functions:
                                                            // TextFormat(), TextSubtext(), TextToUpper(), TextToLower(), TextToPascal(), TextSplit()
    #define MAX_TEXT_UNICODE_CHARS               512        // Maximum number of unicode codepoints: GetCodepoints()
    #define MAX_TEXTSPLIT_COUNT                  128        // Maximum number of substrings to split: TextSplit()


#include "luaobj.h"
#include "lovesdl.h"
#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"     // Required for: ttf font rectangles packaging
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"      // Required for: ttf font data reading
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"   // Required for: raystbi_write_*()



#define PIXELFORMAT_R5G5B5A1_ALPHA_THRESHOLD  50
extern SDL_Window* gWindow ;
extern SDL_Renderer* gRenderer;



// Check if the file exists
bool FileExists(const char *fileName)
{
    bool result = false;

#if defined(_WIN32)
    if (_access(fileName, 0) != -1) result = true;
#else
    if (access(fileName, F_OK) != -1) result = true;
#endif

    return result;
}

// Check file extension
// NOTE: Extensions checking is not case-sensitive
bool IsFileExtension(const char *fileName, const char *ext)
{
    bool result = false;
    const char *fileExt = GetFileExtension(fileName);

    if (fileExt != NULL)
    {

        int extCount = 0;
        const char **checkExts = TextSplit(ext, ';', &extCount);

        char fileExtLower[16] = { 0 };
        strcpy(fileExtLower, TextToLower(fileExt));

        for (int i = 0; i < extCount; i++)
        {
            if (TextIsEqual(fileExtLower, TextToLower(checkExts[i] + 1)))
            {
                result = true;
                break;
            }
        }
    }

    return result;
}

// Check if a directory path exists
bool DirectoryExists(const char *dirPath)
{
    bool result = false;
    DIR *dir = opendir(dirPath);

    if (dir != NULL)
    {
        result = true;
        closedir(dir);
    }

    return result;
}

// Get pointer to extension for a filename string
const char *GetFileExtension(const char *fileName)
{
    const char *dot = strrchr(fileName, '.');

    if (!dot || dot == fileName) return NULL;

    return (dot + 1);
}

// String pointer reverse break: returns right-most occurrence of charset in s
static const char *strprbrk(const char *s, const char *charset)
{
    const char *latestMatch = NULL;
    for (; s = strpbrk(s, charset), s != NULL; latestMatch = s++) { }
    return latestMatch;
}

// Get pointer to filename for a path string
const char *GetFileName(const char *filePath)
{
    const char *fileName = NULL;
    if (filePath != NULL) fileName = strprbrk(filePath, "\\/");

    if (!fileName) return filePath;

    return fileName + 1;
}

// Get filename string without extension (uses static string)
const char *GetFileNameWithoutExt(const char *filePath)
{
    #define MAX_FILENAMEWITHOUTEXT_LENGTH   128

    static char fileName[MAX_FILENAMEWITHOUTEXT_LENGTH];
    memset(fileName, 0, MAX_FILENAMEWITHOUTEXT_LENGTH);

    if (filePath != NULL) strcpy(fileName, GetFileName(filePath));   // Get filename with extension

    int len = (int)strlen(fileName);

    for (int i = 0; (i < len) && (i < MAX_FILENAMEWITHOUTEXT_LENGTH); i++)
    {
        if (fileName[i] == '.')
        {
            // NOTE: We break on first '.' found
            fileName[i] = '\0';
            break;
        }
    }

    return fileName;
}

// Get directory for a given filePath
const char *GetDirectoryPath(const char *filePath)
{
    // NOTE: Directory separator is different in Windows and other platforms,
    // fortunately, Windows also support the '/' separator, that's the one should be used
    #if defined(_WIN32)
        char separator = '\\';
    #else
        char separator = '/';
    #endif

    const char *lastSlash = NULL;
    static char dirPath[MAX_FILEPATH_LENGTH];
    memset(dirPath, 0, MAX_FILEPATH_LENGTH);

    // In case provided path does not contain a root drive letter (C:\, D:\) nor leading path separator (\, /),
    // we add the current directory path to dirPath
    if (filePath[1] != ':' && filePath[0] != '\\' && filePath[0] != '/')
    {
        // For security, we set starting path to current directory,
        // obtained path will be concated to this
        dirPath[0] = '.';
        dirPath[1] = '/';
    }

    lastSlash = strprbrk(filePath, "\\/");
    if (lastSlash)
    {
        if (lastSlash == filePath)
        {
            // The last and only slash is the leading one: path is in a root directory
            dirPath[0] = filePath[0];
            dirPath[1] = '\0';
        }
        else
        {
            // NOTE: Be careful, strncpy() is not safe, it does not care about '\0'
            memcpy(dirPath + (filePath[1] != ':' && filePath[0] != '\\' && filePath[0] != '/' ? 2 : 0), filePath, strlen(filePath) - (strlen(lastSlash) - 1));
            dirPath[strlen(filePath) - strlen(lastSlash) + (filePath[1] != ':' && filePath[0] != '\\' && filePath[0] != '/' ? 2 : 0)] = '\0';  // Add '\0' manually
        }
    }

    return dirPath;
}

// Get previous directory path for a given path
const char *GetPrevDirectoryPath(const char *dirPath)
{
    static char prevDirPath[MAX_FILEPATH_LENGTH];
    memset(prevDirPath, 0, MAX_FILEPATH_LENGTH);
    int pathLen = (int)strlen(dirPath);

    if (pathLen <= 3) strcpy(prevDirPath, dirPath);

    for (int i = (pathLen - 1); (i >= 0) && (pathLen > 3); i--)
    {
        if ((dirPath[i] == '\\') || (dirPath[i] == '/'))
        {
            // Check for root: "C:\" or "/"
            if (((i == 2) && (dirPath[1] ==':')) || (i == 0)) i++;

            strncpy(prevDirPath, dirPath, i);
            break;
        }
    }

    return prevDirPath;
}



const char *GetApplicationDirectory(void)
{
    static char appDir[512] = { 0 };
    memset(appDir, 0, 512);

#if defined(_WIN32)
    int len = 0;
#if defined (UNICODE)
    unsigned short widePath[MAX_PATH];
    len = GetModuleFileNameW(NULL, widePath, MAX_PATH);
    len = WideCharToMultiByte(0, 0, widePath, len, appDir, MAX_PATH, NULL, NULL);
#else
    len = GetModuleFileNameA(NULL, appDir, MAX_PATH);
#endif
    if (len > 0)
    {
        for (int i = len; i >= 0; --i)
        {
            if (appDir[i] == '\\')
            {
                appDir[i + 1] = '\0';
                break;
            }
        }
    }
    else
    {
        appDir[0] = '.';
        appDir[1] = '\\';
    }

#elif defined(__linux__)
    unsigned int size = sizeof(appDir);
    ssize_t len = readlink("/proc/self/exe", appDir, size);

    if (len > 0)
    {
        for (int i = len; i >= 0; --i)
        {
            if (appDir[i] == '/')
            {
                appDir[i + 1] = '\0';
                break;
            }
        }
    }
    else
    {
        appDir[0] = '.';
        appDir[1] = '/';
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(appDir);

    if (_NSGetExecutablePath(appDir, &size) == 0)
    {
        int len = strlen(appDir);
        for (int i = len; i >= 0; --i)
        {
            if (appDir[i] == '/')
            {
                appDir[i + 1] = '\0';
                break;
            }
        }
    }
    else
    {
        appDir[0] = '.';
        appDir[1] = '/';
    }
#endif

    return appDir;
    }

// Get current working directory
const char *GetWorkingDirectory(void)
{
    static char currentDir[MAX_FILEPATH_LENGTH];
    memset(currentDir, 0, MAX_FILEPATH_LENGTH);

    char *ptr = GETCWD(currentDir, MAX_FILEPATH_LENGTH - 1);

    return ptr;
}

static char **dirFilesPath = NULL;          // Store directory files paths as strings
static int dirFilesCount = 0;               // Count directory files strings


// Get filenames in a directory path (max 512 files)
// NOTE: Files count is returned by parameters pointer
char **GetDirectoryFiles(const char *dirPath, int *fileCount)
{
    #define MAX_DIRECTORY_FILES     512

    ClearDirectoryFiles();

    // Memory allocation for MAX_DIRECTORY_FILES
    dirFilesPath = (char **)RL_MALLOC(sizeof(char *)*MAX_DIRECTORY_FILES);

    for (int i = 0; i < MAX_DIRECTORY_FILES; i++) dirFilesPath[i] = (char *)RL_MALLOC(sizeof(char)*MAX_FILEPATH_LENGTH);

    int counter = 0;
    struct dirent *entity;
    DIR *dir = opendir(dirPath);

    if (dir != NULL)  // It's a directory
    {
        // TODO: Reading could be done in two passes,
        // first one to count files and second one to read names
        // That way we can allocate required memory, instead of a limited pool

        while ((entity = readdir(dir)) != NULL)
        {
            strcpy(dirFilesPath[counter], entity->d_name);
            counter++;
        }

        closedir(dir);
    }
    else SDL_Log( "FILEIO: Failed to open requested directory");  // Maybe it's a file...

    dirFilesCount = counter;
    *fileCount = dirFilesCount;

    return dirFilesPath;
}

// Clear directory files paths buffers
void ClearDirectoryFiles(void)
{
    if (dirFilesCount > 0)
    {
        for (int i = 0; i < MAX_DIRECTORY_FILES; i++) RL_FREE(dirFilesPath[i]);

        RL_FREE(dirFilesPath);
    }

    dirFilesCount = 0;
}

// Change working directory, returns true on success
bool ChangeDirectory(const char *dir)
{
    bool result = CHDIR(dir);

    if (result != 0) SDL_Log( "SYSTEM: Failed to change to directory: %s", dir);

    return (result == 0);
}


// Load data from file into a buffer
unsigned char *LoadFileData(const char *fileName, unsigned int *bytesRead)
{
   unsigned char *data = NULL;
    *bytesRead = 0;

    if (fileName != NULL)
    {
        SDL_RWops* file= SDL_RWFromFile(fileName, "rb");








        if (file != NULL)
        {
            int size =(int) SDL_RWsize(file);

            if (size > 0)
            {
                data = (unsigned char *)RL_MALLOC(size*sizeof(unsigned char));

                unsigned int count = (unsigned int) SDL_RWread(file, data, sizeof(unsigned char), size);
                *bytesRead = count;

                if (count != size) SDL_LogWarn(0, "FILEIO: [%s] File partially loaded", fileName);
                else  SDL_LogInfo(0, "FILEIO: [%s] File loaded successfully", fileName);
            }
            else SDL_LogWarn(0, "FILEIO: [%s] Failed to read file", fileName);
            SDL_RWclose(file);
        }
        else SDL_LogWarn(0, "FILEIO: [%s] Failed to open file", fileName);
    }
    else SDL_LogWarn(0, "FILEIO: File name provided is not valid");
    return data;
}

// Unload file data allocated by LoadFileData()
void UnloadFileData(unsigned char *data)
{
    RL_FREE(data);
}

// Save data to file from buffer
bool SaveFileData(const char *fileName, void *data, unsigned int bytesToWrite)
{
    bool success = false;

    if (fileName != NULL)
    {
        SDL_RWops* file= SDL_RWFromFile(fileName, "wb");



        if (file != NULL)
        {
            //unsigned int count = (unsigned int)fwrite(data, sizeof(unsigned char), bytesToWrite, file);

            unsigned int count = (unsigned int) SDL_RWwrite(file, data, sizeof(unsigned char), bytesToWrite);


            if (count == 0) SDL_LogWarn(0, "FILEIO: [%s] Failed to write file", fileName);
            else if (count != bytesToWrite) SDL_LogWarn(0, "FILEIO: [%s] File partially written", fileName);
            else  SDL_LogInfo(0, "FILEIO: [%s] File saved successfully", fileName);

            int result = SDL_RWclose(file);
            if (result == 0) success = true;
        }
        else SDL_LogWarn(0, "FILEIO: [%s] Failed to open file", fileName);
    }
    else SDL_LogWarn(0, "FILEIO: File name provided is not valid");

    return success;
}

// Load text data from file, returns a '\0' terminated string
// NOTE: text chars array should be freed manually
char *LoadFileText(const char *fileName)
{
    char *text = NULL;

    if (fileName != NULL)
    {

        SDL_RWops* textFile= SDL_RWFromFile(fileName, "rt");



        if (textFile != NULL)
        {
            // WARNING: When reading a file as 'text' file,
            // text mode causes carriage return-linefeed translation...
            // ...but using fseek() should return correct byte-offset

            int size =(int) SDL_RWsize(textFile);


            if (size > 0)
            {
                text = (char *)RL_MALLOC((size + 1)*sizeof(char));
            //    unsigned int count = (unsigned int)fread(text, sizeof(char), size, textFile);

                unsigned int count = (unsigned int) SDL_RWread(textFile, text, sizeof( char), size);

                // WARNING: \r\n is converted to \n on reading, so,
                // read bytes count gets reduced by the number of lines
                if (count < size) text = (char*)RL_REALLOC(text, count + 1);

                // Zero-terminate the string
                text[count] = '\0';

                 SDL_LogInfo(0, "FILEIO: [%s] Text file loaded successfully", fileName);
            }
            else SDL_LogWarn(0, "FILEIO: [%s] Failed to read text file", fileName);

            SDL_RWclose(textFile);
        }
        else SDL_LogWarn(0, "FILEIO: [%s] Failed to open text file", fileName);
    }
    else SDL_LogWarn(0, "FILEIO: File name provided is not valid");

    return text;
}

// Unload file text data allocated by LoadFileText()
void UnloadFileText(unsigned char *text)
{
    RL_FREE(text);
}

// Save text data to file (write), string must be '\0' terminated
bool SaveFileText(const char *fileName, char *text)
{
    bool success = false;

    if (fileName != NULL)
    {

         SDL_RWops* file= SDL_RWFromFile(fileName, "wt");


        if (file != NULL)
        {
             size_t strLen = SDL_strlen( text );
	         int count = SDL_RWwrite( file, text, 1, strLen );


            if (count < 0) SDL_LogWarn(0, "FILEIO: [%s] Failed to write text file", fileName);
            else SDL_LogInfo(0, "FILEIO: [%s] Text file saved successfully", fileName);

            int result = SDL_RWclose(file);
            if (result == 0) success = true;
        }
        else SDL_LogWarn(0, "FILEIO: [%s] Failed to open text file", fileName);
    }
    else SDL_LogWarn(0, "FILEIO: File name provided is not valid");

    return success;
}

// Get file modification time (last write time)
long GetFileModTime(const char *fileName)
{
    struct stat result = { 0 };

    if (stat(fileName, &result) == 0)
    {
        time_t mod = result.st_mtime;

        return (long)mod;
    }

    return 0;
}
long GetFileSize(const char *fileName)
{
    struct stat result = { 0 };

    if (stat(fileName, &result) == 0)
    {
        int mod = result.st_size;

        return (long)mod;
    }

    return 0;
}


// Save integer value to storage file (to defined position)
// NOTE: Storage positions is directly related to file memory layout (4 bytes each integer)
bool SaveStorageValue(unsigned int position, int value)
{
    bool success = false;

#if defined(SUPPORT_DATA_STORAGE)
    char path[512] = { 0 };
#if defined(PLATFORM_ANDROID)
    strcpy(path, SSDL_AndroidGetInternalStoragePath());
    strcat(path, "/");
    strcat(path, STORAGE_DATA_FILE);
#else
    strcpy(path, STORAGE_DATA_FILE);
#endif

    unsigned int dataSize = 0;
    unsigned int newDataSize = 0;
    unsigned char *fileData = LoadFileData(path, &dataSize);
    unsigned char *newFileData = NULL;

    if (fileData != NULL)
    {
        if (dataSize <= (position*sizeof(int)))
        {
            // Increase data size up to position and store value
            newDataSize = (position + 1)*sizeof(int);
            newFileData = (unsigned char *)RL_REALLOC(fileData, newDataSize);

            if (newFileData != NULL)
            {
                // RL_REALLOC succeded
                int *dataPtr = (int *)newFileData;
                dataPtr[position] = value;
            }
            else
            {
                // RL_REALLOC failed
                TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to realloc data (%u), position in bytes (%u) bigger than actual file size", path, dataSize, position*sizeof(int));

                // We store the old size of the file
                newFileData = fileData;
                newDataSize = dataSize;
            }
        }
        else
        {
            // Store the old size of the file
            newFileData = fileData;
            newDataSize = dataSize;

            // Replace value on selected position
            int *dataPtr = (int *)newFileData;
            dataPtr[position] = value;
        }

        success = SaveFileData(path, newFileData, newDataSize);
        RL_FREE(newFileData);
    }
    else
    {
        TRACELOG(LOG_INFO, "FILEIO: [%s] File not found, creating it", path);

        dataSize = (position + 1)*sizeof(int);
        fileData = (unsigned char *)RL_MALLOC(dataSize);
        int *dataPtr = (int *)fileData;
        dataPtr[position] = value;

        success = SaveFileData(path, fileData, dataSize);
        RL_FREE(fileData);
    }
#endif

    return success;
}

// Load integer value from storage file (from defined position)
// NOTE: If requested position could not be found, value 0 is returned
int LoadStorageValue(unsigned int position)
{
    int value = 0;
#if defined(SUPPORT_DATA_STORAGE)
    char path[512] = { 0 };
#if defined(PLATFORM_ANDROID)
    strcpy(path, CORE.Android.internalDataPath);
    strcat(path, "/");
    strcat(path, STORAGE_DATA_FILE);
#else
    strcpy(path, STORAGE_DATA_FILE);
#endif

    unsigned int dataSize = 0;
    unsigned char *fileData = LoadFileData(path, &dataSize);

    if (fileData != NULL)
    {
        if (dataSize < (position*4)) SDL_Log("SYSTEM: Failed to find storage position");
        else
        {
            int *dataPtr = (int *)fileData;
            value = dataPtr[position];
        }

        RL_FREE(fileData);
    }
#endif
    return value;
}

// Open URL with default system browser (if available)
// NOTE: This function is only safe to use if you control the URL given.
// A user could craft a malicious string performing another action.
// Only call this function yourself not with user input or make sure to check the string yourself.
// Ref: https://github.com/raysan5/raylib/issues/686
void OpenURL(const char *url)
{
    // Small security check trying to avoid (partially) malicious code...
    // sorry for the inconvenience when you hit this point...
    if (strchr(url, '\'') != NULL)
    {
        SDL_Log( "SYSTEM: Provided URL is not valid");
    }
    else
    {
#if defined(PLATFORM_DESKTOP)
        char *cmd = (char *)RL_CALLOC(strlen(url) + 10, sizeof(char));
    #if defined(_WIN32)
        sprintf(cmd, "explorer %s", url);
    #elif defined(__linux__)
        sprintf(cmd, "xdg-open '%s'", url); // Alternatives: firefox, x-www-browser
    #elif defined(__APPLE__)
        sprintf(cmd, "open '%s'", url);
    #endif
        system(cmd);
        RL_FREE(cmd);
#endif
#if defined(PLATFORM_WEB)
        emscripten_run_script(TextFormat("window.open('%s', '_blank')", url));
#endif
    }
}


//----------------------------------------------------------------------------------
// Text strings management functions
//----------------------------------------------------------------------------------
// Get text length in bytes, check for \0 character
unsigned int TextLength(const char *text)
{
    unsigned int length = 0; //strlen(text)

    if (text != NULL)
    {
        while (*text++) length++;
    }

    return length;
}

// Formatting of text with variables to 'embed'
// WARNING: String returned will expire after this function is called MAX_TEXTFORMAT_BUFFERS times
const char *TextFormat(const char *text, ...)
{

    #define MAX_TEXTFORMAT_BUFFERS 4        // Maximum number of static buffers for text formatting

    // We create an array of buffers so strings don't expire until MAX_TEXTFORMAT_BUFFERS invocations
    static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = { 0 };
    static int  index = 0;

    char *currentBuffer = buffers[index];
    memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);   // Clear buffer before using

    va_list args;
    va_start(args, text);
    vsprintf(currentBuffer, text, args);
    va_end(args);

    index += 1;     // Move to next buffer for next function call
    if (index >= MAX_TEXTFORMAT_BUFFERS) index = 0;

    return currentBuffer;
}

// Get integer value from text
// NOTE: This function replaces atoi() [stdlib.h]
int TextToInteger(const char *text)
{
    int value = 0;
    int sign = 1;

    if ((text[0] == '+') || (text[0] == '-'))
    {
        if (text[0] == '-') sign = -1;
        text++;
    }

    for (int i = 0; ((text[i] >= '0') && (text[i] <= '9')); ++i) value = value*10 + (int)(text[i] - '0');

    return value*sign;
}


// Copy one string to another, returns bytes copied
int TextCopy(char *dst, const char *src)
{
    int bytes = 0;

    if (dst != NULL)
    {
        while (*src != '\0')
        {
            *dst = *src;
            dst++;
            src++;

            bytes++;
        }

        *dst = '\0';
    }

    return bytes;
}

// Check if two text string are equal
// REQUIRES: strcmp()
bool TextIsEqual(const char *text1, const char *text2)
{
    bool result = false;

    if (strcmp(text1, text2) == 0) result = true;

    return result;
}

// Get a piece of a text string
const char *TextSubtext(const char *text, int position, int length)
{
    static char buffer[MAX_TEXT_BUFFER_LENGTH] = { 0 };

    int textLength = TextLength(text);

    if (position >= textLength)
    {
        position = textLength - 1;
        length = 0;
    }

    if (length >= textLength) length = textLength;

    for (int c = 0 ; c < length ; c++)
    {
        *(buffer + c) = *(text + position);
        text++;
    }

    *(buffer + length) = '\0';

    return buffer;
}

// Replace text string
// REQUIRES: strstr(), strncpy(), strcpy()
// WARNING: Internally allocated memory must be freed by the user (if return != NULL)
char *TextReplace(char *text, const char *replace, const char *by)
{
    // Sanity checks and initialization
    if (!text || !replace || !by) return NULL;

    char *result;

    char *insertPoint;      // Next insert point
    char *temp;             // Temp pointer
    int replaceLen;         // Replace string length of (the string to remove)
    int byLen;              // Replacement length (the string to replace replace by)
    int lastReplacePos;     // Distance between replace and end of last replace
    int count;              // Number of replacements

    replaceLen = TextLength(replace);
    if (replaceLen == 0) return NULL;  // Empty replace causes infinite loop during count

    byLen = TextLength(by);

    // Count the number of replacements needed
    insertPoint = text;
    for (count = 0; (temp = strstr(insertPoint, replace)); count++) insertPoint = temp + replaceLen;

    // Allocate returning string and point temp to it
    temp = result =(char*) RL_MALLOC(TextLength(text) + (byLen - replaceLen)*count + 1);

    if (!result) return NULL;   // Memory could not be allocated

    // First time through the loop, all the variable are set correctly from here on,
    //    temp points to the end of the result string
    //    insertPoint points to the next occurrence of replace in text
    //    text points to the remainder of text after "end of replace"
    while (count--)
    {
        insertPoint = strstr(text, replace);
        lastReplacePos = (int)(insertPoint - text);
        temp = strncpy(temp, text, lastReplacePos) + lastReplacePos;
        temp = strcpy(temp, by) + byLen;
        text += lastReplacePos + replaceLen; // Move to next "end of replace"
    }

    // Copy remaind text part after replacement to result (pointed by moving temp)
    strcpy(temp, text);

    return result;
}

// Insert text in a specific position, moves all text forward
// WARNING: Allocated memory should be manually freed
char *TextInsert(const char *text, const char *insert, int position)
{
    int textLen = TextLength(text);
    int insertLen =  TextLength(insert);

    char *result = (char *)RL_MALLOC(textLen + insertLen + 1);

    for (int i = 0; i < position; i++) result[i] = text[i];
    for (int i = position; i < insertLen + position; i++) result[i] = insert[i];
    for (int i = (insertLen + position); i < (textLen + insertLen); i++) result[i] = text[i];

    result[textLen + insertLen] = '\0';     // Make sure text string is valid!

    return result;
}

// Join text strings with delimiter
// REQUIRES: memset(), memcpy()
const char *TextJoin(const char **textList, int count, const char *delimiter)
{
    static char text[MAX_TEXT_BUFFER_LENGTH] = { 0 };
    memset(text, 0, MAX_TEXT_BUFFER_LENGTH);
    char *textPtr = text;

    int totalLength = 0;
    int delimiterLen = TextLength(delimiter);

    for (int i = 0; i < count; i++)
    {
        int textLength = TextLength(textList[i]);

        // Make sure joined text could fit inside MAX_TEXT_BUFFER_LENGTH
        if ((totalLength + textLength) < MAX_TEXT_BUFFER_LENGTH)
        {
            memcpy(textPtr, textList[i], textLength);
            totalLength += textLength;
            textPtr += textLength;

            if ((delimiterLen > 0) && (i < (count - 1)))
            {
                memcpy(textPtr, delimiter, delimiterLen);
                totalLength += delimiterLen;
                textPtr += delimiterLen;
            }
        }
    }

    return text;
}

// Split string into multiple strings
const char **TextSplit(const char *text, char delimiter, int *count)
{
    // NOTE: Current implementation returns a copy of the provided string with '\0' (string end delimiter)
    // inserted between strings defined by "delimiter" parameter. No memory is dynamically allocated,
    // all used memory is static... it has some limitations:
    //      1. Maximum number of possible split strings is set by MAX_TEXTSPLIT_COUNT
    //      2. Maximum size of text to split is MAX_TEXT_BUFFER_LENGTH

    static const char *result[MAX_TEXTSPLIT_COUNT] = { NULL };
    static char buffer[MAX_TEXT_BUFFER_LENGTH] = { 0 };
    memset(buffer, 0, MAX_TEXT_BUFFER_LENGTH);

    result[0] = buffer;
    int counter = 0;

    if (text != NULL)
    {
        counter = 1;

        // Count how many substrings we have on text and point to every one
        for (int i = 0; i < MAX_TEXT_BUFFER_LENGTH; i++)
        {
            buffer[i] = text[i];
            if (buffer[i] == '\0') break;
            else if (buffer[i] == delimiter)
            {
                buffer[i] = '\0';   // Set an end of string at this point
                result[counter] = buffer + i + 1;
                counter++;

                if (counter == MAX_TEXTSPLIT_COUNT) break;
            }
        }
    }

    *count = counter;
    return result;
}

// Append text at specific position and move cursor!
// REQUIRES: strcpy()
void TextAppend(char *text, const char *append, int *position)
{
    strcpy(text + *position, append);
    *position += TextLength(append);
}

// Find first text occurrence within a string
// REQUIRES: strstr()
int TextFindIndex(const char *text, const char *find)
{
    int position = -1;

    char *ptr = (char*)strstr(text, find);

    if (ptr != NULL) position = (int)(ptr - text);

    return position;
}

// Get upper case version of provided string
const char *TextToUpper(const char *text)
{
    static char buffer[MAX_TEXT_BUFFER_LENGTH] = { 0 };

    for (int i = 0; i < MAX_TEXT_BUFFER_LENGTH; i++)
    {
        if (text[i] != '\0')
        {
            buffer[i] = (char)toupper(text[i]);
            //if ((text[i] >= 'a') && (text[i] <= 'z')) buffer[i] = text[i] - 32;

            // TODO: Support Utf8 diacritics!
            //if ((text[i] >= 'à') && (text[i] <= 'ý')) buffer[i] = text[i] - 32;
        }
        else { buffer[i] = '\0'; break; }
    }

    return buffer;
}

// Get lower case version of provided string
const char *TextToLower(const char *text)
{
    static char buffer[MAX_TEXT_BUFFER_LENGTH] = { 0 };

    for (int i = 0; i < MAX_TEXT_BUFFER_LENGTH; i++)
    {
        if (text[i] != '\0')
        {
            buffer[i] = (char)tolower(text[i]);
            //if ((text[i] >= 'A') && (text[i] <= 'Z')) buffer[i] = text[i] + 32;
        }
        else { buffer[i] = '\0'; break; }
    }

    return buffer;
}

// Get Pascal case notation version of provided string
const char *TextToPascal(const char *text)
{
    static char buffer[MAX_TEXT_BUFFER_LENGTH] = { 0 };

    buffer[0] = (char)toupper(text[0]);

    for (int i = 1, j = 1; i < MAX_TEXT_BUFFER_LENGTH; i++, j++)
    {
        if (text[j] != '\0')
        {
            if (text[j] != '_') buffer[i] = text[j];
            else
            {
                j++;
                buffer[i] = (char)toupper(text[j]);
            }
        }
        else { buffer[i] = '\0'; break; }
    }

    return buffer;
}

// Encode text codepoint into utf8 text (memory must be freed!)
char *TextToUtf8(int *codepoints, int length)
{
    // We allocate enough memory fo fit all possible codepoints
    // NOTE: 5 bytes for every codepoint should be enough
    char *text = (char*)RL_CALLOC(length*5, 1);
    const char *utf8 = NULL;
    int size = 0;

    for (int i = 0, bytes = 0; i < length; i++)
    {
        utf8 = CodepointToUtf8(codepoints[i], &bytes);
        memcpy(text + size, utf8, bytes);
        size += bytes;
    }

    // Resize memory to text length + string NULL terminator
    void *ptr = RL_REALLOC(text, size + 1);

    if (ptr != NULL) text = (char *)ptr;

    return text;
}

// Encode codepoint into utf8 text (char array length returned as parameter)
 const char *CodepointToUtf8(int codepoint, int *byteLength)
{
    static char utf8[6] = { 0 };
    int length = 0;

    if (codepoint <= 0x7f)
    {
        utf8[0] = (char)codepoint;
        length = 1;
    }
    else if (codepoint <= 0x7ff)
    {
        utf8[0] = (char)(((codepoint >> 6) & 0x1f) | 0xc0);
        utf8[1] = (char)((codepoint & 0x3f) | 0x80);
        length = 2;
    }
    else if (codepoint <= 0xffff)
    {
        utf8[0] = (char)(((codepoint >> 12) & 0x0f) | 0xe0);
        utf8[1] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[2] = (char)((codepoint & 0x3f) | 0x80);
        length = 3;
    }
    else if (codepoint <= 0x10ffff)
    {
        utf8[0] = (char)(((codepoint >> 18) & 0x07) | 0xf0);
        utf8[1] = (char)(((codepoint >> 12) & 0x3f) | 0x80);
        utf8[2] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[3] = (char)((codepoint & 0x3f) | 0x80);
        length = 4;
    }

    *byteLength = length;

    return utf8;
}



// Encode codepoint into utf8 text (char array length returned as parameter)
// NOTE: It uses a static array to store UTF-8 bytes
 const char *CodepointToUTF8(int codepoint, int *byteSize)
{
    static char utf8[6] = { 0 };
    int size = 0;   // Byte size of codepoint

    if (codepoint <= 0x7f)
    {
        utf8[0] = (char)codepoint;
        size = 1;
    }
    else if (codepoint <= 0x7ff)
    {
        utf8[0] = (char)(((codepoint >> 6) & 0x1f) | 0xc0);
        utf8[1] = (char)((codepoint & 0x3f) | 0x80);
        size = 2;
    }
    else if (codepoint <= 0xffff)
    {
        utf8[0] = (char)(((codepoint >> 12) & 0x0f) | 0xe0);
        utf8[1] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[2] = (char)((codepoint & 0x3f) | 0x80);
        size = 3;
    }
    else if (codepoint <= 0x10ffff)
    {
        utf8[0] = (char)(((codepoint >> 18) & 0x07) | 0xf0);
        utf8[1] = (char)(((codepoint >> 12) & 0x3f) | 0x80);
        utf8[2] = (char)(((codepoint >>  6) & 0x3f) | 0x80);
        utf8[3] = (char)((codepoint & 0x3f) | 0x80);
        size = 4;
    }

    *byteSize = size;

    return utf8;
}


// Get next codepoint in a UTF-8 encoded text, scanning until '\0' is found
// When a invalid UTF-8 byte is encountered we exit as soon as possible and a '?'(0x3f) codepoint is returned
// Total number of bytes processed are returned as a parameter
// NOTE: The standard says U+FFFD should be returned in case of errors
// but that character is not supported by the default font in raylib
int GetCodepoint(const char *text, int *bytesProcessed)
{
/*
    UTF-8 specs from https://www.ietf.org/rfc/rfc3629.txt

    Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
    // NOTE: on decode errors we return as soon as possible

    int code = 0x3f;   // Codepoint (defaults to '?')
    int octet = (unsigned char)(text[0]); // The first UTF8 octet
    *bytesProcessed = 1;

    if (octet <= 0x7f)
    {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    }
    else if ((octet & 0xe0) == 0xc0)
    {
        // Two octets

        // [0]xC2-DF    [1]UTF8-tail(x80-BF)
        unsigned char octet1 = text[1];

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf))
        {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytesProcessed = 2;
        }
    }
    else if ((octet & 0xf0) == 0xe0)
    {
        // Three octets
        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; } // Unexpected sequence

        // [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
        // [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        // [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
        // [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) { *bytesProcessed = 2; return code; }

        if ((octet >= 0xe0) && (0 <= 0xef))
        {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytesProcessed = 3;
        }
    }
    else if ((octet & 0xf8) == 0xf0)
    {
        // Four octets
        if (octet > 0xf4) return code;

        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';
        unsigned char octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; }  // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; }  // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytesProcessed = 4; return code; }  // Unexpected sequence

        // [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
        // [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
        // [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if (octet >= 0xf0)
        {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytesProcessed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f;     // Codepoints after U+10ffff are invalid

    return code;
}

// Get all codepoints in a string, codepoints count returned by parameters
int *GetCodepoints(const char *text, int *count)
{
    static int codepoints[MAX_TEXT_UNICODE_CHARS] = { 0 };
    memset(codepoints, 0, MAX_TEXT_UNICODE_CHARS*sizeof(int));

    int bytesProcessed = 0;
    int textLength = TextLength(text);
    int codepointsCount = 0;

    for (int i = 0; i < textLength; codepointsCount++)
    {
        codepoints[codepointsCount] = GetNextCodepoint(text + i, &bytesProcessed);
        i += bytesProcessed;
    }

    *count = codepointsCount;

    return codepoints;
}

// Returns total number of characters(codepoints) in a UTF8 encoded text, until '\0' is found
// NOTE: If an invalid UTF8 sequence is encountered a '?'(0x3f) codepoint is counted instead
int GetCodepointsCount(const char *text)
{
    unsigned int len = 0;
    char *ptr = (char *)&text[0];

    while (*ptr != '\0')
    {
        int next = 0;
        int letter = GetNextCodepoint(ptr, &next);

        if (letter == 0x3f) ptr += 1;
        else ptr += next;

        len++;
    }

    return len;
}


// Returns next codepoint in a UTF8 encoded text, scanning until '\0' is found
// When a invalid UTF8 byte is encountered we exit as soon as possible and a '?'(0x3f) codepoint is returned
// Total number of bytes processed are returned as a parameter
// NOTE: the standard says U+FFFD should be returned in case of errors
// but that character is not supported by the default font in raylib
// TODO: optimize this code for speed!!
int GetNextCodepoint(const char *text, int *bytesProcessed)
{
/*
    UTF8 specs from https://www.ietf.org/rfc/rfc3629.txt

    Char. number range  |        UTF-8 octet sequence
      (hexadecimal)    |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
    // NOTE: on decode errors we return as soon as possible

    int code = 0x3f;   // Codepoint (defaults to '?')
    int octet = (unsigned char)(text[0]); // The first UTF8 octet
    *bytesProcessed = 1;

    if (octet <= 0x7f)
    {
        // Only one octet (ASCII range x00-7F)
        code = text[0];
    }
    else if ((octet & 0xe0) == 0xc0)
    {
        // Two octets
        // [0]xC2-DF    [1]UTF8-tail(x80-BF)
        unsigned char octet1 = text[1];

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if ((octet >= 0xc2) && (octet <= 0xdf))
        {
            code = ((octet & 0x1f) << 6) | (octet1 & 0x3f);
            *bytesProcessed = 2;
        }
    }
    else if ((octet & 0xf0) == 0xe0)
    {
        // Three octets
        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; } // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; } // Unexpected sequence

        /*
            [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
            [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
            [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
            [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        */

        if (((octet == 0xe0) && !((octet1 >= 0xa0) && (octet1 <= 0xbf))) ||
            ((octet == 0xed) && !((octet1 >= 0x80) && (octet1 <= 0x9f)))) { *bytesProcessed = 2; return code; }

        if ((octet >= 0xe0) && (0 <= 0xef))
        {
            code = ((octet & 0xf) << 12) | ((octet1 & 0x3f) << 6) | (octet2 & 0x3f);
            *bytesProcessed = 3;
        }
    }
    else if ((octet & 0xf8) == 0xf0)
    {
        // Four octets
        if (octet > 0xf4) return code;

        unsigned char octet1 = text[1];
        unsigned char octet2 = '\0';
        unsigned char octet3 = '\0';

        if ((octet1 == '\0') || ((octet1 >> 6) != 2)) { *bytesProcessed = 2; return code; }  // Unexpected sequence

        octet2 = text[2];

        if ((octet2 == '\0') || ((octet2 >> 6) != 2)) { *bytesProcessed = 3; return code; }  // Unexpected sequence

        octet3 = text[3];

        if ((octet3 == '\0') || ((octet3 >> 6) != 2)) { *bytesProcessed = 4; return code; }  // Unexpected sequence

        /*
            [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
            [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
            [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail
        */

        if (((octet == 0xf0) && !((octet1 >= 0x90) && (octet1 <= 0xbf))) ||
            ((octet == 0xf4) && !((octet1 >= 0x80) && (octet1 <= 0x8f)))) { *bytesProcessed = 2; return code; } // Unexpected sequence

        if (octet >= 0xf0)
        {
            code = ((octet & 0x7) << 18) | ((octet1 & 0x3f) << 12) | ((octet2 & 0x3f) << 6) | (octet3 & 0x3f);
            *bytesProcessed = 4;
        }
    }

    if (code > 0x10ffff) code = 0x3f;     // Codepoints after U+10ffff are invalid

    return code;
}

// Set pixel color formatted into destination pointer
void SetPixelColor(void *dstPtr, Color color, int format)
{
    switch (format)
    {
        case PIXELFORMAT_GRAYSCALE:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dstPtr)[0] = gray;

        } break;
        case PIXELFORMAT_GRAY_ALPHA:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dstPtr)[0] = gray;
            ((unsigned char *)dstPtr)[1] = color.a;

        } break;
        case PIXELFORMAT_R5G6B5:
        {
            // NOTE: Calculate R5G6B5 equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*31.0f));
            unsigned char g = (unsigned char)(round(coln.y*63.0f));
            unsigned char b = (unsigned char)(round(coln.z*31.0f));

            ((unsigned short *)dstPtr)[0] = (unsigned short)r << 11 | (unsigned short)g << 5 | (unsigned short)b;

        } break;
        case PIXELFORMAT_R5G5B5A1:
        {
            // NOTE: Calculate R5G5B5A1 equivalent color
            Vector4 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f, (float)color.a/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*31.0f));
            unsigned char g = (unsigned char)(round(coln.y*31.0f));
            unsigned char b = (unsigned char)(round(coln.z*31.0f));
            unsigned char a = (coln.w > ((float)PIXELFORMAT_R5G5B5A1_ALPHA_THRESHOLD/255.0f))? 1 : 0;;

            ((unsigned short *)dstPtr)[0] = (unsigned short)r << 11 | (unsigned short)g << 6 | (unsigned short)b << 1 | (unsigned short)a;

        } break;
        case PIXELFORMAT_R4G4B4A4:
        {
            // NOTE: Calculate R5G5B5A1 equivalent color
            Vector4 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f, (float)color.a/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*15.0f));
            unsigned char g = (unsigned char)(round(coln.y*15.0f));
            unsigned char b = (unsigned char)(round(coln.z*15.0f));
            unsigned char a = (unsigned char)(round(coln.w*15.0f));

            ((unsigned short *)dstPtr)[0] = (unsigned short)r << 12 | (unsigned short)g << 8 | (unsigned short)b << 4 | (unsigned short)a;

        } break;
        case PIXELFORMAT_R8G8B8:
        {
            ((unsigned char *)dstPtr)[0] = color.r;
            ((unsigned char *)dstPtr)[1] = color.g;
            ((unsigned char *)dstPtr)[2] = color.b;

        } break;
        case PIXELFORMAT_R8G8B8A8:
        {
            ((unsigned char *)dstPtr)[0] = color.r;
            ((unsigned char *)dstPtr)[1] = color.g;
            ((unsigned char *)dstPtr)[2] = color.b;
            ((unsigned char *)dstPtr)[3] = color.a;

        } break;
        default: break;
    }
}

// Load color data from image as a Color array (RGBA - 32bit)
// NOTE: Memory allocated should be freed using UnloadImageColors();
Color *LoadImageColors(Image image)
{
    if ((image.width == 0) || (image.height == 0)) return NULL;

    Color *pixels = (Color *)RL_MALLOC(image.width*image.height*sizeof(Color));


        if ((image.format == PIXELFORMAT_R32) ||
            (image.format == PIXELFORMAT_R32G32B32) ||
            (image.format == PIXELFORMAT_R32G32B32A32)) TRACELOG(LOG_WARNING, "IMAGE: Pixel format converted from 32bit to 8bit per channel");

        for (int i = 0, k = 0; i < image.width*image.height; i++)
        {
            switch (image.format)
            {
                case PIXELFORMAT_GRAYSCALE:
                {
                    pixels[i].r = ((unsigned char *)image.data)[i];
                    pixels[i].g = ((unsigned char *)image.data)[i];
                    pixels[i].b = ((unsigned char *)image.data)[i];
                    pixels[i].a = 255;

                } break;
                case PIXELFORMAT_GRAY_ALPHA:
                {
                    pixels[i].r = ((unsigned char *)image.data)[k];
                    pixels[i].g = ((unsigned char *)image.data)[k];
                    pixels[i].b = ((unsigned char *)image.data)[k];
                    pixels[i].a = ((unsigned char *)image.data)[k + 1];

                    k += 2;
                } break;
                case PIXELFORMAT_R5G5B5A1:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].r = (unsigned char)((float)((pixel & 0b1111100000000000) >> 11)*(255/31));
                    pixels[i].g = (unsigned char)((float)((pixel & 0b0000011111000000) >> 6)*(255/31));
                    pixels[i].b = (unsigned char)((float)((pixel & 0b0000000000111110) >> 1)*(255/31));
                    pixels[i].a = (unsigned char)((pixel & 0b0000000000000001)*255);

                } break;
                case PIXELFORMAT_R5G6B5:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].r = (unsigned char)((float)((pixel & 0b1111100000000000) >> 11)*(255/31));
                    pixels[i].g = (unsigned char)((float)((pixel & 0b0000011111100000) >> 5)*(255/63));
                    pixels[i].b = (unsigned char)((float)(pixel & 0b0000000000011111)*(255/31));
                    pixels[i].a = 255;

                } break;
                case PIXELFORMAT_R4G4B4A4:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].r = (unsigned char)((float)((pixel & 0b1111000000000000) >> 12)*(255/15));
                    pixels[i].g = (unsigned char)((float)((pixel & 0b0000111100000000) >> 8)*(255/15));
                    pixels[i].b = (unsigned char)((float)((pixel & 0b0000000011110000) >> 4)*(255/15));
                    pixels[i].a = (unsigned char)((float)(pixel & 0b0000000000001111)*(255/15));

                } break;
                case PIXELFORMAT_R8G8B8A8:
                {
                    pixels[i].r = ((unsigned char *)image.data)[k];
                    pixels[i].g = ((unsigned char *)image.data)[k + 1];
                    pixels[i].b = ((unsigned char *)image.data)[k + 2];
                    pixels[i].a = ((unsigned char *)image.data)[k + 3];

                    k += 4;
                } break;
                case PIXELFORMAT_R8G8B8:
                {
                    pixels[i].r = (unsigned char)((unsigned char *)image.data)[k];
                    pixels[i].g = (unsigned char)((unsigned char *)image.data)[k + 1];
                    pixels[i].b = (unsigned char)((unsigned char *)image.data)[k + 2];
                    pixels[i].a = 255;

                    k += 3;
                } break;
                case PIXELFORMAT_R32:
                {
                    pixels[i].r = (unsigned char)(((float *)image.data)[k]*255.0f);
                    pixels[i].g = 0;
                    pixels[i].b = 0;
                    pixels[i].a = 255;

                } break;
                case PIXELFORMAT_R32G32B32:
                {
                    pixels[i].r = (unsigned char)(((float *)image.data)[k]*255.0f);
                    pixels[i].g = (unsigned char)(((float *)image.data)[k + 1]*255.0f);
                    pixels[i].b = (unsigned char)(((float *)image.data)[k + 2]*255.0f);
                    pixels[i].a = 255;

                    k += 3;
                } break;
                case PIXELFORMAT_R32G32B32A32:
                {
                    pixels[i].r = (unsigned char)(((float *)image.data)[k]*255.0f);
                    pixels[i].g = (unsigned char)(((float *)image.data)[k]*255.0f);
                    pixels[i].b = (unsigned char)(((float *)image.data)[k]*255.0f);
                    pixels[i].a = (unsigned char)(((float *)image.data)[k]*255.0f);

                    k += 4;
                } break;
                default: break;
            }
        }


    return pixels;
}




// Get pixel data size in bytes for certain format
// NOTE: Size can be requested for Image or Texture data
int GetPixelDataSize(int width, int height, int format)
{
    int dataSize = 0;       // Size in bytes
    int bpp = 0;            // Bits per pixel

    switch (format)
    {
        case PIXELFORMAT_GRAYSCALE: bpp = 8; break;
        case PIXELFORMAT_GRAY_ALPHA:
        case PIXELFORMAT_R5G6B5:
        case PIXELFORMAT_R5G5B5A1:
        case PIXELFORMAT_R4G4B4A4: bpp = 16; break;
        case PIXELFORMAT_R8G8B8A8: bpp = 32; break;
        case PIXELFORMAT_R8G8B8: bpp = 24; break;
        case PIXELFORMAT_R32: bpp = 32; break;
        case PIXELFORMAT_R32G32B32: bpp = 32*3; break;
        case PIXELFORMAT_R32G32B32A32: bpp = 32*4; break;

        default: break;
    }

    dataSize = width*height*bpp/8;

    return dataSize;
}

void UnloadImage(Image image)
{
    if (image.data)
       STBI_FREE(image.data);
}

Image LoadImage(const char *fileName)
{
    Image image = { 0 };


    // Loading file to memory
    unsigned int fileSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &fileSize);



    if (fileData != NULL)
    {
        // Loading image from memory data
        image = LoadImageFromMemory( fileData, fileSize);

       if (image.data != NULL) TRACELOG(LOG_INFO, "IMAGE: [%s] Data loaded successfully (%ix%i)", fileName, image.width, image.height);
        else TRACELOG(0, "IMAGE: [%s] Failed to load data", fileName);

       RL_FREE(fileData);
    }

    return image;
}

// Create an image from another image piece
Image ImageFromImage(Image image, Rectangle rec)
{
    Image result = { 0 };

    int bytesPerPixel = GetPixelDataSize(1, 1, image.format);

    // TODO: Check rec is valid?

    result.width = (int)rec.width;
    result.height = (int)rec.height;
    result.data = RL_CALLOC((int)(rec.width*rec.height)*bytesPerPixel, 1);
    result.format = image.format;
    result.mipmaps = 1;

    for (int y = 0; y < rec.height; y++)
    {
        memcpy(((unsigned char *)result.data) + y*(int)rec.width*bytesPerPixel, ((unsigned char *)image.data) + ((y + (int)rec.y)*image.width + (int)rec.x)*bytesPerPixel, (int)rec.width*bytesPerPixel);
    }

    return result;
}
// Load image from memory buffer, fileType refers to extension: i.e. ".png"
Image LoadImageFromMemory(const unsigned char *fileData, int dataSize)
{
    Image image = { 0 };


        // NOTE: Using stb_image to load images (Supports multiple image formats)

        if (fileData != NULL)
        {
            int comp = 0;
            image.data = stbi_load_from_memory(fileData, dataSize, &image.width, &image.height, &comp, 4);


            image.mipmaps = 1;

            if (comp == 1) image.format = PIXELFORMAT_GRAYSCALE;
            else if (comp == 2) image.format = PIXELFORMAT_GRAY_ALPHA;
            else if (comp == 3) image.format = PIXELFORMAT_R8G8B8;
            else if (comp == 4) image.format = PIXELFORMAT_R8G8B8A8;

            image.format = PIXELFORMAT_R8G8B8A8;
        }





    return image;
}



// Draw pixel within an image
// NOTE: Compressed image formats not supported
void ImageDrawPixel(Image *dst, int x, int y, Color color)
{
    // Security check to avoid program crash
    if ((dst->data == NULL) || (x < 0) || (x >= dst->width) || (y < 0) || (y >= dst->height)) return;

    switch (dst->format)
    {
        case PIXELFORMAT_GRAYSCALE:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dst->data)[y*dst->width + x] = gray;

        } break;
        case PIXELFORMAT_GRAY_ALPHA:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dst->data)[(y*dst->width + x)*2] = gray;
            ((unsigned char *)dst->data)[(y*dst->width + x)*2 + 1] = color.a;

        } break;
        case PIXELFORMAT_R5G6B5:
        {
            // NOTE: Calculate R5G6B5 equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*31.0f));
            unsigned char g = (unsigned char)(round(coln.y*63.0f));
            unsigned char b = (unsigned char)(round(coln.z*31.0f));

            ((unsigned short *)dst->data)[y*dst->width + x] = (unsigned short)r << 11 | (unsigned short)g << 5 | (unsigned short)b;

        } break;
        case PIXELFORMAT_R5G5B5A1:
        {
            // NOTE: Calculate R5G5B5A1 equivalent color
            Vector4 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f, (float)color.a/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*31.0f));
            unsigned char g = (unsigned char)(round(coln.y*31.0f));
            unsigned char b = (unsigned char)(round(coln.z*31.0f));
            unsigned char a = (coln.w > ((float)PIXELFORMAT_R5G5B5A1_ALPHA_THRESHOLD/255.0f))? 1 : 0;;

            ((unsigned short *)dst->data)[y*dst->width + x] = (unsigned short)r << 11 | (unsigned short)g << 6 | (unsigned short)b << 1 | (unsigned short)a;

        } break;
        case PIXELFORMAT_R4G4B4A4:
        {
            // NOTE: Calculate R5G5B5A1 equivalent color
            Vector4 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f, (float)color.a/255.0f };

            unsigned char r = (unsigned char)(round(coln.x*15.0f));
            unsigned char g = (unsigned char)(round(coln.y*15.0f));
            unsigned char b = (unsigned char)(round(coln.z*15.0f));
            unsigned char a = (unsigned char)(round(coln.w*15.0f));

            ((unsigned short *)dst->data)[y*dst->width + x] = (unsigned short)r << 12 | (unsigned short)g << 8 | (unsigned short)b << 4 | (unsigned short)a;

        } break;
        case PIXELFORMAT_R8G8B8:
        {
            ((unsigned char *)dst->data)[(y*dst->width + x)*3] = color.r;
            ((unsigned char *)dst->data)[(y*dst->width + x)*3 + 1] = color.g;
            ((unsigned char *)dst->data)[(y*dst->width + x)*3 + 2] = color.b;

        } break;
        case PIXELFORMAT_R8G8B8A8:
        {
            ((unsigned char *)dst->data)[(y*dst->width + x)*4] = color.r;
            ((unsigned char *)dst->data)[(y*dst->width + x)*4 + 1] = color.g;
            ((unsigned char *)dst->data)[(y*dst->width + x)*4 + 2] = color.b;
            ((unsigned char *)dst->data)[(y*dst->width + x)*4 + 3] = color.a;

        } break;
        case PIXELFORMAT_R32:
        {
            // NOTE: Calculate grayscale equivalent color (normalized to 32bit)
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };

            ((float *)dst->data)[y*dst->width + x] = coln.x*0.299f + coln.y*0.587f + coln.z*0.114f;

        } break;
        case PIXELFORMAT_R32G32B32:
        {
            // NOTE: Calculate R32G32B32 equivalent color (normalized to 32bit)
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };

            ((float *)dst->data)[(y*dst->width + x)*3] = coln.x;
            ((float *)dst->data)[(y*dst->width + x)*3 + 1] = coln.y;
            ((float *)dst->data)[(y*dst->width + x)*3 + 2] = coln.z;
        } break;
        case PIXELFORMAT_R32G32B32A32:
        {
            // NOTE: Calculate R32G32B32A32 equivalent color (normalized to 32bit)
            Vector4 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f, (float)color.a/255.0f };

            ((float *)dst->data)[(y*dst->width + x)*4] = coln.x;
            ((float *)dst->data)[(y*dst->width + x)*4 + 1] = coln.y;
            ((float *)dst->data)[(y*dst->width + x)*4 + 2] = coln.z;
            ((float *)dst->data)[(y*dst->width + x)*4 + 3] = coln.w;

        } break;
        default: break;
    }
}
void ImageClearBackground(Image *dst, Color color)
{
    for (int i = 0; i < dst->width*dst->height; ++i) ImageDrawPixel(dst, i%dst->width, i/dst->width, color);
}
Image LoadImageFromFile(const char* filename)
{
    Image image = { 0 };

   int comp = 0;
   image.data = stbi_load(filename,&image.width, &image.height, &comp, 4);

            image.mipmaps = 1;

            if (comp == 1) image.format = PIXELFORMAT_GRAYSCALE;
            else if (comp == 2) image.format = PIXELFORMAT_GRAY_ALPHA;
            else if (comp == 3) image.format = PIXELFORMAT_R8G8B8;
            else if (comp == 4) image.format = PIXELFORMAT_R8G8B8A8;

            image.format = PIXELFORMAT_R8G8B8A8;

int dataSize = GetPixelDataSize(image.width, image.height, image.format);

SDL_Log("Load from image %d %d %d  %d",image.width,image.height,image.format,dataSize);





    return image;
}

// Export image data to file
// NOTE: File format depends on fileName extension
bool ExportImage(Image image, const char *fileName)
{
    int success = 0;


    int channels = 4;
    bool allocatedData = false;
    unsigned char *imgData = (unsigned char *)image.data;

    if (image.format == PIXELFORMAT_GRAYSCALE) channels = 1;
    else if (image.format == PIXELFORMAT_GRAY_ALPHA) channels = 2;
    else if (image.format == PIXELFORMAT_R8G8B8) channels = 3;
    else if (image.format == PIXELFORMAT_R8G8B8A8) channels = 4;
    else
    {
        // NOTE: Getting Color array as RGBA unsigned char values
        imgData = (unsigned char *)LoadImageColors(image);
        allocatedData = true;
    }

    if (IsFileExtension(fileName, ".png")) success = stbi_write_png(fileName, image.width, image.height, channels, imgData, image.width*channels);
    else if (IsFileExtension(fileName, ".bmp")) success = stbi_write_bmp(fileName, image.width, image.height, channels, imgData);
    else if (IsFileExtension(fileName, ".tga")) success = stbi_write_tga(fileName, image.width, image.height, channels, imgData);
    else if (IsFileExtension(fileName, ".jpg")) success = stbi_write_jpg(fileName, image.width, image.height, channels, imgData, 90);  // JPG quality: between 1 and 100
    else if (IsFileExtension(fileName, ".raw"))
    {
        // Export raw pixel data (without header)
        // NOTE: It's up to the user to track image parameters
        success = SaveFileData(fileName, image.data, GetPixelDataSize(image.width, image.height, image.format));
    }

    if (allocatedData) RL_FREE(imgData);


    if (success != 0) TRACELOG(LOG_INFO, "FILEIO: [%s] Image exported successfully", fileName);
    else TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to export image", fileName);

    return success;
}

// Export image as code file (.h) defining an array of bytes
bool ExportImageAsCode(Image image, const char *fileName)
{
    bool success = false;


    #define TEXT_BYTES_PER_LINE     20

    int dataSize = GetPixelDataSize(image.width, image.height, image.format);

    // NOTE: Text data buffer size is estimated considering image data size in bytes
    // and requiring 6 char bytes for every byte: "0x00, "
    char *txtData = (char *)RL_CALLOC(6*dataSize + 2000, sizeof(char));

    int bytesCount = 0;
    bytesCount += sprintf(txtData + bytesCount, "////////////////////////////////////////////////////////////////////////////////////////\n");
    bytesCount += sprintf(txtData + bytesCount, "//                                                                                    //\n");
    bytesCount += sprintf(txtData + bytesCount, "// ImageAsCode exporter v1.0 - Image pixel data exported as an array of bytes         //\n");
    bytesCount += sprintf(txtData + bytesCount, "//                                                                                    //\n");
    bytesCount += sprintf(txtData + bytesCount, "// more info and bugs-report:  github.com/raysan5/raylib                              //\n");
    bytesCount += sprintf(txtData + bytesCount, "// feedback and support:       ray[at]raylib.com                                      //\n");
    bytesCount += sprintf(txtData + bytesCount, "//                                                                                    //\n");
    bytesCount += sprintf(txtData + bytesCount, "// Copyright (c) 2020 Ramon Santamaria (@raysan5)                                     //\n");
    bytesCount += sprintf(txtData + bytesCount, "//                                                                                    //\n");
    bytesCount += sprintf(txtData + bytesCount, "////////////////////////////////////////////////////////////////////////////////////////\n\n");

    // Get file name from path and convert variable name to uppercase
    char varFileName[256] = { 0 };
    strcpy(varFileName, GetFileNameWithoutExt(fileName));
    for (int i = 0; varFileName[i] != '\0'; i++) if ((varFileName[i] >= 'a') && (varFileName[i] <= 'z')) { varFileName[i] = varFileName[i] - 32; }

    // Add image information
    bytesCount += sprintf(txtData + bytesCount, "// Image data information\n");
    bytesCount += sprintf(txtData + bytesCount, "#define %s_WIDTH    %i\n", varFileName, image.width);
    bytesCount += sprintf(txtData + bytesCount, "#define %s_HEIGHT   %i\n", varFileName, image.height);
    bytesCount += sprintf(txtData + bytesCount, "#define %s_FORMAT   %i          // raylib internal pixel format\n\n", varFileName, image.format);

    bytesCount += sprintf(txtData + bytesCount, "static unsigned char %s_DATA[%i] = { ", varFileName, dataSize);
    for (int i = 0; i < dataSize - 1; i++) bytesCount += sprintf(txtData + bytesCount, ((i%TEXT_BYTES_PER_LINE == 0)? "0x%x,\n" : "0x%x, "), ((unsigned char *)image.data)[i]);
    bytesCount += sprintf(txtData + bytesCount, "0x%x };\n", ((unsigned char *)image.data)[dataSize - 1]);

    // NOTE: Text data length exported is determined by '\0' (NULL) character
    success = SaveFileText(fileName, txtData);

    RL_FREE(txtData);

    return success;
}

// Clear alpha channel to desired color
// NOTE: Threshold defines the alpha limit, 0.0f to 1.0f
void ImageAlphaClear(Image *image, Color color, float threshold)
{
    // Security check to avoid program crash
    if ((image->data == NULL) || (image->width == 0) || (image->height == 0)) return;

    if (image->mipmaps > 1) TRACELOG(LOG_WARNING, "Image manipulation only applied to base mipmap level");
        switch (image->format)
        {
            case PIXELFORMAT_GRAY_ALPHA:
            {
                unsigned char thresholdValue = (unsigned char)(threshold*255.0f);
                for (int i = 1; i < image->width*image->height*2; i += 2)
                {
                    if (((unsigned char *)image->data)[i] <= thresholdValue)
                    {
                        ((unsigned char *)image->data)[i - 1] = color.r;
                        ((unsigned char *)image->data)[i] = color.a;
                    }
                }
            } break;
            case PIXELFORMAT_R5G5B5A1:
            {
                unsigned char thresholdValue = ((threshold < 0.5f)? 0 : 1);

                unsigned char r = (unsigned char)(round((float)color.r*31.0f));
                unsigned char g = (unsigned char)(round((float)color.g*31.0f));
                unsigned char b = (unsigned char)(round((float)color.b*31.0f));
                unsigned char a = (color.a < 128)? 0 : 1;

                for (int i = 0; i < image->width*image->height; i++)
                {
                    if ((((unsigned short *)image->data)[i] & 0b0000000000000001) <= thresholdValue)
                    {
                        ((unsigned short *)image->data)[i] = (unsigned short)r << 11 | (unsigned short)g << 6 | (unsigned short)b << 1 | (unsigned short)a;
                    }
                }
            } break;
            case PIXELFORMAT_R4G4B4A4:
            {
                unsigned char thresholdValue = (unsigned char)(threshold*15.0f);

                unsigned char r = (unsigned char)(round((float)color.r*15.0f));
                unsigned char g = (unsigned char)(round((float)color.g*15.0f));
                unsigned char b = (unsigned char)(round((float)color.b*15.0f));
                unsigned char a = (unsigned char)(round((float)color.a*15.0f));

                for (int i = 0; i < image->width*image->height; i++)
                {
                    if ((((unsigned short *)image->data)[i] & 0x000f) <= thresholdValue)
                    {
                        ((unsigned short *)image->data)[i] = (unsigned short)r << 12 | (unsigned short)g << 8 | (unsigned short)b << 4 | (unsigned short)a;
                    }
                }
            } break;
            case PIXELFORMAT_R8G8B8A8:
            {
                unsigned char thresholdValue = (unsigned char)(threshold*255.0f);
                for (int i = 3; i < image->width*image->height*4; i += 4)
                {
                    if (((unsigned char *)image->data)[i] <= thresholdValue)
                    {
                        ((unsigned char *)image->data)[i - 3] = color.r;
                        ((unsigned char *)image->data)[i - 2] = color.g;
                        ((unsigned char *)image->data)[i - 1] = color.b;
                        ((unsigned char *)image->data)[i] = color.a;
                    }
                }
            } break;
            case PIXELFORMAT_R32G32B32A32:
            {
                for (int i = 3; i < image->width*image->height*4; i += 4)
                {
                    if (((float *)image->data)[i] <= threshold)
                    {
                        ((float *)image->data)[i - 3] = (float)color.r/255.0f;
                        ((float *)image->data)[i - 2] = (float)color.g/255.0f;
                        ((float *)image->data)[i - 1] = (float)color.b/255.0f;
                        ((float *)image->data)[i] = (float)color.a/255.0f;
                    }
                }
            } break;
            default: break;
        }

}
Image ImageCopy(Image image)
{
    Image newImage = { 0 };

    int width = image.width;
    int height = image.height;
    int size = 0;

    for (int i = 0; i < image.mipmaps; i++)
    {
        size += GetPixelDataSize(width, height, image.format);

        width /= 2;
        height /= 2;

        // Security check for NPOT textures
        if (width < 1) width = 1;
        if (height < 1) height = 1;
    }

    newImage.data = RL_MALLOC(size);

    if (newImage.data != NULL)
    {
        // NOTE: Size must be provided in bytes
        memcpy(newImage.data, image.data, size);

        newImage.width = image.width;
        newImage.height = image.height;
        newImage.mipmaps = image.mipmaps;
        newImage.format = image.format;
    }

    return newImage;
}

// Get pixel data from image as Vector4 array (float normalized)
static Vector4 *LoadImageDataNormalized(Image image)
{
    Vector4 *pixels = (Vector4 *)RL_MALLOC(image.width*image.height*sizeof(Vector4));


        for (int i = 0, k = 0; i < image.width*image.height; i++)
        {
            switch (image.format)
            {
                case PIXELFORMAT_GRAYSCALE:
                {
                    pixels[i].x = (float)((unsigned char *)image.data)[i]/255.0f;
                    pixels[i].y = (float)((unsigned char *)image.data)[i]/255.0f;
                    pixels[i].z = (float)((unsigned char *)image.data)[i]/255.0f;
                    pixels[i].w = 1.0f;

                } break;
                case PIXELFORMAT_GRAY_ALPHA:
                {
                    pixels[i].x = (float)((unsigned char *)image.data)[k]/255.0f;
                    pixels[i].y = (float)((unsigned char *)image.data)[k]/255.0f;
                    pixels[i].z = (float)((unsigned char *)image.data)[k]/255.0f;
                    pixels[i].w = (float)((unsigned char *)image.data)[k + 1]/255.0f;

                    k += 2;
                } break;
                case PIXELFORMAT_R5G5B5A1:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].x = (float)((pixel & 0b1111100000000000) >> 11)*(1.0f/31);
                    pixels[i].y = (float)((pixel & 0b0000011111000000) >> 6)*(1.0f/31);
                    pixels[i].z = (float)((pixel & 0b0000000000111110) >> 1)*(1.0f/31);
                    pixels[i].w = ((pixel & 0b0000000000000001) == 0)? 0.0f : 1.0f;

                } break;
                case PIXELFORMAT_R5G6B5:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].x = (float)((pixel & 0b1111100000000000) >> 11)*(1.0f/31);
                    pixels[i].y = (float)((pixel & 0b0000011111100000) >> 5)*(1.0f/63);
                    pixels[i].z = (float)(pixel & 0b0000000000011111)*(1.0f/31);
                    pixels[i].w = 1.0f;

                } break;
                case PIXELFORMAT_R4G4B4A4:
                {
                    unsigned short pixel = ((unsigned short *)image.data)[i];

                    pixels[i].x = (float)((pixel & 0b1111000000000000) >> 12)*(1.0f/15);
                    pixels[i].y = (float)((pixel & 0b0000111100000000) >> 8)*(1.0f/15);
                    pixels[i].z = (float)((pixel & 0b0000000011110000) >> 4)*(1.0f/15);
                    pixels[i].w = (float)(pixel & 0b0000000000001111)*(1.0f/15);

                } break;
                case PIXELFORMAT_R8G8B8A8:
                {
                    pixels[i].x = (float)((unsigned char *)image.data)[k]/255.0f;
                    pixels[i].y = (float)((unsigned char *)image.data)[k + 1]/255.0f;
                    pixels[i].z = (float)((unsigned char *)image.data)[k + 2]/255.0f;
                    pixels[i].w = (float)((unsigned char *)image.data)[k + 3]/255.0f;

                    k += 4;
                } break;
                case PIXELFORMAT_R8G8B8:
                {
                    pixels[i].x = (float)((unsigned char *)image.data)[k]/255.0f;
                    pixels[i].y = (float)((unsigned char *)image.data)[k + 1]/255.0f;
                    pixels[i].z = (float)((unsigned char *)image.data)[k + 2]/255.0f;
                    pixels[i].w = 1.0f;

                    k += 3;
                } break;
                case PIXELFORMAT_R32:
                {
                    pixels[i].x = ((float *)image.data)[k];
                    pixels[i].y = 0.0f;
                    pixels[i].z = 0.0f;
                    pixels[i].w = 1.0f;

                } break;
                case PIXELFORMAT_R32G32B32:
                {
                    pixels[i].x = ((float *)image.data)[k];
                    pixels[i].y = ((float *)image.data)[k + 1];
                    pixels[i].z = ((float *)image.data)[k + 2];
                    pixels[i].w = 1.0f;

                    k += 3;
                } break;
                case PIXELFORMAT_R32G32B32A32:
                {
                    pixels[i].x = ((float *)image.data)[k];
                    pixels[i].y = ((float *)image.data)[k + 1];
                    pixels[i].z = ((float *)image.data)[k + 2];
                    pixels[i].w = ((float *)image.data)[k + 3];

                    k += 4;
                }
                default: break;
            }

    }

    return pixels;
}

// Convert image data to desired format
void ImageFormat(Image *image, int newFormat)
{
    // Security check to avoid program crash
    if ((image->data == NULL) || (image->width == 0) || (image->height == 0)) return;

    if ((newFormat != 0) && (image->format != newFormat))
    {
           Vector4 *pixels = LoadImageDataNormalized(*image);     // Supports 8 to 32 bit per channel

            RL_FREE(image->data);      // WARNING! We loose mipmaps data --> Regenerated at the end...
            image->data = NULL;
            image->format = newFormat;

            int k = 0;

            switch (image->format)
            {
                case PIXELFORMAT_GRAYSCALE:
                {
                    image->data = (unsigned char *)RL_MALLOC(image->width*image->height*sizeof(unsigned char));

                    for (int i = 0; i < image->width*image->height; i++)
                    {
                        ((unsigned char *)image->data)[i] = (unsigned char)((pixels[i].x*0.299f + pixels[i].y*0.587f + pixels[i].z*0.114f)*255.0f);
                    }

                } break;
                case PIXELFORMAT_GRAY_ALPHA:
                {
                    image->data = (unsigned char *)RL_MALLOC(image->width*image->height*2*sizeof(unsigned char));

                    for (int i = 0; i < image->width*image->height*2; i += 2, k++)
                    {
                        ((unsigned char *)image->data)[i] = (unsigned char)((pixels[k].x*0.299f + (float)pixels[k].y*0.587f + (float)pixels[k].z*0.114f)*255.0f);
                        ((unsigned char *)image->data)[i + 1] = (unsigned char)(pixels[k].w*255.0f);
                    }

                } break;
                case PIXELFORMAT_R5G6B5:
                {
                    image->data = (unsigned short *)RL_MALLOC(image->width*image->height*sizeof(unsigned short));

                    unsigned char r = 0;
                    unsigned char g = 0;
                    unsigned char b = 0;

                    for (int i = 0; i < image->width*image->height; i++)
                    {
                        r = (unsigned char)(round(pixels[i].x*31.0f));
                        g = (unsigned char)(round(pixels[i].y*63.0f));
                        b = (unsigned char)(round(pixels[i].z*31.0f));

                        ((unsigned short *)image->data)[i] = (unsigned short)r << 11 | (unsigned short)g << 5 | (unsigned short)b;
                    }

                } break;
                case PIXELFORMAT_R8G8B8:
                {
                    image->data = (unsigned char *)RL_MALLOC(image->width*image->height*3*sizeof(unsigned char));

                    for (int i = 0, k = 0; i < image->width*image->height*3; i += 3, k++)
                    {
                        ((unsigned char *)image->data)[i] = (unsigned char)(pixels[k].x*255.0f);
                        ((unsigned char *)image->data)[i + 1] = (unsigned char)(pixels[k].y*255.0f);
                        ((unsigned char *)image->data)[i + 2] = (unsigned char)(pixels[k].z*255.0f);
                    }
                } break;
                case PIXELFORMAT_R5G5B5A1:
                {
                    image->data = (unsigned short *)RL_MALLOC(image->width*image->height*sizeof(unsigned short));

                    unsigned char r = 0;
                    unsigned char g = 0;
                    unsigned char b = 0;
                    unsigned char a = 0;

                    for (int i = 0; i < image->width*image->height; i++)
                    {
                        r = (unsigned char)(round(pixels[i].x*31.0f));
                        g = (unsigned char)(round(pixels[i].y*31.0f));
                        b = (unsigned char)(round(pixels[i].z*31.0f));
                        a = (pixels[i].w > ((float)PIXELFORMAT_R5G5B5A1_ALPHA_THRESHOLD/255.0f))? 1 : 0;

                        ((unsigned short *)image->data)[i] = (unsigned short)r << 11 | (unsigned short)g << 6 | (unsigned short)b << 1 | (unsigned short)a;
                    }

                } break;
                case PIXELFORMAT_R4G4B4A4:
                {
                    image->data = (unsigned short *)RL_MALLOC(image->width*image->height*sizeof(unsigned short));

                    unsigned char r = 0;
                    unsigned char g = 0;
                    unsigned char b = 0;
                    unsigned char a = 0;

                    for (int i = 0; i < image->width*image->height; i++)
                    {
                        r = (unsigned char)(round(pixels[i].x*15.0f));
                        g = (unsigned char)(round(pixels[i].y*15.0f));
                        b = (unsigned char)(round(pixels[i].z*15.0f));
                        a = (unsigned char)(round(pixels[i].w*15.0f));

                        ((unsigned short *)image->data)[i] = (unsigned short)r << 12 | (unsigned short)g << 8 | (unsigned short)b << 4 | (unsigned short)a;
                    }

                } break;
                case PIXELFORMAT_R8G8B8A8:
                {
                    image->data = (unsigned char *)RL_MALLOC(image->width*image->height*4*sizeof(unsigned char));

                    for (int i = 0, k = 0; i < image->width*image->height*4; i += 4, k++)
                    {
                        ((unsigned char *)image->data)[i] = (unsigned char)(pixels[k].x*255.0f);
                        ((unsigned char *)image->data)[i + 1] = (unsigned char)(pixels[k].y*255.0f);
                        ((unsigned char *)image->data)[i + 2] = (unsigned char)(pixels[k].z*255.0f);
                        ((unsigned char *)image->data)[i + 3] = (unsigned char)(pixels[k].w*255.0f);
                    }
                } break;
                case PIXELFORMAT_R32:
                {
                    // WARNING: Image is converted to GRAYSCALE eqeuivalent 32bit

                    image->data = (float *)RL_MALLOC(image->width*image->height*sizeof(float));

                    for (int i = 0; i < image->width*image->height; i++)
                    {
                        ((float *)image->data)[i] = (float)(pixels[i].x*0.299f + pixels[i].y*0.587f + pixels[i].z*0.114f);
                    }
                } break;
                case PIXELFORMAT_R32G32B32:
                {
                    image->data = (float *)RL_MALLOC(image->width*image->height*3*sizeof(float));

                    for (int i = 0, k = 0; i < image->width*image->height*3; i += 3, k++)
                    {
                        ((float *)image->data)[i] = pixels[k].x;
                        ((float *)image->data)[i + 1] = pixels[k].y;
                        ((float *)image->data)[i + 2] = pixels[k].z;
                    }
                } break;
                case PIXELFORMAT_R32G32B32A32:
                {
                    image->data = (float *)RL_MALLOC(image->width*image->height*4*sizeof(float));

                    for (int i = 0, k = 0; i < image->width*image->height*4; i += 4, k++)
                    {
                        ((float *)image->data)[i] = pixels[k].x;
                        ((float *)image->data)[i + 1] = pixels[k].y;
                        ((float *)image->data)[i + 2] = pixels[k].z;
                        ((float *)image->data)[i + 3] = pixels[k].w;
                    }
                } break;
                default: break;
            }

            RL_FREE(pixels);
            pixels = NULL;





    }
}

// Apply alpha mask to image
// NOTE 1: Returned image is GRAY_ALPHA (16bit) or RGBA (32bit)
// NOTE 2: alphaMask should be same size as image
void ImageAlphaMask(Image *image, Image alphaMask)
{
    if ((image->width != alphaMask.width) || (image->height != alphaMask.height))
    {
        TRACELOG(LOG_WARNING, "IMAGE: Alpha mask must be same size as image");
    }
    else
    {
        // Force mask to be Grayscale
        Image mask = ImageCopy(alphaMask);
        if (mask.format != PIXELFORMAT_GRAYSCALE) ImageFormat(&mask, PIXELFORMAT_GRAYSCALE);

        // In case image is only grayscale, we just add alpha channel
        if (image->format == PIXELFORMAT_GRAYSCALE)
        {
            unsigned char *data = (unsigned char *)RL_MALLOC(image->width*image->height*2);

            // Apply alpha mask to alpha channel
            for (int i = 0, k = 0; (i < mask.width*mask.height) || (i < image->width*image->height); i++, k += 2)
            {
                data[k] = ((unsigned char *)image->data)[i];
                data[k + 1] = ((unsigned char *)mask.data)[i];
            }

            RL_FREE(image->data);
            image->data = data;
            image->format = PIXELFORMAT_GRAY_ALPHA;
        }
        else
        {
            // Convert image to RGBA
            if (image->format != PIXELFORMAT_R8G8B8A8) ImageFormat(image, PIXELFORMAT_R8G8B8A8);

            // Apply alpha mask to alpha channel
            for (int i = 0, k = 3; (i < mask.width*mask.height) || (i < image->width*image->height); i++, k += 4)
            {
                ((unsigned char *)image->data)[k] = ((unsigned char *)mask.data)[i];
            }
        }

        UnloadImage(mask);
    }
}


// Returns color with alpha applied, alpha goes from 0.0f to 1.0f
Color ColorAlpha(Color color, float alpha)
{
    if (alpha < 0.0f) alpha = 0.0f;
    else if (alpha > 1.0f) alpha = 1.0f;

    return (Color){color.r, color.g, color.b, (unsigned char)(255.0f*alpha)};
}

// Returns src alpha-blended into dst color with tint
Color ColorAlphaBlend(Color dst, Color src, Color tint)
{
    Color out = WHITE;

    // Apply color tint to source color
    src.r = (unsigned char)(((unsigned int)src.r*(unsigned int)tint.r) >> 8);
    src.g = (unsigned char)(((unsigned int)src.g*(unsigned int)tint.g) >> 8);
    src.b = (unsigned char)(((unsigned int)src.b*(unsigned int)tint.b) >> 8);
    src.a = (unsigned char)(((unsigned int)src.a*(unsigned int)tint.a) >> 8);

//#define COLORALPHABLEND_FLOAT
#define COLORALPHABLEND_INTEGERS
#if defined(COLORALPHABLEND_INTEGERS)
    if (src.a == 0) out = dst;
    else if (src.a == 255) out = src;
    else
    {
        unsigned int alpha = (unsigned int)src.a + 1;     // We are shifting by 8 (dividing by 256), so we need to take that excess into account
        out.a = (unsigned char)(((unsigned int)alpha*256 + (unsigned int)dst.a*(256 - alpha)) >> 8);

        if (out.a > 0)
        {
            out.r = (unsigned char)((((unsigned int)src.r*alpha*256 + (unsigned int)dst.r*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
            out.g = (unsigned char)((((unsigned int)src.g*alpha*256 + (unsigned int)dst.g*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
            out.b = (unsigned char)((((unsigned int)src.b*alpha*256 + (unsigned int)dst.b*(unsigned int)dst.a*(256 - alpha))/out.a) >> 8);
        }
    }
#endif
#if defined(COLORALPHABLEND_FLOAT)
    if (src.a == 0) out = dst;
    else if (src.a == 255) out = src;
    else
    {
        Vector4 fdst = ColorNormalize(dst);
        Vector4 fsrc = ColorNormalize(src);
        Vector4 ftint = ColorNormalize(tint);
        Vector4 fout = { 0 };

        fout.w = fsrc.w + fdst.w*(1.0f - fsrc.w);

        if (fout.w > 0.0f)
        {
            fout.x = (fsrc.x*fsrc.w + fdst.x*fdst.w*(1 - fsrc.w))/fout.w;
            fout.y = (fsrc.y*fsrc.w + fdst.y*fdst.w*(1 - fsrc.w))/fout.w;
            fout.z = (fsrc.z*fsrc.w + fdst.z*fdst.w*(1 - fsrc.w))/fout.w;
        }

        out = (Color){ (unsigned char)(fout.x*255.0f), (unsigned char)(fout.y*255.0f), (unsigned char)(fout.z*255.0f), (unsigned char)(fout.w*255.0f) };
    }
#endif

    return out;
}




// Returns color normalized as float [0..1]
Vector4 ColorNormalize(Color color)
{
    Vector4 result;

    result.x = (float)color.r/255.0f;
    result.y = (float)color.g/255.0f;
    result.z = (float)color.b/255.0f;
    result.w = (float)color.a/255.0f;

    return result;
}

// Returns color from normalized values [0..1]
Color ColorFromNormalized(Vector4 normalized)
{
    Color result;

    result.r = (unsigned char)(normalized.x*255.0f);
    result.g = (unsigned char)(normalized.y*255.0f);
    result.b = (unsigned char)(normalized.z*255.0f);
    result.a = (unsigned char)(normalized.w*255.0f);

    return result;
}

Texture2D LoadTextureFromImage(Image image)
{
    Texture2D texture = { 0 };

    if (!gRenderer) return texture;




int depth, pitch;
Uint32 pixel_format;
int imgbpp=0;

if (image.format == PIXELFORMAT_GRAY_ALPHA)
{

    depth = 2;
    pitch = 2*image.width;
    imgbpp=2*8;
    pixel_format = SDL_PIXELFORMAT_RGBA4444;
  //  SDL_Log("GRAY");


}else if (image.format == PIXELFORMAT_R8G8B8)
{
    depth = 24;
    pitch = 3*image.width; // 3 bytes per pixel * pixels per row
    imgbpp=3*8;
    pixel_format = SDL_PIXELFORMAT_RGB24;
   // SDL_Log("RGB");
}else if (image.format == PIXELFORMAT_R8G8B8A8)
{
    depth = 32;
    pitch = 4*image.width;
    imgbpp=4*8;
    pixel_format = SDL_PIXELFORMAT_RGBA32;
  //  SDL_Log("RGBA");
}


SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom((void*)image.data,  image.width, image.height,   depth, pitch, pixel_format);

//SDL_Surface* surface = SDL_CreateRGBSurfaceFrom((void*)image.data,  image.width, image.height,   depth, pitch,rmask, gmask, bmask, amask);
if (depth==2)
{
SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0,0,255));
}
//SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 0));
/*
SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
SDL_Surface *output = SDL_ConvertSurface(surface, format, 0);
SDL_FreeFormat(format);

SDL_FreeSurface(surface);
surface = NULL;

surface=output;*/



//img.format*8, img.format*img.w,

//SDL_Surface* surface =   SDL_CreateRGBSurfaceFrom((void*)image.data,  image.width, image.height,   imgbpp,pitch,

	//                                rmask, gmask, bmask, amask);


//SDL_Surface *surface = SDL_CreateRGBSurface(0, image.width, image.height, 32, rmask, gmask, bmask, amask);

if (surface == NULL)
{
    SDL_Log( "CreateRGBSurface failed: %s\n", SDL_GetError());
 //   exit(1);
}

    texture.id     = SDL_CreateTextureFromSurface(gRenderer, surface);
  //  SDL_SetTextureBlendMode(texture.id , SDL_BLENDMODE_BLEND);
   // SDL_SetTextureAlphaMod(texture.id , 255);

   SDL_SetTextureColorMod(texture.id, 255,255,255);
   SDL_SetTextureAlphaMod(texture.id, 255);

    texture.width   = image.width;
    texture.height  = image.height;
    texture.mipmaps = image.mipmaps;
    texture.format  = image.format;






SDL_FreeSurface(surface);
surface = NULL;



    return texture;
}



//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
#define MAX_TEXT_BUFFER_LENGTH              1024        // Size of internal static buffers used on some functions:
#define MAX_TEXT_UNICODE_CHARS               512        // Maximum number of unicode codepoints: GetCodepoints()
#define MAX_TEXTSPLIT_COUNT                  128        // Maximum number of substrings to split: TextSplit()

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
static Font defaultFont = { 0 };
static Font LoadBMFont(const char *fileName);     // Load a BMFont file (AngelCode font file)

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------



// Measure string size for Font
Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing)
{
    int len = TextLength(text);
    int tempLen = 0;                // Used to count longer text line num chars
    int lenCounter = 0;

    float textWidth = 0.0f;
    float tempTextWidth = 0.0f;     // Used to count longer text line width

    float textHeight = (float)font.baseSize;
    float scaleFactor = fontSize/(float)font.baseSize;

    int letter = 0;                 // Current character
    int index = 0;                  // Index position in sprite font

    for (int i = 0; i < len; i++)
    {
        lenCounter++;

        int next = 0;
        letter = GetNextCodepoint(&text[i], &next);
        index = GetGlyphIndex(font, letter);

        // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
        if (letter == 0x3f) next = 1;
        i += next - 1;

        if (letter != '\n')
        {
            if (font.chars[index].advanceX != 0) textWidth += font.chars[index].advanceX;
            else textWidth += (font.recs[index].width + font.chars[index].offsetX);
        }
        else
        {
            if (tempTextWidth < textWidth) tempTextWidth = textWidth;
            lenCounter = 0;
            textWidth = 0;
            textHeight += ((float)font.baseSize*1.5f); // NOTE: Fixed line spacing of 1.5 lines
        }

        if (tempLen < lenCounter) tempLen = lenCounter;
    }

    if (tempTextWidth < textWidth) tempTextWidth = textWidth;

    Vector2 vec = { 0 };
    vec.x = tempTextWidth*scaleFactor + (float)((tempLen - 1)*spacing); // Adds chars spacing to measure
    vec.y = textHeight*scaleFactor;

    return vec;
}
// Measure string width for default font
int MeasureText(const char *text, int fontSize)
{
    Vector2 vec = { 0.0f, 0.0f };

    // Check if default font has been loaded
    if (GetFontDefault().texture.id != 0)
    {
        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize/defaultFontSize;

        vec = MeasureTextEx(GetFontDefault(), text, (float)fontSize, (float)spacing);
    }

    return (int)vec.x;
}
// Returns index position for a unicode character on spritefont
int GetGlyphIndex(Font font, int codepoint)
{
    #define GLYPH_NOTFOUND_CHAR_FALLBACK     63      // Character used if requested codepoint is not found: '?'

// Support charsets with any characters order
    int index = GLYPH_NOTFOUND_CHAR_FALLBACK;

    for (int i = 0; i < font.charsCount; i++)
    {
        if (font.chars[i].value == codepoint)
        {
            index = i;
            break;
        }
    }

    return index;

}

// Modify image color: replace color
void ImageColorReplace(Image *image, Color color, Color replace)
{
    // Security check to avoid program crash
    if ((image->data == NULL) || (image->width == 0) || (image->height == 0)) return;

    Color *pixels = LoadImageColors(*image);

    for (int y = 0; y < image->height; y++)
    {
        for (int x = 0; x < image->width; x++)
        {
            if ((pixels[y*image->width + x].r == color.r) &&
                (pixels[y*image->width + x].g == color.g) &&
                (pixels[y*image->width + x].b == color.b) &&
                (pixels[y*image->width + x].a == color.a))
            {
                pixels[y*image->width + x].r = replace.r;
                pixels[y*image->width + x].g = replace.g;
                pixels[y*image->width + x].b = replace.b;
                pixels[y*image->width + x].a = replace.a;
            }
        }
    }

    int format = image->format;
    RL_FREE(image->data);

    image->data = pixels;
    image->format = PIXELFORMAT_R8G8B8A8;

    ImageFormat(image, format);
}

// Load raylib default font
 void LoadFontDefault()
{
    #define BIT_CHECK(a,b) ((a) & (1u << (b)))

    // NOTE: Using UTF8 encoding table for Unicode U+0000..U+00FF Basic Latin + Latin-1 Supplement
    // Ref: http://www.utf8-chartable.de/unicode-utf8-table.pl

    defaultFont.charsCount = 224;   // Number of chars included in our default font
    defaultFont.charsPadding = 0;   // Characters padding

    // Default font is directly defined here (data generated from a sprite font image)
    // This way, we reconstruct Font without creating large global variables
    // This data is automatically allocated to Stack and automatically deallocated at the end of this function
    unsigned int defaultFontData[512] = {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200020, 0x0001b000, 0x00000000, 0x00000000, 0x8ef92520, 0x00020a00, 0x7dbe8000, 0x1f7df45f,
        0x4a2bf2a0, 0x0852091e, 0x41224000, 0x10041450, 0x2e292020, 0x08220812, 0x41222000, 0x10041450, 0x10f92020, 0x3efa084c, 0x7d22103c, 0x107df7de,
        0xe8a12020, 0x08220832, 0x05220800, 0x10450410, 0xa4a3f000, 0x08520832, 0x05220400, 0x10450410, 0xe2f92020, 0x0002085e, 0x7d3e0281, 0x107df41f,
        0x00200000, 0x8001b000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xc0000fbe, 0xfbf7e00f, 0x5fbf7e7d, 0x0050bee8, 0x440808a2, 0x0a142fe8, 0x50810285, 0x0050a048,
        0x49e428a2, 0x0a142828, 0x40810284, 0x0048a048, 0x10020fbe, 0x09f7ebaf, 0xd89f3e84, 0x0047a04f, 0x09e48822, 0x0a142aa1, 0x50810284, 0x0048a048,
        0x04082822, 0x0a142fa0, 0x50810285, 0x0050a248, 0x00008fbe, 0xfbf42021, 0x5f817e7d, 0x07d09ce8, 0x00008000, 0x00000fe0, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x000c0180,
        0xdfbf4282, 0x0bfbf7ef, 0x42850505, 0x004804bf, 0x50a142c6, 0x08401428, 0x42852505, 0x00a808a0, 0x50a146aa, 0x08401428, 0x42852505, 0x00081090,
        0x5fa14a92, 0x0843f7e8, 0x7e792505, 0x00082088, 0x40a15282, 0x08420128, 0x40852489, 0x00084084, 0x40a16282, 0x0842022a, 0x40852451, 0x00088082,
        0xc0bf4282, 0xf843f42f, 0x7e85fc21, 0x3e0900bf, 0x00000000, 0x00000004, 0x00000000, 0x000c0180, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x04000402, 0x41482000, 0x00000000, 0x00000800,
        0x04000404, 0x4100203c, 0x00000000, 0x00000800, 0xf7df7df0, 0x514bef85, 0xbefbefbe, 0x04513bef, 0x14414500, 0x494a2885, 0xa28a28aa, 0x04510820,
        0xf44145f0, 0x474a289d, 0xa28a28aa, 0x04510be0, 0x14414510, 0x494a2884, 0xa28a28aa, 0x02910a00, 0xf7df7df0, 0xd14a2f85, 0xbefbe8aa, 0x011f7be0,
        0x00000000, 0x00400804, 0x20080000, 0x00000000, 0x00000000, 0x00600f84, 0x20080000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0xac000000, 0x00000f01, 0x00000000, 0x00000000, 0x24000000, 0x00000f01, 0x00000000, 0x06000000, 0x24000000, 0x00000f01, 0x00000000, 0x09108000,
        0x24fa28a2, 0x00000f01, 0x00000000, 0x013e0000, 0x2242252a, 0x00000f52, 0x00000000, 0x038a8000, 0x2422222a, 0x00000f29, 0x00000000, 0x010a8000,
        0x2412252a, 0x00000f01, 0x00000000, 0x010a8000, 0x24fbe8be, 0x00000f01, 0x00000000, 0x0ebe8000, 0xac020000, 0x00000f01, 0x00000000, 0x00048000,
        0x0003e000, 0x00000f00, 0x00000000, 0x00008000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000038, 0x8443b80e, 0x00203a03,
        0x02bea080, 0xf0000020, 0xc452208a, 0x04202b02, 0xf8029122, 0x07f0003b, 0xe44b388e, 0x02203a02, 0x081e8a1c, 0x0411e92a, 0xf4420be0, 0x01248202,
        0xe8140414, 0x05d104ba, 0xe7c3b880, 0x00893a0a, 0x283c0e1c, 0x04500902, 0xc4400080, 0x00448002, 0xe8208422, 0x04500002, 0x80400000, 0x05200002,
        0x083e8e00, 0x04100002, 0x804003e0, 0x07000042, 0xf8008400, 0x07f00003, 0x80400000, 0x04000022, 0x00000000, 0x00000000, 0x80400000, 0x04000002,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00800702, 0x1848a0c2, 0x84010000, 0x02920921, 0x01042642, 0x00005121, 0x42023f7f, 0x00291002,
        0xefc01422, 0x7efdfbf7, 0xefdfa109, 0x03bbbbf7, 0x28440f12, 0x42850a14, 0x20408109, 0x01111010, 0x28440408, 0x42850a14, 0x2040817f, 0x01111010,
        0xefc78204, 0x7efdfbf7, 0xe7cf8109, 0x011111f3, 0x2850a932, 0x42850a14, 0x2040a109, 0x01111010, 0x2850b840, 0x42850a14, 0xefdfbf79, 0x03bbbbf7,
        0x001fa020, 0x00000000, 0x00001000, 0x00000000, 0x00002070, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x08022800, 0x00012283, 0x02430802, 0x01010001, 0x8404147c, 0x20000144, 0x80048404, 0x00823f08, 0xdfbf4284, 0x7e03f7ef, 0x142850a1, 0x0000210a,
        0x50a14684, 0x528a1428, 0x142850a1, 0x03efa17a, 0x50a14a9e, 0x52521428, 0x142850a1, 0x02081f4a, 0x50a15284, 0x4a221428, 0xf42850a1, 0x03efa14b,
        0x50a16284, 0x4a521428, 0x042850a1, 0x0228a17a, 0xdfbf427c, 0x7e8bf7ef, 0xf7efdfbf, 0x03efbd0b, 0x00000000, 0x04000000, 0x00000000, 0x00000008,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00200508, 0x00840400, 0x11458122, 0x00014210,
        0x00514294, 0x51420800, 0x20a22a94, 0x0050a508, 0x00200000, 0x00000000, 0x00050000, 0x08000000, 0xfefbefbe, 0xfbefbefb, 0xfbeb9114, 0x00fbefbe,
        0x20820820, 0x8a28a20a, 0x8a289114, 0x3e8a28a2, 0xfefbefbe, 0xfbefbe0b, 0x8a289114, 0x008a28a2, 0x228a28a2, 0x08208208, 0x8a289114, 0x088a28a2,
        0xfefbefbe, 0xfbefbefb, 0xfa2f9114, 0x00fbefbe, 0x00000000, 0x00000040, 0x00000000, 0x00000000, 0x00000000, 0x00000020, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00210100, 0x00000004, 0x00000000, 0x00000000, 0x14508200, 0x00001402, 0x00000000, 0x00000000,
        0x00000010, 0x00000020, 0x00000000, 0x00000000, 0xa28a28be, 0x00002228, 0x00000000, 0x00000000, 0xa28a28aa, 0x000022e8, 0x00000000, 0x00000000,
        0xa28a28aa, 0x000022a8, 0x00000000, 0x00000000, 0xa28a28aa, 0x000022e8, 0x00000000, 0x00000000, 0xbefbefbe, 0x00003e2f, 0x00000000, 0x00000000,
        0x00000004, 0x00002028, 0x00000000, 0x00000000, 0x80000000, 0x00003e0f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };

    int charsHeight = 10;
    int charsDivisor = 1;    // Every char is separated from the consecutive by a 1 pixel divisor, horizontally and vertically

    int charsWidth[224] = { 3, 1, 4, 6, 5, 7, 6, 2, 3, 3, 5, 5, 2, 4, 1, 7, 5, 2, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 3, 4, 3, 6,
                            7, 6, 6, 6, 6, 6, 6, 6, 6, 3, 5, 6, 5, 7, 6, 6, 6, 6, 6, 6, 7, 6, 7, 7, 6, 6, 6, 2, 7, 2, 3, 5,
                            2, 5, 5, 5, 5, 5, 4, 5, 5, 1, 2, 5, 2, 5, 5, 5, 5, 5, 5, 5, 4, 5, 5, 5, 5, 5, 5, 3, 1, 3, 4, 4,
                            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                            1, 1, 5, 5, 5, 7, 1, 5, 3, 7, 3, 5, 4, 1, 7, 4, 3, 5, 3, 3, 2, 5, 6, 1, 2, 2, 3, 5, 6, 6, 6, 6,
                            6, 6, 6, 6, 6, 6, 7, 6, 6, 6, 6, 6, 3, 3, 3, 3, 7, 6, 6, 6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 6,
                            5, 5, 5, 5, 5, 5, 9, 5, 5, 5, 5, 5, 2, 2, 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 5 };

    // Re-construct image from defaultFontData and generate OpenGL texture
    //----------------------------------------------------------------------
    Image imFont = {
        .data = RL_CALLOC(128*128, 2),  // 2 bytes per pixel (gray + alpha)
        .width = 128,
        .height = 128,
        .mipmaps = 1,
        .format = PIXELFORMAT_GRAY_ALPHA,

    };

    // Fill image.data with defaultFontData (convert from bit to pixel!)
    for (int i = 0, counter = 0; i < imFont.width*imFont.height; i += 32)
    {
        for (int j = 31; j >= 0; j--)
        {
            if (BIT_CHECK(defaultFontData[counter], j))
            {
                // NOTE: We are unreferencing data as short, so,
                // we must consider data as little-endian order (alpha + gray)
                ((unsigned short *)imFont.data)[i + j] = 0xffff;
            }
            else ((unsigned short *)imFont.data)[i + j] = 0x00ff;
        }

        counter++;
    }

    //ExportImageAsCode(imFont,"font.txt");
   // ExportImage(imFont,"font.p");


  //ImageColorReplace(&imFont,GetColor(0x0000ffff),GetColor(0x000000ff));

    defaultFont.texture = LoadTextureFromImage(imFont);

    // Reconstruct charSet using charsWidth[], charsHeight, charsDivisor, charsCount
    //------------------------------------------------------------------------------

    // Allocate space for our characters info data
    // NOTE: This memory should be freed at end! --> CloseWindow()
    defaultFont.chars = (CharInfo *)RL_MALLOC(defaultFont.charsCount*sizeof(CharInfo));
    defaultFont.recs = (Rectangle *)RL_MALLOC(defaultFont.charsCount*sizeof(Rectangle));

    int currentLine = 0;
    int currentPosX = charsDivisor;
    int testPosX = charsDivisor;

    for (int i = 0; i < defaultFont.charsCount; i++)
    {
        defaultFont.chars[i].value = 32 + i;  // First char is 32

        defaultFont.recs[i].x = (float)currentPosX;
        defaultFont.recs[i].y = (float)(charsDivisor + currentLine*(charsHeight + charsDivisor));
        defaultFont.recs[i].width = (float)charsWidth[i];
        defaultFont.recs[i].height = (float)charsHeight;

        testPosX += (int)(defaultFont.recs[i].width + (float)charsDivisor);

        if (testPosX >= defaultFont.texture.width)
        {
            currentLine++;
            currentPosX = 2*charsDivisor + charsWidth[i];
            testPosX = currentPosX;

            defaultFont.recs[i].x = (float)charsDivisor;
            defaultFont.recs[i].y = (float)(charsDivisor + currentLine*(charsHeight + charsDivisor));
        }
        else currentPosX = testPosX;

        // NOTE: On default font character offsets and xAdvance are not required
        defaultFont.chars[i].offsetX = 0;
        defaultFont.chars[i].offsetY = 0;
        defaultFont.chars[i].advanceX = 0;

        // Fill character image data from fontClear data
        defaultFont.chars[i].image = ImageFromImage(imFont, defaultFont.recs[i]);
    }

    UnloadImage(imFont);

    defaultFont.baseSize = (int)defaultFont.recs[0].height;

    TRACELOG(LOG_INFO, "FONT: Default font loaded successfully");
}

// Unload raylib default font
 void UnloadFontDefault(void)
{
    for (int i = 0; i < defaultFont.charsCount; i++) UnloadImage(defaultFont.chars[i].image);
    UnloadTexture(defaultFont.texture);
    RL_FREE(defaultFont.chars);
    RL_FREE(defaultFont.recs);
}




// Get the default font, useful to be used with extended parameters
Font GetFontDefault()
{
    return defaultFont;

}


// Read a line from memory
// REQUIRES: memcpy()
// NOTE: Returns the number of bytes read
static int GetLine(const char *origin, char *buffer, int maxLength)
{
    int count = 0;
    for (; count < maxLength; count++) if (origin[count] == '\n') break;
    memcpy(buffer, origin, count);
    return count;
}

// Load a BMFont file (AngelCode font file)
// REQUIRES: strstr(), sscanf(), strrchr(), memcpy()
static Font LoadBMFont(const char *fileName)
{
    #define MAX_BUFFER_SIZE     256

    Font font = { 0 };

    char buffer[MAX_BUFFER_SIZE] = { 0 };
    char *searchPoint = NULL;

    int fontSize = 0;
    int charsCount = 0;

    int imWidth = 0;
    int imHeight = 0;
    char imFileName[129];

    int base = 0;   // Useless data

    char *fileText = LoadFileText(fileName);

    if (fileText == NULL) return font;

    char *fileTextPtr = fileText;

    // NOTE: We skip first line, it contains no useful information
    int lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
    fileTextPtr += (lineBytes + 1);

    // Read line data
    lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
    searchPoint = strstr(buffer, "lineHeight");
    sscanf(searchPoint, "lineHeight=%i base=%i scaleW=%i scaleH=%i", &fontSize, &base, &imWidth, &imHeight);
    fileTextPtr += (lineBytes + 1);

    TRACELOGD("FONT: [%s] Loaded font info:", fileName);
    TRACELOGD("    > Base size: %i", fontSize);
    TRACELOGD("    > Texture scale: %ix%i", imWidth, imHeight);

    lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
    searchPoint = strstr(buffer, "file");
    sscanf(searchPoint, "file=\"%128[^\"]\"", imFileName);
    fileTextPtr += (lineBytes + 1);

    TRACELOGD("    > Texture filename: %s", imFileName);

    lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
    searchPoint = strstr(buffer, "count");
    sscanf(searchPoint, "count=%i", &charsCount);
    fileTextPtr += (lineBytes + 1);

    TRACELOGD("    > Chars count: %i", charsCount);

    // Compose correct path using route of .fnt file (fileName) and imFileName
    char *imPath = NULL;
    char *lastSlash = NULL;

    lastSlash = (char*)strrchr(fileName, '/');
    if (lastSlash == NULL) lastSlash = (char*)strrchr(fileName, '\\');

    if (lastSlash != NULL)
    {
        // NOTE: We need some extra space to avoid memory corruption on next allocations!
        imPath =(char*) RL_CALLOC(TextLength(fileName) - TextLength(lastSlash) + TextLength(imFileName) + 4, 1);
        memcpy(imPath, fileName, TextLength(fileName) - TextLength(lastSlash) + 1);
        memcpy(imPath + TextLength(fileName) - TextLength(lastSlash) + 1, imFileName, TextLength(imFileName));
    }
    else imPath = imFileName;

    TRACELOGD("    > Image loading path: %s", imPath);

    Image imFont = LoadImage(imPath);

    if (imFont.format == PIXELFORMAT_GRAYSCALE)
    {
        // Convert image to GRAYSCALE + ALPHA, using the mask as the alpha channel
        Image imFontAlpha = {
            .data = RL_CALLOC(imFont.width*imFont.height, 2),
            .width = imFont.width,
            .height = imFont.height,
            .mipmaps = 1,
            .format = PIXELFORMAT_GRAY_ALPHA,

        };

        for (int p = 0, i = 0; p < (imFont.width*imFont.height*2); p += 2, i++)
        {
            ((unsigned char *)(imFontAlpha.data))[p] = 0xff;
            ((unsigned char *)(imFontAlpha.data))[p + 1] = ((unsigned char *)imFont.data)[i];
        }

        UnloadImage(imFont);
        imFont = imFontAlpha;
    }

    font.texture = LoadTextureFromImage(imFont);

    if (lastSlash != NULL) RL_FREE(imPath);

    // Fill font characters info data
    font.baseSize = fontSize;
    font.charsCount = charsCount;
    font.charsPadding = 0;
    font.chars = (CharInfo *)RL_MALLOC(charsCount*sizeof(CharInfo));
    font.recs = (Rectangle *)RL_MALLOC(charsCount*sizeof(Rectangle));

    int charId, charX, charY, charWidth, charHeight, charOffsetX, charOffsetY, charAdvanceX;

    for (int i = 0; i < charsCount; i++)
    {
        lineBytes = GetLine(fileTextPtr, buffer, MAX_BUFFER_SIZE);
        sscanf(buffer, "char id=%i x=%i y=%i width=%i height=%i xoffset=%i yoffset=%i xadvance=%i",
                       &charId, &charX, &charY, &charWidth, &charHeight, &charOffsetX, &charOffsetY, &charAdvanceX);
        fileTextPtr += (lineBytes + 1);

        // Get character rectangle in the font atlas texture
        font.recs[i] = (Rectangle){ (float)charX, (float)charY, (float)charWidth, (float)charHeight };

        // Save data properly in sprite font
        font.chars[i].value = charId;
        font.chars[i].offsetX = charOffsetX;
        font.chars[i].offsetY = charOffsetY;
        font.chars[i].advanceX = charAdvanceX;

        // Fill character image data from imFont data
        font.chars[i].image = ImageFromImage(imFont, font.recs[i]);
    }

    UnloadImage(imFont);
    RL_FREE(fileText);

    if (font.texture.id == 0)
    {
        UnloadFont(font);
        font = GetFontDefault();
        TRACELOG(LOG_WARNING, "FONT: [%s] Failed to load texture, reverted to default font", fileName);
    }
    else TRACELOG(LOG_INFO, "FONT: [%s] Font loaded successfully", fileName);

    return font;
}
// Load Font from file into GPU memory (VRAM)
Font LoadFont(const char *fileName)
{
    // Default values for ttf font generation
    #define FONT_TTF_DEFAULT_SIZE           32      // TTF font generation default char size (char-height)
    #define FONT_TTF_DEFAULT_NUMCHARS       95      // TTF font generation default charset: 95 glyphs (ASCII 32..126)
    #define FONT_TTF_DEFAULT_FIRST_CHAR     32      // TTF font generation default first char for image sprite font (32-Space)
    #define FONT_TTF_DEFAULT_CHARS_PADDING   4      // TTF font generation default chars padding

    Font font = { 0 };


    if (IsFileExtension(fileName, ".ttf;.otf")) font = LoadFontEx(fileName, FONT_TTF_DEFAULT_SIZE, NULL, FONT_TTF_DEFAULT_NUMCHARS);
    else
    if (IsFileExtension(fileName, ".fnt")) font = LoadBMFont(fileName);
    else
    {
        Image image = LoadImage(fileName);
        if (image.data != NULL) font = LoadFontFromImage(image, MAGENTA, FONT_TTF_DEFAULT_FIRST_CHAR);
        UnloadImage(image);
    }

    if (font.texture.id == 0)
    {
        TRACELOG(0, "FONT: [%s] Failed to load font texture -> Using default font", fileName);
        font = GetFontDefault();
    }

    return font;
}

// Load Font from TTF font file with generation parameters
// NOTE: You can pass an array with desired characters, those characters should be available in the font
// if array is NULL, default char set is selected 32..126
Font LoadFontEx(const char *fileName, int fontSize, int *fontChars, int charsCount)
{
    Font font = { 0 };

    // Loading file to memory
    unsigned int fileSize = 0;
    unsigned char *fileData = LoadFileData(fileName, &fileSize);





    if (fileData != NULL)
    {
        // Loading font from memory data
        font = LoadFontFromMemory(GetFileExtension(fileName), fileData, fileSize, fontSize, fontChars, charsCount);

       // SDL_Log("file size %d %d %d ",fileSize,charsCount,fontSize);

        RL_FREE(fileData);
    }
    else
    font = GetFontDefault();

    return font;
}

// Load an Image font file (XNA style)
Font LoadFontFromImage(Image image, Color key, int firstChar)
{
    #define MAX_GLYPHS_FROM_IMAGE   256     // Maximum number of glyphs supported on image scan

    #define COLOR_EQUAL(col1, col2) ((col1.r == col2.r)&&(col1.g == col2.g)&&(col1.b == col2.b)&&(col1.a == col2.a))

    int charSpacing = 0;
    int lineSpacing = 0;

    int x = 0;
    int y = 0;

    // We allocate a temporal arrays for chars data measures,
    // once we get the actual number of chars, we copy data to a sized arrays
    int tempCharValues[MAX_GLYPHS_FROM_IMAGE];
    Rectangle tempCharRecs[MAX_GLYPHS_FROM_IMAGE];

    Color *pixels = LoadImageColors(image);

    // Parse image data to get charSpacing and lineSpacing
    for (y = 0; y < image.height; y++)
    {
        for (x = 0; x < image.width; x++)
        {
            if (!COLOR_EQUAL(pixels[y*image.width + x], key)) break;
        }

        if (!COLOR_EQUAL(pixels[y*image.width + x], key)) break;
    }

    charSpacing = x;
    lineSpacing = y;

    int charHeight = 0;
    int j = 0;

    while (!COLOR_EQUAL(pixels[(lineSpacing + j)*image.width + charSpacing], key)) j++;

    charHeight = j;

    // Check array values to get characters: value, x, y, w, h
    int index = 0;
    int lineToRead = 0;
    int xPosToRead = charSpacing;

    // Parse image data to get rectangle sizes
    while ((lineSpacing + lineToRead*(charHeight + lineSpacing)) < image.height)
    {
        while ((xPosToRead < image.width) &&
              !COLOR_EQUAL((pixels[(lineSpacing + (charHeight+lineSpacing)*lineToRead)*image.width + xPosToRead]), key))
        {
            tempCharValues[index] = firstChar + index;

            tempCharRecs[index].x = (float)xPosToRead;
            tempCharRecs[index].y = (float)(lineSpacing + lineToRead*(charHeight + lineSpacing));
            tempCharRecs[index].height = (float)charHeight;

            int charWidth = 0;

            while (!COLOR_EQUAL(pixels[(lineSpacing + (charHeight+lineSpacing)*lineToRead)*image.width + xPosToRead + charWidth], key)) charWidth++;

            tempCharRecs[index].width = (float)charWidth;

            index++;

            xPosToRead += (charWidth + charSpacing);
        }

        lineToRead++;
        xPosToRead = charSpacing;
    }

    // NOTE: We need to remove key color borders from image to avoid weird
    // artifacts on texture scaling when using TEXTURE_FILTER_BILINEAR or TEXTURE_FILTER_TRILINEAR
    for (int i = 0; i < image.height*image.width; i++) if (COLOR_EQUAL(pixels[i], key)) pixels[i] = BLANK;

    // Create a new image with the processed color data (key color replaced by BLANK)
    Image fontClear = {
        .data = pixels,
        .width = image.width,
        .height = image.height,
        .mipmaps = 1,
        .format = PIXELFORMAT_R8G8B8A8,

    };

    // Create spritefont with all data parsed from image
    Font font = { 0 };

    font.texture = LoadTextureFromImage(fontClear); // Convert processed image to OpenGL texture
    font.charsCount = index;
    font.charsPadding = 0;

    // We got tempCharValues and tempCharsRecs populated with chars data
    // Now we move temp data to sized charValues and charRecs arrays
    font.chars = (CharInfo *)RL_MALLOC(font.charsCount*sizeof(CharInfo));
    font.recs = (Rectangle *)RL_MALLOC(font.charsCount*sizeof(Rectangle));

    for (int i = 0; i < font.charsCount; i++)
    {
        font.chars[i].value = tempCharValues[i];

        // Get character rectangle in the font atlas texture
        font.recs[i] = tempCharRecs[i];

        // NOTE: On image based fonts (XNA style), character offsets and xAdvance are not required (set to 0)
        font.chars[i].offsetX = 0;
        font.chars[i].offsetY = 0;
        font.chars[i].advanceX = 0;

        // Fill character image data from fontClear data
        font.chars[i].image = ImageFromImage(fontClear, tempCharRecs[i]);
    }

    UnloadImage(fontClear);     // Unload processed image once converted to texture

    font.baseSize = (int)font.recs[0].height;

    return font;
}

// Load font from memory buffer, fileType refers to extension: i.e. ".ttf"
Font LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int charsCount)
{
    Font font = { 0 };

    char fileExtLower[16] = { 0 };
    strcpy(fileExtLower, TextToLower(fileType));




    if (TextIsEqual(fileExtLower, "ttf") ||
        TextIsEqual(fileExtLower, "otf"))
    {
        font.baseSize = fontSize;
        font.charsCount = (charsCount > 0)? charsCount : 95;
        font.charsPadding = 0;

        font.chars = LoadFontData(fileData, dataSize, font.baseSize, fontChars, font.charsCount, FONT_DEFAULT);

        if (font.chars != NULL)
        {
            font.charsPadding = FONT_TTF_DEFAULT_CHARS_PADDING;


            Image atlas = GenImageFontAtlas(font.chars, &font.recs, font.charsCount, font.baseSize, font.charsPadding, 0);

            font.texture = LoadTextureFromImage(atlas);

            // Update chars[i].image to use alpha, required to be used on ImageDrawText()
            for (int i = 0; i < font.charsCount; i++)
            {
                UnloadImage(font.chars[i].image);
                font.chars[i].image = ImageFromImage(atlas, font.recs[i]);
            }

            UnloadImage(atlas);
        }
        else font = GetFontDefault();
    } else{

      SDL_Log("Font unknow type.");
    }


    return font;
}

// Load font data for further use
// NOTE: Requires TTF font memory data and can generate SDF data
CharInfo *LoadFontData(const unsigned char *fileData, int dataSize, int fontSize, int *fontChars, int charsCount, int type)
{
    // NOTE: Using some SDF generation default values,
    // trades off precision with ability to handle *smaller* sizes
    #define FONT_SDF_CHAR_PADDING            4      // SDF font generation char padding
    #define FONT_SDF_ON_EDGE_VALUE         128      // SDF font generation on edge value
    #define FONT_SDF_PIXEL_DIST_SCALE     64.0f     // SDF font generation pixel distance scale
    #define FONT_BITMAP_ALPHA_THRESHOLD     80      // Bitmap (B&W) font generation alpha threshold

    CharInfo *chars = NULL;


    // Load font data (including pixel data) from TTF memory file
    // NOTE: Loaded information should be enough to generate font image atlas, using any packaging method
    if (fileData != NULL)
    {
        int genFontChars = false;
        stbtt_fontinfo fontInfo = { 0 };

        if (stbtt_InitFont(&fontInfo, (unsigned char *)fileData, 0))     // Init font for data reading
        {
            // Calculate font scale factor
            float scaleFactor = stbtt_ScaleForPixelHeight(&fontInfo, (float)fontSize);

            // Calculate font basic metrics
            // NOTE: ascent is equivalent to font baseline
            int ascent, descent, lineGap;
            stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

            // In case no chars count provided, default to 95
            charsCount = (charsCount > 0)? charsCount : 95;

            // Fill fontChars in case not provided externally
            // NOTE: By default we fill charsCount consecutevely, starting at 32 (Space)

            if (fontChars == NULL)
            {
                fontChars = (int *)RL_MALLOC(charsCount*sizeof(int));
                for (int i = 0; i < charsCount; i++) fontChars[i] = i + 32;
                genFontChars = true;
            }

            chars = (CharInfo *)RL_MALLOC(charsCount*sizeof(CharInfo));

            // NOTE: Using simple packaging, one char after another
            for (int i = 0; i < charsCount; i++)
            {
                int chw = 0, chh = 0;   // Character width and height (on generation)
                int ch = fontChars[i];  // Character value to get info for
                chars[i].value = ch;

                //  Render a unicode codepoint to a bitmap
                //      stbtt_GetCodepointBitmap()           -- allocates and returns a bitmap
                //      stbtt_GetCodepointBitmapBox()        -- how big the bitmap must be
                //      stbtt_MakeCodepointBitmap()          -- renders into bitmap you provide

                if (type != FONT_SDF) chars[i].image.data = stbtt_GetCodepointBitmap(&fontInfo, scaleFactor, scaleFactor, ch, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);
                else if (ch != 32) chars[i].image.data = stbtt_GetCodepointSDF(&fontInfo, scaleFactor, ch, FONT_SDF_CHAR_PADDING, FONT_SDF_ON_EDGE_VALUE, FONT_SDF_PIXEL_DIST_SCALE, &chw, &chh, &chars[i].offsetX, &chars[i].offsetY);
                else chars[i].image.data = NULL;

                stbtt_GetCodepointHMetrics(&fontInfo, ch, &chars[i].advanceX, NULL);
                chars[i].advanceX = (int)((float)chars[i].advanceX*scaleFactor);

                // Load characters images
                chars[i].image.width = chw;
                chars[i].image.height = chh;
                chars[i].image.mipmaps = 1;
                chars[i].image.format = PIXELFORMAT_GRAYSCALE;

                chars[i].offsetY += (int)((float)ascent*scaleFactor);

                // NOTE: We create an empty image for space character, it could be further required for atlas packing
                if (ch == 32)
                {
                    Image imSpace = {
                        .data = RL_CALLOC(chars[i].advanceX*fontSize, 2),
                        .width = chars[i].advanceX,
                        .height = fontSize,
                        .mipmaps = 1,
                        .format = PIXELFORMAT_GRAYSCALE,

                    };

                    chars[i].image = imSpace;
                }

                if (type == FONT_BITMAP)
                {
                    // Aliased bitmap (black & white) font generation, avoiding anti-aliasing
                    // NOTE: For optimum results, bitmap font should be generated at base pixel size
                    for (int p = 0; p < chw*chh; p++)
                    {
                        if (((unsigned char *)chars[i].image.data)[p] < FONT_BITMAP_ALPHA_THRESHOLD) ((unsigned char *)chars[i].image.data)[p] = 0;
                        else ((unsigned char *)chars[i].image.data)[p] = 255;
                    }
                }


            }
        }
        else TRACELOG(0, "FONT: Failed to process TTF font data");

        if (genFontChars) RL_FREE(fontChars);
    }


    return chars;
}

// Generate image font atlas using chars info
// NOTE: Packing method: 0-Default, 1-Skyline

Image GenImageFontAtlas(const CharInfo *chars, Rectangle **charRecs, int charsCount, int fontSize, int padding, int packMethod)
{
    Image atlas = { 0 };

    if (chars == NULL)
    {
        SDL_Log( "FONT: Provided chars info not valid, returning empty image atlas");
        return atlas;
    }

    *charRecs = NULL;

    // In case no chars count provided we suppose default of 95
    charsCount = (charsCount > 0)? charsCount : 95;

    // NOTE: Rectangles memory is loaded here!
    Rectangle *recs = (Rectangle *)RL_MALLOC(charsCount*sizeof(Rectangle));

    // Calculate image size based on required pixel area
    // NOTE 1: Image is forced to be squared and POT... very conservative!
    // NOTE 2: SDF font characters already contain an internal padding,
    // so image size would result bigger than default font type
    float requiredArea = 0;
    for (int i = 0; i < charsCount; i++) requiredArea += ((chars[i].image.width + 2*padding)*(chars[i].image.height + 2*padding));
    float guessSize = sqrtf(requiredArea)*1.3f;
    int imageSize = (int)powf(2, ceilf(logf((float)guessSize)/logf(2)));  // Calculate next POT

    atlas.width = imageSize;   // Atlas bitmap width
    atlas.height = imageSize;  // Atlas bitmap height
    atlas.data = (unsigned char *)RL_CALLOC(1, atlas.width*atlas.height);      // Create a bitmap to store characters (8 bpp)
    atlas.format = PIXELFORMAT_GRAYSCALE;
    atlas.mipmaps = 1;

    // DEBUG: We can see padding in the generated image setting a gray background...
    //for (int i = 0; i < atlas.width*atlas.height; i++) ((unsigned char *)atlas.data)[i] = 100;

    if (packMethod == 0)   // Use basic packing algorythm
    {
        int offsetX = padding;
        int offsetY = padding;

        // NOTE: Using simple packaging, one char after another
        for (int i = 0; i < charsCount; i++)
        {
            // Copy pixel data from fc.data to atlas
            for (int y = 0; y < chars[i].image.height; y++)
            {
                for (int x = 0; x < chars[i].image.width; x++)
                {
                    ((unsigned char *)atlas.data)[(offsetY + y)*atlas.width + (offsetX + x)] = ((unsigned char *)chars[i].image.data)[y*chars[i].image.width + x];
                }
            }

            // Fill chars rectangles in atlas info
            recs[i].x = (float)offsetX;
            recs[i].y = (float)offsetY;
            recs[i].width = (float)chars[i].image.width;
            recs[i].height = (float)chars[i].image.height;

            // Move atlas position X for next character drawing
            offsetX += (chars[i].image.width + 2*padding);

            if (offsetX >= (atlas.width - chars[i].image.width - 2*padding))
            {
                offsetX = padding;

                // NOTE: Be careful on offsetY for SDF fonts, by default SDF
                // use an internal padding of 4 pixels, it means char rectangle
                // height is bigger than fontSize, it could be up to (fontSize + 8)
                offsetY += (fontSize + 2*padding);

                if (offsetY > (atlas.height - fontSize - padding)) break;
            }
        }
    }
    else if (packMethod == 1)  // Use Skyline rect packing algorythm (stb_pack_rect)
    {
        stbrp_context *context = (stbrp_context *)RL_MALLOC(sizeof(*context));
        stbrp_node *nodes = (stbrp_node *)RL_MALLOC(charsCount*sizeof(*nodes));

        stbrp_init_target(context, atlas.width, atlas.height, nodes, charsCount);
        stbrp_rect *rects = (stbrp_rect *)RL_MALLOC(charsCount*sizeof(stbrp_rect));

        // Fill rectangles for packaging
        for (int i = 0; i < charsCount; i++)
        {
            rects[i].id = i;
            rects[i].w = chars[i].image.width + 2*padding;
            rects[i].h = chars[i].image.height + 2*padding;
        }

        // Package rectangles into atlas
        stbrp_pack_rects(context, rects, charsCount);

        for (int i = 0; i < charsCount; i++)
        {
            // It return char rectangles in atlas
            recs[i].x = rects[i].x + (float)padding;
            recs[i].y = rects[i].y + (float)padding;
            recs[i].width = (float)chars[i].image.width;
            recs[i].height = (float)chars[i].image.height;

            if (rects[i].was_packed)
            {
                // Copy pixel data from fc.data to atlas
                for (int y = 0; y < chars[i].image.height; y++)
                {
                    for (int x = 0; x < chars[i].image.width; x++)
                    {
                        ((unsigned char *)atlas.data)[(rects[i].y + padding + y)*atlas.width + (rects[i].x + padding + x)] = ((unsigned char *)chars[i].image.data)[y*chars[i].image.width + x];
                    }
                }
            }
            else TRACELOG(0, "FONT: Failed to package character (%i)", i);
        }

        RL_FREE(rects);
        RL_FREE(nodes);
        RL_FREE(context);
    }

    // TODO: Crop image if required for smaller size

    // Convert image data from GRAYSCALE to GRAY_ALPHA
    unsigned char *dataGrayAlpha = (unsigned char *)RL_MALLOC(atlas.width*atlas.height*sizeof(unsigned char)*2); // Two channels

    for (int i = 0, k = 0; i < atlas.width*atlas.height; i++, k += 2)
    {
        dataGrayAlpha[k] = 255;
        dataGrayAlpha[k + 1] = ((unsigned char *)atlas.data)[i];
    }

    RL_FREE(atlas.data);
    atlas.data = dataGrayAlpha;
    atlas.format = PIXELFORMAT_GRAY_ALPHA;

    *charRecs = recs;

    return atlas;
}


// Unload font chars info data (RAM)
void UnloadFontData(CharInfo *chars, int charsCount)
{
    for (int i = 0; i < charsCount; i++) UnloadImage(chars[i].image);

    RL_FREE(chars);
}

// Unload Font from GPU memory (VRAM)
void UnloadFont(Font font)
{
    // NOTE: Make sure font is not default font (fallback)
    if (font.texture.id != GetFontDefault().texture.id)
    {
        UnloadFontData(font.chars, font.charsCount);
        UnloadTexture(font.texture);
        RL_FREE(font.recs);

        TRACELOGD("FONT: Unloaded font data from RAM and VRAM");
    }
}



// Draw text (using default font)
// NOTE: fontSize work like in any drawing program but if fontSize is lower than font-base-size, then font-base-size is used
// NOTE: chars spacing is proportional to fontSize
void DrawText(const char *text, int posX, int posY, int fontSize, Color color)
{



    // Check if default font has been loaded
    if (GetFontDefault().texture.id != 0)
    {
        Vector2 position = { (float)posX, (float)posY };

        int defaultFontSize = 10;   // Default Font chars height in pixel
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = fontSize/defaultFontSize;

        DrawTextEx(GetFontDefault(), text, position, (float)fontSize, (float)spacing, color);
    }
}

// Draw text using Font
// NOTE: chars spacing is NOT proportional to fontSize
void DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
{
    int length = TextLength(text);      // Total length in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/font.baseSize;     // Character quad scaling factor

    SDL_SetTextureColorMod(font.texture.id, (Uint8) tint.r, (Uint8) tint.g, (Uint8) tint.b);
    SDL_SetTextureAlphaMod(font.texture.id,(Uint8) tint.a);


    for (int i = 0; i < length;)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetNextCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;

        if (codepoint == '\n')
        {
            // NOTE: Fixed line spacing of 1.5 line-height
            // TODO: Support custom line spacing defined by user
            textOffsetY += (int)((font.baseSize + font.baseSize/2)*scaleFactor);
            textOffsetX = 0.0f;
        }
        else
        {
            if ((codepoint != ' ') && (codepoint != '\t'))
            {
                DrawTextCodepoint(font, codepoint, (Vector2){ position.x + textOffsetX, position.y + textOffsetY }, fontSize, tint);
            }

            if (font.chars[index].advanceX == 0) textOffsetX += ((float)font.recs[index].width*scaleFactor + spacing);
            else textOffsetX += ((float)font.chars[index].advanceX*scaleFactor + spacing);
        }

        i += codepointByteCount;   // Move text bytes counter to next codepoint
    }
}


void UnloadTexture(Texture2D texture)
{
    if (texture.id > 0)
    {

    SDL_DestroyTexture(texture.id);

        TRACELOG(0, "TEXTURE: [ID %i] Unloaded texture data from VRAM (GPU)", texture.id);
    }
}


// Draw one character (codepoint)
void DrawTextCodepoint(Font font, int codepoint, Vector2 position, float fontSize, Color tint)
{
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scaleFactor = fontSize/font.baseSize;     // Character quad scaling factor

    // Character destination rectangle on screen
    // NOTE: We consider charsPadding on drawing
    Rectangle dstRec = { position.x + font.chars[index].offsetX*scaleFactor - (float)font.charsPadding*scaleFactor,
                      position.y + font.chars[index].offsetY*scaleFactor - (float)font.charsPadding*scaleFactor,
                      (font.recs[index].width + 2.0f*font.charsPadding)*scaleFactor,
                      (font.recs[index].height + 2.0f*font.charsPadding)*scaleFactor };

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    Rectangle srcRec = { font.recs[index].x - (float)font.charsPadding, font.recs[index].y - (float)font.charsPadding,
                         font.recs[index].width + 2.0f*font.charsPadding, font.recs[index].height + 2.0f*font.charsPadding };

    // Draw the character texture on the screen
  //  DrawTexturePro(font.texture, srcRec, dstRec, (Vector2){ 0, 0 }, 0.0f, tint);


  SDL_Rect SrcR;
  SDL_Rect DestR;

  SrcR.x = (int)srcRec.x;
  SrcR.y = (int)srcRec.y;
  SrcR.w = (int)srcRec.width;
  SrcR.h = (int)srcRec.height;

  DestR.x = (int)dstRec.x;
  DestR.y = (int)dstRec.y;
  DestR.w = (int)dstRec.width;
  DestR.h = (int)dstRec.height;



	//	SDL_RenderCopy(gRenderer,tex.id, &SrcR, &DestR);

    SDL_RenderCopy(gRenderer,font.texture.id, &SrcR, &DestR);
}



// Draw text using font inside rectangle limits
void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
 SDL_SetRenderDrawColor(gRenderer, (Uint8) tint.r, (Uint8) tint.g, (Uint8) tint.b,(Uint8) tint.a);
    DrawTextRecEx(font, text, rec, fontSize, spacing, wordWrap, tint, 0, 0, WHITE, WHITE);
}

void DrawRectangleRec(Rectangle rec, Color color)
{

   SDL_Rect SrcR;

  SrcR.x = (int)rec.x;
  SrcR.y = (int)rec.y;
  SrcR.w = (int)rec.width;
  SrcR.h = (int)rec.height;

  SDL_RenderDrawRect(gRenderer,&SrcR);
}


// Draw text using font inside rectangle limits with support for text selection
void DrawTextRecEx(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint)
{
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    int textOffsetY = 0;            // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/font.baseSize;     // Character quad scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position

    for (int i = 0, k = 0; i < length; i++, k++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetNextCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        int glyphWidth = 0;
        if (codepoint != '\n')
        {
            glyphWidth = (font.chars[index].advanceX == 0)?
                         (int)(font.recs[index].width*scaleFactor + spacing):
                         (int)(font.chars[index].advanceX*scaleFactor + spacing);
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE)
        {
            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

            if ((textOffsetX + glyphWidth + 1) >= rec.width)
            {
                endLine = (endLine < 1)? i : endLine;
                if (i == endLine) endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                state = !state;
            }
            else if ((i + 1) == length)
            {
                endLine = i;

                state = !state;
            }
            else if (codepoint == '\n') state = !state;

            if (state == DRAW_STATE)
            {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        }
        else
        {
            if (codepoint == '\n')
            {
                if (!wordWrap)
                {
                    textOffsetY += (int)((font.baseSize + font.baseSize/2)*scaleFactor);
                    textOffsetX = 0;
                }
            }
            else
            {
                if (!wordWrap && ((textOffsetX + glyphWidth + 1) >= rec.width))
                {
                    textOffsetY += (int)((font.baseSize + font.baseSize/2)*scaleFactor);
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + (int)(font.baseSize*scaleFactor)) > rec.height) break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength)))
                {
                    DrawRectangleRec((Rectangle){ rec.x + textOffsetX - 1, rec.y + textOffsetY, (float)glyphWidth, (float)font.baseSize*scaleFactor }, selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t'))
                {
                    DrawTextCodepoint(font, codepoint, (Vector2){ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, isGlyphSelected? selectTint : tint);
                }
            }

            if (wordWrap && (i == endLine))
            {
                textOffsetY += (int)((font.baseSize + font.baseSize/2)*scaleFactor);
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
            }
        }

        textOffsetX += glyphWidth;
    }
}


SDL_Surface* LoadSurfaceFromFile(const char* filename)
{


int req_format = 4;
int width, height, orig_format;
void* data = stbi_load(filename, &width, &height, &orig_format, req_format);
if (data == NULL)
{
    SDL_Log("Loading image failed: %s", stbi_failure_reason());
    return NULL;
}


SDL_Log("Loading image : %s %d BPP  (%d %d)", filename,orig_format,width,height);


int depth, pitch;
Uint32 pixel_format;
if (req_format == 1)
{
    depth = 4;
    pitch = 1*width;
    pixel_format = SDL_PIXELFORMAT_INDEX8;
 //   SDL_Log("1 ALPHA");
} else if (req_format == 2)
{
    depth = 2;
    pitch = 2*width;
    pixel_format = SDL_PIXELFORMAT_RGBA4444;
  //  SDL_Log("2 GRAY");
}
else if (req_format == 3)
{
    depth = 24;
    pitch = 3*width;
    pixel_format = SDL_PIXELFORMAT_RGB24;
  //  SDL_Log("3 RGB");
} else if (req_format == 4)
{
    depth = 32;
    pitch = 4*width;
    pixel_format = SDL_PIXELFORMAT_RGBA32;
  //  SDL_Log("4 RGBA");
}



SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(data, width, height, depth, pitch, pixel_format);


if (surf == NULL)
{
    SDL_Log("Creating surface failed: %s", SDL_GetError());
    stbi_image_free(data);
    return NULL;
}



stbi_image_free(data);


return surf;


}

SDL_Texture* LoadTextureFromSurface(SDL_Surface* surface,bool discarSurface)
{
   if(surface)
   {
     SDL_Texture* tex = SDL_CreateTextureFromSurface(gRenderer, surface);

      if (discarSurface)
      {
        SDL_FreeSurface(surface);
        surface = NULL;
       }
     return tex;
   }
return NULL;
}


Color *LoadSurfaceColors(const SDL_Surface* image)
{
    if ((image->w == 0) || (image->h == 0)) return NULL;

    Color *pixels = (Color *)RL_MALLOC(image->w*image->h*sizeof(Color));

        // Security check to avoid program crash

    int channels = image->pitch/image->w;


        for (int i = 0, k = 0; i < image->w*image->h; i++)
        {
            switch (channels)
            {
                case 1:
                {
                    pixels[i].r = ((unsigned char *)image->pixels)[i];
                    pixels[i].g = ((unsigned char *)image->pixels)[i];
                    pixels[i].b = ((unsigned char *)image->pixels)[i];
                    pixels[i].a = 255;

                } break;
                case 2:
                {
                    pixels[i].r = ((unsigned char *)image->pixels)[k];
                    pixels[i].g = ((unsigned char *)image->pixels)[k];
                    pixels[i].b = ((unsigned char *)image->pixels)[k];
                    pixels[i].a = ((unsigned char *)image->pixels)[k + 1];

                    k += 2;
                } break;

                case 3:
                {
                    pixels[i].r = (unsigned char)((unsigned char *)image->pixels)[k];
                    pixels[i].g = (unsigned char)((unsigned char *)image->pixels)[k + 1];
                    pixels[i].b = (unsigned char)((unsigned char *)image->pixels)[k + 2];
                    pixels[i].a = 255;

                    k += 3;

                } break;
                case 4:
                {
                    pixels[i].r = ((unsigned char *)image->pixels)[k];
                    pixels[i].g = ((unsigned char *)image->pixels)[k + 1];
                    pixels[i].b = ((unsigned char *)image->pixels)[k + 2];
                    pixels[i].a = ((unsigned char *)image->pixels)[k + 3];

                    k += 4;
                } break;


            }
        }

    return pixels;
}


bool ExportSurface(const SDL_Surface* image, const char *fileName)
{
    int success = 0;


   int channels = image->pitch/image->w;
   SDL_Log("export %d", channels);



//SDL_LockSurface(image);

    bool allocatedData = false;
    unsigned char *imgData = (unsigned char *)image->pixels;

   // GetPixelDataSize(image.width, image.height, image.format)


    if (IsFileExtension(fileName, ".png")) success = stbi_write_png(fileName, image->w, image->h, channels, imgData, image->w*channels);
    else if (IsFileExtension(fileName, ".bmp")) success = stbi_write_bmp(fileName, image->w, image->h, channels, imgData);
    else if (IsFileExtension(fileName, ".tga")) success = stbi_write_tga(fileName, image->w, image->h, channels, imgData);
    else if (IsFileExtension(fileName, ".jpg")) success = stbi_write_jpg(fileName, image->w, image->h, channels, imgData, 90);  // JPG quality: between 1 and 100
    else if (IsFileExtension(fileName, ".raw"))
    {
        // Export raw pixel data (without header)
        // NOTE: It's up to the user to track image parameters
    //    success = SaveFileData(fileName, image.data, GetPixelDataSize(image.width, image.height, image.format));
    }


//SDL_UnlockSurface(image);

    if (success != 0) TRACELOG(LOG_INFO, "FILEIO: [%s] Image exported successfully", fileName);
    else TRACELOG(LOG_WARNING, "FILEIO: [%s] Failed to export image", fileName);

    return success;
}


void SurfaceDrawPixel(SDL_Surface *dst, int x, int y, Color color)
{
    // Security check to avoid program crash
    if ( (x < 0) || (x >= dst->w) || (y < 0) || (y >= dst->h)) return;

    int channels = dst->pitch/dst->w;

    switch (channels)
    {
        case 1:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dst->pixels)[y*dst->w + x] = gray;

        } break;
        case 2:
        {
            // NOTE: Calculate grayscale equivalent color
            Vector3 coln = { (float)color.r/255.0f, (float)color.g/255.0f, (float)color.b/255.0f };
            unsigned char gray = (unsigned char)((coln.x*0.299f + coln.y*0.587f + coln.z*0.114f)*255.0f);

            ((unsigned char *)dst->pixels)[(y*dst->w + x)*2] = gray;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*2 + 1] = color.a;

        } break;

        case 3:
        {
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*3] = color.r;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*3 + 1] = color.g;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*3 + 2] = color.b;

        } break;
        case 4:
        {
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*4] = color.r;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*4 + 1] = color.g;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*4 + 2] = color.b;
            ((unsigned char *)dst->pixels)[(y*dst->w + x)*4 + 3] = color.a;

        } break;

    }
}

void SurfaceClearBackground(SDL_Surface *dst, Color color)
{
    SDL_LockSurface(dst);
    for (int i = 0; i < dst->w*dst->h; ++i) SurfaceDrawPixel(dst, i%dst->w, i/dst->w, color);
    SDL_UnlockSurface(dst);

}

void SurfaceColorReplace(SDL_Surface *image, Color color, Color replace)
{
    // Security check to avoid program crash
    if ( (image->w == 0) || (image->h == 0)) return;

    SDL_LockSurface(image);
    Color *pixels = LoadSurfaceColors(image);

    for (int y = 0; y < image->h; y++)
    {
        for (int x = 0; x < image->w; x++)
        {

        Color pixel = pixels[y*image->w + x];



             if ((pixel.r == color.r) &&
                (pixel.g == color.g) &&
                (pixel.b == color.b) &&
                (pixel.a == color.a))
            {
                pixels[y*image->w + x].r = replace.r;
                pixels[y*image->w + x].g = replace.g;
                pixels[y*image->w + x].b = replace.b;
                pixels[y*image->w + x].a = replace.a;

           }
        }
    }


    image->pixels=pixels;
SDL_UnlockSurface(image);

}

void SurfaceAlphaClear(SDL_Surface *image, Color color, float threshold)
{
    // Security check to avoid program crash
    if ( (image->w == 0) || (image->h == 0)) return;

SDL_LockSurface(image);
    int channels = image->pitch/image->w;

        switch (channels)
        {
            case 1:
            {
             /*   unsigned char thresholdValue = (unsigned char)(threshold*255.0f);
                for (int i = 1; i < image->width*image->height*2; i += 2)
                {
                    if (((unsigned char *)image->data)[i] <= thresholdValue)
                    {
                        ((unsigned char *)image->data)[i - 1] = color.r;
                        ((unsigned char *)image->data)[i] = color.a;
                    }
                }*/
            } break;
            case 2:
            {
              /*  unsigned char thresholdValue = ((threshold < 0.5f)? 0 : 1);

                unsigned char r = (unsigned char)(round((float)color.r*31.0f));
                unsigned char g = (unsigned char)(round((float)color.g*31.0f));
                unsigned char b = (unsigned char)(round((float)color.b*31.0f));
                unsigned char a = (color.a < 128)? 0 : 1;

                for (int i = 0; i < image->width*image->height; i++)
                {
                    if ((((unsigned short *)image->data)[i] & 0b0000000000000001) <= thresholdValue)
                    {
                        ((unsigned short *)image->data)[i] = (unsigned short)r << 11 | (unsigned short)g << 6 | (unsigned short)b << 1 | (unsigned short)a;
                    }
                }*/
            } break;
            case 3:
            {
             /*   unsigned char thresholdValue = (unsigned char)(threshold*15.0f);

                unsigned char r = (unsigned char)(round((float)color.r*15.0f));
                unsigned char g = (unsigned char)(round((float)color.g*15.0f));
                unsigned char b = (unsigned char)(round((float)color.b*15.0f));
                unsigned char a = (unsigned char)(round((float)color.a*15.0f));

                for (int i = 0; i < image->width*image->height; i++)
                {
                    if ((((unsigned short *)image->data)[i] & 0x000f) <= thresholdValue)
                    {
                        ((unsigned short *)image->data)[i] = (unsigned short)r << 12 | (unsigned short)g << 8 | (unsigned short)b << 4 | (unsigned short)a;
                    }
                }*/
            } break;
            case 4:
            {
                unsigned char thresholdValue = (unsigned char)(threshold*255.0f);
                for (int i = 3; i < image->w*image->h*4; i += 4)
                {
                    if (((unsigned char *)image->pixels)[i] <= thresholdValue)
                    {
                        ((unsigned char *)image->pixels)[i - 3] = color.r;
                        ((unsigned char *)image->pixels)[i - 2] = color.g;
                        ((unsigned char *)image->pixels)[i - 1] = color.b;
                        ((unsigned char *)image->pixels)[i] = color.a;
                    }
                }
            } break;

        }
SDL_UnlockSurface(image);
}

SDL_Surface* SurfaceFromSurface( SDL_Surface* image, Rectangle rec)
{
    SDL_Surface* result ;



     int bytesPerPixel = GetSurfacePixelDataSize(1,1,image->pitch/image->w);
     int channels = image->pitch/image->w;



     SDL_Log("bbp %d channels %d",bytesPerPixel,channels);



    switch (channels)
    {
    case 1:result = SDL_CreateRGBSurfaceWithFormat(0,(int) rec.width, (int)rec.height, 8,   SDL_PIXELFORMAT_RGB332); break;
    case 2:result = SDL_CreateRGBSurfaceWithFormat(0,(int) rec.width, (int)rec.height, 16,  SDL_PIXELFORMAT_RGB565); break;
    case 3:result = SDL_CreateRGBSurfaceWithFormat(0,(int) rec.width, (int)rec.height, 24,  SDL_PIXELFORMAT_RGB24); break;
    case 4:result = SDL_CreateRGBSurfaceWithFormat(0,(int) rec.width, (int)rec.height, 32,  SDL_PIXELFORMAT_RGBA32); break;
    }




     SDL_LockSurface(image);
      for (int y = 0; y < rec.height; y++)
    {
        memcpy(((unsigned char *)result->pixels) + y*(int)rec.width*bytesPerPixel,
        ((unsigned char *)image->pixels) + ((y + (int)rec.y)*image->w + (int)rec.x)*bytesPerPixel,
        (int)rec.width*bytesPerPixel);
    }
    SDL_UnlockSurface(image);
    return result;
}


void ResizeSurfaceCanvas(SDL_Surface *image, int newWidth, int newHeight, int offsetX, int offsetY, Color fill)
{
    // Security check to avoid program crash
    if ( (image->w == 0) || (image->h == 0)) return;




    if ((newWidth != image->w) || (newHeight != image->h))
    {
        Rectangle srcRec = { 0, 0, (float)image->w, (float)image->h };
        Vector2 dstPos = { (float)offsetX, (float)offsetY };

        if (offsetX < 0)
        {
            srcRec.x = (float)-offsetX;
            srcRec.width += (float)offsetX;
            dstPos.x = 0;
        }
        else if ((offsetX + image->w) > newWidth) srcRec.width = (float)(newWidth - offsetX);

        if (offsetY < 0)
        {
            srcRec.y = (float)-offsetY;
            srcRec.height += (float)offsetY;
            dstPos.y = 0;
        }
        else if ((offsetY + image->h) > newHeight) srcRec.height = (float)(newHeight - offsetY);

        if (newWidth < srcRec.width) srcRec.width = (float)newWidth;
        if (newHeight < srcRec.height) srcRec.height = (float)newHeight;

        int bytesPerPixel =  GetSurfacePixelDataSize(1,1,image->pitch/image->w);
        unsigned char *resizedData = (unsigned char *)RL_CALLOC(newWidth*newHeight*bytesPerPixel, 1);

        // TODO: Fill resizedData with fill color (must be formatted to image->format)

        int dstOffsetSize = ((int)dstPos.y*newWidth + (int)dstPos.x)*bytesPerPixel;

        for (int y = 0; y < (int)srcRec.height; y++)
        {
            memcpy(resizedData + dstOffsetSize, ((unsigned char *)image->pixels) + ((y + (int)srcRec.y)*image->w + (int)srcRec.x)*bytesPerPixel, (int)srcRec.width*bytesPerPixel);
            dstOffsetSize += (newWidth*bytesPerPixel);
        }

        image->w=newWidth;
        image->h=newHeight;
      //  RL_FREE(image->data);
      //  image->data = resizedData;
      //  image->width = newWidth;
     //   image->height = newHeight;
    }
}


int GetSurfacePixelDataSize(int width, int height, int channels)
{
    int dataSize = 0;       // Size in bytes
    int bpp = 0;            // Bits per pixel

    int format =0;

    switch (channels)
    {
    case 1: format=PIXELFORMAT_GRAYSCALE; break;
    case 2: format=PIXELFORMAT_GRAY_ALPHA; break;
    case 3: format=PIXELFORMAT_R8G8B8; break;
    case 4: format=PIXELFORMAT_R8G8B8A8; break;
    }


    switch (format)
    {
        case PIXELFORMAT_GRAYSCALE: bpp = 8; break;
        case PIXELFORMAT_GRAY_ALPHA:
        case PIXELFORMAT_R5G6B5:
        case PIXELFORMAT_R5G5B5A1:
        case PIXELFORMAT_R4G4B4A4: bpp = 16; break;
        case PIXELFORMAT_R8G8B8A8: bpp = 32; break;
        case PIXELFORMAT_R8G8B8: bpp = 24; break;
        case PIXELFORMAT_R32: bpp = 32; break;
        case PIXELFORMAT_R32G32B32: bpp = 32*3; break;
        case PIXELFORMAT_R32G32B32A32: bpp = 32*4; break;

        default: break;
    }

    dataSize = width*height*bpp/8;

    return dataSize;
}



void DrawRectangleLines(int x, int y, int w, int h, Color c)
{

SDL_Rect sr ={x,y,w,h};
SDL_RenderDrawRect(gRenderer,&sr);
}
void DrawRectangleLinesEx(Rectangle rec, int lineThick, Color color)
{
    if ((lineThick > rec.width) || (lineThick > rec.height))
    {
        if (rec.width > rec.height) lineThick = (int)rec.height/2;
        else if (rec.width < rec.height) lineThick = (int)rec.width/2;
    }

    // When rec = { x, y, 8.0f, 6.0f } and lineThick = 2, the following
    // four rectangles are drawn ([T]op, [B]ottom, [L]eft, [R]ight):
    //
    //   TTTTTTTT
    //   TTTTTTTT
    //   LL    RR
    //   LL    RR
    //   BBBBBBBB
    //   BBBBBBBB
    //
    float thick = (float)lineThick;
    Rectangle top = { rec.x, rec.y, rec.width, thick };
    Rectangle bottom = { rec.x, rec.y - thick + rec.height, rec.width, thick };
    Rectangle left = { rec.x, rec.y + thick, thick, rec.height - thick*2.0f };
    Rectangle right = { rec.x - thick + rec.width, rec.y + thick, thick, rec.height - thick*2.0f };

    DrawRectangleRec(top, color);
    DrawRectangleRec(bottom, color);
    DrawRectangleRec(left, color);
    DrawRectangleRec(right, color);
}


void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color c)
{


	Sint16 vx[3];
	Sint16 vy[3];

	vx[0]=(Sint16)v1.x;
	vx[1]=(Sint16)v2.x;
	vx[2]=(Sint16)v3.x;
	vy[0]=(Sint16)v1.y;
	vy[1]=(Sint16)v2.y;
	vy[2]=(Sint16)v3.y;

//	filledPolygon(gRenderer,vx,vy,3);

/*
SDL_RenderDrawLineF(gRenderer,v1.x,v1.y,v2.x,v2.y);
SDL_RenderDrawLineF(gRenderer,v2.x,v2.y,v3.x,v3.y);
SDL_RenderDrawLineF(gRenderer,v3.x,v3.y,v1.x,v1.y);*/

}



void DrawRectangleGradientEx(Rectangle rec, Color col1, Color col2, Color col3, Color col4)
{

int steps=10;

 /* Acumulator initial position */
    float yt = rec.y;
    float rt = col1.r;
    float gt = col1.g;
    float bt = col1.b;
    float at = col1.a;

    /* Changes in each attribute */
    float ys = rec.height/steps;
    float rs = (col2.r - col1.r)/steps;
    float gs = (col2.g - col1.g)/steps;
    float bs = (col2.b - col1.b)/steps;
    float as = (col2.a - col1.a)/steps;

    for(int i = 0; i < steps ; i++)
    {
        /* Create an horizontal rectangle sliced by the number of steps */
        SDL_Rect rect = { rec.x, yt, rec.width, ys+1 };

        /* Sets the rectangle color based on iteration */
        SDL_SetRenderDrawColor(gRenderer, rt, gt, bt, at);

        /* Paint it or coverit*/
            SDL_RenderFillRect(gRenderer, &rect);

        /* Update colors and positions */
        yt += ys;
        rt += rs;
        gt += gs;
        bt += bs;
        at += as;
    }


}


void DrawRectangle(int x, int y, int w, int h, Color c)
{

SDL_Rect sr ={x,y,w,h};
SDL_RenderFillRect(gRenderer,&sr);
}

void DrawPoint(int x, int y, Color c)
{

SDL_RenderDrawPoint(gRenderer,x,y);

}

void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color c)
{

  SDL_RenderDrawLineF(gRenderer,startPos.x,startPos.y,endPos.x,endPos.y);
}


void DrawLine(int x, int y, int x2,int y2, Color c)
{

  SDL_RenderDrawLineF(gRenderer,x,y,x2,y2);
}


// Draw rectangle with vertical gradient fill color
// NOTE: This function is only used by GuiColorPicker()
 void DrawRectangleGradientV(int posX, int posY, int width, int height, Color color1, Color color2)
{
    Rectangle bounds = { (float)posX, (float)posY, (float)width, (float)height };
    DrawRectangleGradientEx(bounds, color1, color2, color2, color1);
}



/* ---- Structures */

/*!
\brief The structure passed to the internal Bresenham iterator.
*/
typedef struct {
	Sint16 x, y;
	int dx, dy, s1, s2, swapdir, error;
	Uint32 count;
} SDL2_gfxBresenhamIterator;

/*!
\brief The structure passed to the internal Murphy iterator.
*/
typedef struct {
	SDL_Renderer *renderer;
	int u, v;		/* delta x , delta y */
	int ku, kt, kv, kd;	/* loop constants */
	int oct2;
	int quad4;
	Sint16 last1x, last1y, last2x, last2y, first1x, first1y, first2x, first2y, tempx, tempy;
} SDL2_gfxMurphyIterator;

/* ---- Pixel */

/*!
\brief Draw pixel  in currently set color.

\param renderer The renderer to draw on.
\param x X (horizontal) coordinate of the pixel.
\param y Y (vertical) coordinate of the pixel.

\returns Returns 0 on success, -1 on failure.
*/
int pixel(SDL_Renderer *renderer, Sint16 x, Sint16 y)
{
	return SDL_RenderDrawPoint(renderer, x, y);
}


/*!
\brief Draw pixel with blending enabled if a<255.

\param renderer The renderer to draw on.
\param x X (horizontal) coordinate of the pixel.
\param y Y (vertical) coordinate of the pixel.
\param r The red color value of the pixel to draw.
\param g The green color value of the pixel to draw.
\param b The blue color value of the pixel to draw.
\param a The alpha value of the pixel to draw.

\returns Returns 0 on success, -1 on failure.
*/
int pixelRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;

	result |= SDL_RenderDrawPoint(renderer, x, y);
	return result;
}

/*!
\brief Draw pixel with blending enabled and using alpha weight on color.

\param renderer The renderer to draw on.
\param x The horizontal coordinate of the pixel.
\param y The vertical position of the pixel.
\param r The red color value of the pixel to draw.
\param g The green color value of the pixel to draw.
\param b The blue color value of the pixel to draw.
\param a The alpha value of the pixel to draw.
\param weight The weight multiplied into the alpha value of the pixel.

\returns Returns 0 on success, -1 on failure.
*/
int pixelRGBAWeight(SDL_Renderer * renderer, Sint16 x, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a, Uint32 weight)
{
	/*
	* Modify Alpha by weight
	*/
	Uint32 ax = a;
	ax = ((ax * weight) >> 8);
	if (ax > 255) {
		a = 255;
	} else {
		a = (Uint8)(ax & 0x000000ff);
	}

	return pixelRGBA(renderer, x, y, r, g, b, a);
}

/* ---- Hline */

/*!
\brief Draw horizontal line in currently set color

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. left) of the line.
\param x2 X coordinate of the second point (i.e. right) of the line.
\param y Y coordinate of the points of the line.

\returns Returns 0 on success, -1 on failure.
*/
int hline(SDL_Renderer * renderer, Sint16 x1, Sint16 x2, Sint16 y)
{
	return SDL_RenderDrawLine(renderer, x1, y, x2, y);;
}



/*!
\brief Draw horizontal line with blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. left) of the line.
\param x2 X coordinate of the second point (i.e. right) of the line.
\param y Y coordinate of the points of the line.
\param r The red value of the line to draw.
\param g The green value of the line to draw.
\param b The blue value of the line to draw.
\param a The alpha value of the line to draw.

\returns Returns 0 on success, -1 on failure.
*/
int hlineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 x2, Sint16 y, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_RenderDrawLine(renderer, x1, y, x2, y);
	return result;
}

/* ---- Vline */

/*!
\brief Draw vertical line in currently set color

\param renderer The renderer to draw on.
\param x X coordinate of points of the line.
\param y1 Y coordinate of the first point (i.e. top) of the line.
\param y2 Y coordinate of the second point (i.e. bottom) of the line.

\returns Returns 0 on success, -1 on failure.
*/
int vline(SDL_Renderer * renderer, Sint16 x, Sint16 y1, Sint16 y2)
{
	return SDL_RenderDrawLine(renderer, x, y1, x, y2);;
}



/*!
\brief Draw vertical line with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the points of the line.
\param y1 Y coordinate of the first point (i.e. top) of the line.
\param y2 Y coordinate of the second point (i.e. bottom) of the line.
\param r The red value of the line to draw.
\param g The green value of the line to draw.
\param b The blue value of the line to draw.
\param a The alpha value of the line to draw.

\returns Returns 0 on success, -1 on failure.
*/
int vlineRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y1, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	result |= SDL_SetRenderDrawBlendMode(renderer, (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);

	return result;
}

/* ---- Rectangle */

/*!
\brief Draw rectangle with blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. top right) of the rectangle.
\param y1 Y coordinate of the first point (i.e. top right) of the rectangle.
\param x2 X coordinate of the second point (i.e. bottom left) of the rectangle.
\param y2 Y coordinate of the second point (i.e. bottom left) of the rectangle.
\param r The red value of the rectangle to draw.
\param g The green value of the rectangle to draw.
\param b The blue value of the rectangle to draw.
\param a The alpha value of the rectangle to draw.

\returns Returns 0 on success, -1 on failure.
*/
int rectangleRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	Sint16 tmp;
	SDL_Rect rect;

	/*
	* Test for special cases of straight lines or single point
	*/
	if (x1 == x2) {
		if (y1 == y2) {
			return (pixelRGBA(renderer, x1, y1, r, g, b, a));
		} else {
			return (vlineRGBA(renderer, x1, y1, y2, r, g, b, a));
		}
	} else {
		if (y1 == y2) {
			return (hlineRGBA(renderer, x1, x2, y1, r, g, b, a));
		}
	}

	/*
	* Swap x1, x2 if required
	*/
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	/*
	* Swap y1, y2 if required
	*/
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	/*
	* Create destination rect
	*/
	rect.x = x1;
	rect.y = y1;
	rect.w = x2 - x1;
	rect.h = y2 - y1;

	/*
	* Draw
	*/
	result = 0;

	result |= SDL_RenderDrawRect(renderer, &rect);
	return result;
}

/* ---- Rounded Rectangle */


/*!
\brief Draw rounded-corner rectangle with blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. top right) of the rectangle.
\param y1 Y coordinate of the first point (i.e. top right) of the rectangle.
\param x2 X coordinate of the second point (i.e. bottom left) of the rectangle.
\param y2 Y coordinate of the second point (i.e. bottom left) of the rectangle.
\param rad The radius of the corner arc.
\param r The red value of the rectangle to draw.
\param g The green value of the rectangle to draw.
\param b The blue value of the rectangle to draw.
\param a The alpha value of the rectangle to draw.

\returns Returns 0 on success, -1 on failure.
*/
int roundedRectangleRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result = 0;
	Sint16 tmp;
	Sint16 w, h;
	Sint16 xx1, xx2;
	Sint16 yy1, yy2;

	/*
	* Check renderer
	*/
	if (renderer == NULL)
	{
		return -1;
	}

	/*
	* Check radius vor valid range
	*/
	if (rad < 0) {
		return -1;
	}

	/*
	* Special case - no rounding
	*/
	if (rad <= 1) {
		return rectangleRGBA(renderer, x1, y1, x2, y2, r, g, b, a);
	}

	/*
	* Test for special cases of straight lines or single point
	*/
	if (x1 == x2) {
		if (y1 == y2) {
			return (pixelRGBA(renderer, x1, y1, r, g, b, a));
		} else {
			return (vlineRGBA(renderer, x1, y1, y2, r, g, b, a));
		}
	} else {
		if (y1 == y2) {
			return (hlineRGBA(renderer, x1, x2, y1, r, g, b, a));
		}
	}

	/*
	* Swap x1, x2 if required
	*/
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	/*
	* Swap y1, y2 if required
	*/
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	/*
	* Calculate width&height
	*/
	w = x2 - x1;
	h = y2 - y1;

	/*
	* Maybe adjust radius
	*/
	if ((rad * 2) > w)
	{
		rad = w / 2;
	}
	if ((rad * 2) > h)
	{
		rad = h / 2;
	}

	/*
	* Draw corners
	*/
	xx1 = x1 + rad;
	xx2 = x2 - rad;
	yy1 = y1 + rad;
	yy2 = y2 - rad;
	result |= arcRGBA(renderer, xx1, yy1, rad, 180, 270, r, g, b, a);
	result |= arcRGBA(renderer, xx2, yy1, rad, 270, 360, r, g, b, a);
	result |= arcRGBA(renderer, xx1, yy2, rad,  90, 180, r, g, b, a);
	result |= arcRGBA(renderer, xx2, yy2, rad,   0,  90, r, g, b, a);

	/*
	* Draw lines
	*/
	if (xx1 <= xx2) {
		result |= hlineRGBA(renderer, xx1, xx2, y1, r, g, b, a);
		result |= hlineRGBA(renderer, xx1, xx2, y2, r, g, b, a);
	}
	if (yy1 <= yy2) {
		result |= vlineRGBA(renderer, x1, yy1, yy2, r, g, b, a);
		result |= vlineRGBA(renderer, x2, yy1, yy2, r, g, b, a);
	}

	return result;
}

/* ---- Rounded Box */



/*!
\brief Draw rounded-corner box (filled rectangle) with blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. top right) of the box.
\param y1 Y coordinate of the first point (i.e. top right) of the box.
\param x2 X coordinate of the second point (i.e. bottom left) of the box.
\param y2 Y coordinate of the second point (i.e. bottom left) of the box.
\param rad The radius of the corner arcs of the box.
\param r The red value of the box to draw.
\param g The green value of the box to draw.
\param b The blue value of the box to draw.
\param a The alpha value of the box to draw.

\returns Returns 0 on success, -1 on failure.
*/
int roundedBoxRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2,
	Sint16 y2, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	Sint16 w, h, r2, tmp;
	Sint16 cx = 0;
	Sint16 cy = rad;
	Sint16 ocx = (Sint16) 0xffff;
	Sint16 ocy = (Sint16) 0xffff;
	Sint16 df = 1 - rad;
	Sint16 d_e = 3;
	Sint16 d_se = -2 * rad + 5;
	Sint16 xpcx, xmcx, xpcy, xmcy;
	Sint16 ypcy, ymcy, ypcx, ymcx;
	Sint16 x, y, dx, dy;

	/*
	* Check destination renderer
	*/
	if (renderer == NULL)
	{
		return -1;
	}

	/*
	* Check radius vor valid range
	*/
	if (rad < 0) {
		return -1;
	}

	/*
	* Special case - no rounding
	*/
	if (rad <= 1) {
		return boxRGBA(renderer, x1, y1, x2, y2, r, g, b, a);
	}

	/*
	* Test for special cases of straight lines or single point
	*/
	if (x1 == x2) {
		if (y1 == y2) {
			return (pixelRGBA(renderer, x1, y1, r, g, b, a));
		} else {
			return (vlineRGBA(renderer, x1, y1, y2, r, g, b, a));
		}
	} else {
		if (y1 == y2) {
			return (hlineRGBA(renderer, x1, x2, y1, r, g, b, a));
		}
	}

	/*
	* Swap x1, x2 if required
	*/
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	/*
	* Swap y1, y2 if required
	*/
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	/*
	* Calculate width&height
	*/
	w = x2 - x1 + 1;
	h = y2 - y1 + 1;

	/*
	* Maybe adjust radius
	*/
	r2 = rad + rad;
	if (r2 > w)
	{
		rad = w / 2;
		r2 = rad + rad;
	}
	if (r2 > h)
	{
		rad = h / 2;
	}

	/* Setup filled circle drawing for corners */
	x = x1 + rad;
	y = y1 + rad;
	dx = x2 - x1 - rad - rad;
	dy = y2 - y1 - rad - rad;

	/*
	* Set color
	*/
	result = 0;


	/*
	* Draw corners
	*/
	do {
		xpcx = x + cx;
		xmcx = x - cx;
		xpcy = x + cy;
		xmcy = x - cy;
		if (ocy != cy) {
			if (cy > 0) {
				ypcy = y + cy;
				ymcy = y - cy;
				result |= hline(renderer, xmcx, xpcx + dx, ypcy + dy);
				result |= hline(renderer, xmcx, xpcx + dx, ymcy);
			} else {
				result |= hline(renderer, xmcx, xpcx + dx, y);
			}
			ocy = cy;
		}
		if (ocx != cx) {
			if (cx != cy) {
				if (cx > 0) {
					ypcx = y + cx;
					ymcx = y - cx;
					result |= hline(renderer, xmcy, xpcy + dx, ymcx);
					result |= hline(renderer, xmcy, xpcy + dx, ypcx + dy);
				} else {
					result |= hline(renderer, xmcy, xpcy + dx, y);
				}
			}
			ocx = cx;
		}

		/*
		* Update
		*/
		if (df < 0) {
			df += d_e;
			d_e += 2;
			d_se += 2;
		} else {
			df += d_se;
			d_e += 2;
			d_se += 4;
			cy--;
		}
		cx++;
	} while (cx <= cy);

	/* Inside */
	if (dx > 0 && dy > 0) {
		result |= boxRGBA(renderer, x1, y1 + rad + 1, x2, y2 - rad, r, g, b, a);
	}

	return (result);
}

/* ---- Box */


/*!
\brief Draw box (filled rectangle) with blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. top right) of the box.
\param y1 Y coordinate of the first point (i.e. top right) of the box.
\param x2 X coordinate of the second point (i.e. bottom left) of the box.
\param y2 Y coordinate of the second point (i.e. bottom left) of the box.
\param r The red value of the box to draw.
\param g The green value of the box to draw.
\param b The blue value of the box to draw.
\param a The alpha value of the box to draw.

\returns Returns 0 on success, -1 on failure.
*/
int boxRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	Sint16 tmp;
	SDL_Rect rect;

	/*
	* Test for special cases of straight lines or single point
	*/
	if (x1 == x2) {
		if (y1 == y2) {
			return (pixelRGBA(renderer, x1, y1, r, g, b, a));
		} else {
			return (vlineRGBA(renderer, x1, y1, y2, r, g, b, a));
		}
	} else {
		if (y1 == y2) {
			return (hlineRGBA(renderer, x1, x2, y1, r, g, b, a));
		}
	}

	/*
	* Swap x1, x2 if required
	*/
	if (x1 > x2) {
		tmp = x1;
		x1 = x2;
		x2 = tmp;
	}

	/*
	* Swap y1, y2 if required
	*/
	if (y1 > y2) {
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	/*
	* Create destination rect
	*/
	rect.x = x1;
	rect.y = y1;
	rect.w = x2 - x1 + 1;
	rect.h = y2 - y1 + 1;

	/*
	* Draw
	*/
	result = 0;

	result |= SDL_RenderFillRect(renderer, &rect);
	return result;
}

/* ----- Line */

/*!
\brief Draw line with alpha blending using the currently set color.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the line.
\param y1 Y coordinate of the first point of the line.
\param x2 X coordinate of the second point of the line.
\param y2 Y coordinate of the second point of the line.

\returns Returns 0 on success, -1 on failure.
*/
int line(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2)
{
	/*
	* Draw
	*/
	return SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}



/*!
\brief Draw line with alpha blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the line.
\param y1 Y coordinate of the first point of the line.
\param x2 X coordinate of the second point of the line.
\param y2 Y coordinate of the second point of the line.
\param r The red value of the line to draw.
\param g The green value of the line to draw.
\param b The blue value of the line to draw.
\param a The alpha value of the line to draw.

\returns Returns 0 on success, -1 on failure.
*/
int lineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	/*
	* Draw
	*/
	int result = 0;

	result |= SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	return result;
}

/* ---- AA Line */

#define AAlevels 256
#define AAbits 8

/*!
\brief Internal function to draw anti-aliased line with alpha blending and endpoint control.

This implementation of the Wu antialiasing code is based on Mike Abrash's
DDJ article which was reprinted as Chapter 42 of his Graphics Programming
Black Book, but has been optimized to work with SDL and utilizes 32-bit
fixed-point arithmetic by A. Schiffler. The endpoint control allows the
supression to draw the last pixel useful for rendering continous aa-lines
with alpha<255.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the aa-line.
\param y1 Y coordinate of the first point of the aa-line.
\param x2 X coordinate of the second point of the aa-line.
\param y2 Y coordinate of the second point of the aa-line.
\param r The red value of the aa-line to draw.
\param g The green value of the aa-line to draw.
\param b The blue value of the aa-line to draw.
\param a The alpha value of the aa-line to draw.
\param draw_endpoint Flag indicating if the endpoint should be drawn; draw if non-zero.

\returns Returns 0 on success, -1 on failure.
*/
int _aalineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int draw_endpoint)
{
	Sint32 xx0, yy0, xx1, yy1;
	int result;
	Uint32 intshift, erracc, erradj;
	Uint32 erracctmp, wgt, wgtcompmask;
	int dx, dy, tmp, xdir, y0p1, x0pxdir;

	/*
	* Keep on working with 32bit numbers
	*/
	xx0 = x1;
	yy0 = y1;
	xx1 = x2;
	yy1 = y2;

	/*
	* Reorder points to make dy positive
	*/
	if (yy0 > yy1) {
		tmp = yy0;
		yy0 = yy1;
		yy1 = tmp;
		tmp = xx0;
		xx0 = xx1;
		xx1 = tmp;
	}

	/*
	* Calculate distance
	*/
	dx = xx1 - xx0;
	dy = yy1 - yy0;

	/*
	* Adjust for negative dx and set xdir
	*/
	if (dx >= 0) {
		xdir = 1;
	} else {
		xdir = -1;
		dx = (-dx);
	}

	/*
	* Check for special cases
	*/
	if (dx == 0) {
		/*
		* Vertical line
		*/
		if (draw_endpoint)
		{
			return (vlineRGBA(renderer, x1, y1, y2, r, g, b, a));
		} else {
			if (dy > 0) {
				return (vlineRGBA(renderer, x1, yy0, yy0+dy, r, g, b, a));
			} else {
				return (pixelRGBA(renderer, x1, y1, r, g, b, a));
			}
		}
	} else if (dy == 0) {
		/*
		* Horizontal line
		*/
		if (draw_endpoint)
		{
			return (hlineRGBA(renderer, x1, x2, y1, r, g, b, a));
		} else {
			if (dx > 0) {
				return (hlineRGBA(renderer, xx0, xx0+(xdir*dx), y1, r, g, b, a));
			} else {
				return (pixelRGBA(renderer, x1, y1, r, g, b, a));
			}
		}
	} else if ((dx == dy) && (draw_endpoint)) {
		/*
		* Diagonal line (with endpoint)
		*/
		return (lineRGBA(renderer, x1, y1, x2, y2,  r, g, b, a));
	}


	/*
	* Line is not horizontal, vertical or diagonal (with endpoint)
	*/
	result = 0;

	/*
	* Zero accumulator
	*/
	erracc = 0;

	/*
	* # of bits by which to shift erracc to get intensity level
	*/
	intshift = 32 - AAbits;

	/*
	* Mask used to flip all bits in an intensity weighting
	*/
	wgtcompmask = AAlevels - 1;

	/*
	* Draw the initial pixel in the foreground color
	*/
	result |= pixelRGBA(renderer, x1, y1, r, g, b, a);

	/*
	* x-major or y-major?
	*/
	if (dy > dx) {

		/*
		* y-major.  Calculate 16-bit fixed point fractional part of a pixel that
		* X advances every time Y advances 1 pixel, truncating the result so that
		* we won't overrun the endpoint along the X axis
		*/
		/*
		* Not-so-portable version: erradj = ((Uint64)dx << 32) / (Uint64)dy;
		*/
		erradj = ((dx << 16) / dy) << 16;

		/*
		* draw all pixels other than the first and last
		*/
		x0pxdir = xx0 + xdir;
		while (--dy) {
			erracctmp = erracc;
			erracc += erradj;
			if (erracc <= erracctmp) {
				/*
				* rollover in error accumulator, x coord advances
				*/
				xx0 = x0pxdir;
				x0pxdir += xdir;
			}
			yy0++;		/* y-major so always advance Y */

			/*
			* the AAbits most significant bits of erracc give us the intensity
			* weighting for this pixel, and the complement of the weighting for
			* the paired pixel.
			*/
			wgt = (erracc >> intshift) & 255;
			result |= pixelRGBAWeight (renderer, xx0, yy0, r, g, b, a, 255 - wgt);
			result |= pixelRGBAWeight (renderer, x0pxdir, yy0, r, g, b, a, wgt);
		}

	} else {

		/*
		* x-major line.  Calculate 16-bit fixed-point fractional part of a pixel
		* that Y advances each time X advances 1 pixel, truncating the result so
		* that we won't overrun the endpoint along the X axis.
		*/
		/*
		* Not-so-portable version: erradj = ((Uint64)dy << 32) / (Uint64)dx;
		*/
		erradj = ((dy << 16) / dx) << 16;

		/*
		* draw all pixels other than the first and last
		*/
		y0p1 = yy0 + 1;
		while (--dx) {

			erracctmp = erracc;
			erracc += erradj;
			if (erracc <= erracctmp) {
				/*
				* Accumulator turned over, advance y
				*/
				yy0 = y0p1;
				y0p1++;
			}
			xx0 += xdir;	/* x-major so always advance X */
			/*
			* the AAbits most significant bits of erracc give us the intensity
			* weighting for this pixel, and the complement of the weighting for
			* the paired pixel.
			*/
			wgt = (erracc >> intshift) & 255;
			result |= pixelRGBAWeight (renderer, xx0, yy0, r, g, b, a, 255 - wgt);
			result |= pixelRGBAWeight (renderer, xx0, y0p1, r, g, b, a, wgt);
		}
	}

	/*
	* Do we have to draw the endpoint
	*/
	if (draw_endpoint) {
		/*
		* Draw final pixel, always exactly intersected by the line and doesn't
		* need to be weighted.
		*/
		result |= pixelRGBA (renderer, x2, y2, r, g, b, a);
	}

	return (result);
}



/*!
\brief Draw anti-aliased line with alpha blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the aa-line.
\param y1 Y coordinate of the first point of the aa-line.
\param x2 X coordinate of the second point of the aa-line.
\param y2 Y coordinate of the second point of the aa-line.
\param r The red value of the aa-line to draw.
\param g The green value of the aa-line to draw.
\param b The blue value of the aa-line to draw.
\param a The alpha value of the aa-line to draw.

\returns Returns 0 on success, -1 on failure.
*/
int aalineRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _aalineRGBA(renderer, x1, y1, x2, y2, r, g, b, a, 1);
}

/* ----- Circle */



/*!
\brief Draw circle with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the circle.
\param y Y coordinate of the center of the circle.
\param rad Radius in pixels of the circle.
\param r The red value of the circle to draw.
\param g The green value of the circle to draw.
\param b The blue value of the circle to draw.
\param a The alpha value of the circle to draw.

\returns Returns 0 on success, -1 on failure.
*/
int circleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return ellipseRGBA(renderer, x, y, rad, rad, r, g, b, a);
}

/* ----- Arc */


/*!
\brief Arc with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the arc.
\param y Y coordinate of the center of the arc.
\param rad Radius in pixels of the arc.
\param start Starting radius in degrees of the arc. 0 degrees is down, increasing counterclockwise.
\param end Ending radius in degrees of the arc. 0 degrees is down, increasing counterclockwise.
\param r The red value of the arc to draw.
\param g The green value of the arc to draw.
\param b The blue value of the arc to draw.
\param a The alpha value of the arc to draw.

\returns Returns 0 on success, -1 on failure.
*/
/* TODO: rewrite algorithm; arc endpoints are not always drawn */
int arcRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	Sint16 cx = 0;
	Sint16 cy = rad;
	Sint16 df = 1 - rad;
	Sint16 d_e = 3;
	Sint16 d_se = -2 * rad + 5;
	Sint16 xpcx, xmcx, xpcy, xmcy;
	Sint16 ypcy, ymcy, ypcx, ymcx;
	Uint8 drawoct;
	int startoct, endoct, oct, stopval_start = 0, stopval_end = 0;
	double dstart, dend, temp = 0.;

	/*
	* Sanity check radius
	*/
	if (rad < 0) {
		return (-1);
	}

	/*
	* Special case for rad=0 - draw a point
	*/
	if (rad == 0) {
		return (pixelRGBA(renderer, x, y, r, g, b, a));
	}

	/*
	 Octant labeling

	  \ 5 | 6 /
	   \  |  /
	  4 \ | / 7
	     \|/
	------+------ +x
	     /|\
	  3 / | \ 0
	   /  |  \
	  / 2 | 1 \
	      +y

	 Initially reset bitmask to 0x00000000
	 the set whether or not to keep drawing a given octant.
	 For example: 0x00111100 means we're drawing in octants 2-5
	*/
	drawoct = 0;

	/*
	* Fixup angles
	*/
	start %= 360;
	end %= 360;
	/* 0 <= start & end < 360; note that sometimes start > end - if so, arc goes back through 0. */
	while (start < 0) start += 360;
	while (end < 0) end += 360;
	start %= 360;
	end %= 360;

	/* now, we find which octants we're drawing in. */
	startoct = start / 45;
	endoct = end / 45;
	oct = startoct - 1;

	/* stopval_start, stopval_end; what values of cx to stop at. */
	do {
		oct = (oct + 1) % 8;

		if (oct == startoct) {
			/* need to compute stopval_start for this octant.  Look at picture above if this is unclear */
			dstart = (double)start;
			switch (oct)
			{
			case 0:
			case 3:
				temp = sin(dstart * M_PI / 180.);
				break;
			case 1:
			case 6:
				temp = cos(dstart * M_PI / 180.);
				break;
			case 2:
			case 5:
				temp = -cos(dstart * M_PI / 180.);
				break;
			case 4:
			case 7:
				temp = -sin(dstart * M_PI / 180.);
				break;
			}
			temp *= rad;
			stopval_start = (int)temp;

			/*
			This isn't arbitrary, but requires graph paper to explain well.
			The basic idea is that we're always changing drawoct after we draw, so we
			stop immediately after we render the last sensible pixel at x = ((int)temp).
			and whether to draw in this octant initially
			*/
			if (oct % 2) drawoct |= (1 << oct);			/* this is basically like saying drawoct[oct] = true, if drawoct were a bool array */
			else		 drawoct &= 255 - (1 << oct);	/* this is basically like saying drawoct[oct] = false */
		}
		if (oct == endoct) {
			/* need to compute stopval_end for this octant */
			dend = (double)end;
			switch (oct)
			{
			case 0:
			case 3:
				temp = sin(dend * M_PI / 180);
				break;
			case 1:
			case 6:
				temp = cos(dend * M_PI / 180);
				break;
			case 2:
			case 5:
				temp = -cos(dend * M_PI / 180);
				break;
			case 4:
			case 7:
				temp = -sin(dend * M_PI / 180);
				break;
			}
			temp *= rad;
			stopval_end = (int)temp;

			/* and whether to draw in this octant initially */
			if (startoct == endoct)	{
				/* note:      we start drawing, stop, then start again in this case */
				/* otherwise: we only draw in this octant, so initialize it to false, it will get set back to true */
				if (start > end) {
					/* unfortunately, if we're in the same octant and need to draw over the whole circle, */
					/* we need to set the rest to true, because the while loop will end at the bottom. */
					drawoct = 255;
				} else {
					drawoct &= 255 - (1 << oct);
				}
			}
			else if (oct % 2) drawoct &= 255 - (1 << oct);
			else			  drawoct |= (1 << oct);
		} else if (oct != startoct) { /* already verified that it's != endoct */
			drawoct |= (1 << oct); /* draw this entire segment */
		}
	} while (oct != endoct);

	/* so now we have what octants to draw and when to draw them. all that's left is the actual raster code. */

	/*
	* Set color
	*/
	result = 0;


	/*
	* Draw arc
	*/
	do {
		ypcy = y + cy;
		ymcy = y - cy;
		if (cx > 0) {
			xpcx = x + cx;
			xmcx = x - cx;

			/* always check if we're drawing a certain octant before adding a pixel to that octant. */
			if (drawoct & 4)  result |= pixel(renderer, xmcx, ypcy);
			if (drawoct & 2)  result |= pixel(renderer, xpcx, ypcy);
			if (drawoct & 32) result |= pixel(renderer, xmcx, ymcy);
			if (drawoct & 64) result |= pixel(renderer, xpcx, ymcy);
		} else {
			if (drawoct & 96) result |= pixel(renderer, x, ymcy);
			if (drawoct & 6)  result |= pixel(renderer, x, ypcy);
		}

		xpcy = x + cy;
		xmcy = x - cy;
		if (cx > 0 && cx != cy) {
			ypcx = y + cx;
			ymcx = y - cx;
			if (drawoct & 8)   result |= pixel(renderer, xmcy, ypcx);
			if (drawoct & 1)   result |= pixel(renderer, xpcy, ypcx);
			if (drawoct & 16)  result |= pixel(renderer, xmcy, ymcx);
			if (drawoct & 128) result |= pixel(renderer, xpcy, ymcx);
		} else if (cx == 0) {
			if (drawoct & 24)  result |= pixel(renderer, xmcy, y);
			if (drawoct & 129) result |= pixel(renderer, xpcy, y);
		}

		/*
		* Update whether we're drawing an octant
		*/
		if (stopval_start == cx) {
			/* works like an on-off switch. */
			/* This is just in case start & end are in the same octant. */
			if (drawoct & (1 << startoct)) drawoct &= 255 - (1 << startoct);
			else						   drawoct |= (1 << startoct);
		}
		if (stopval_end == cx) {
			if (drawoct & (1 << endoct)) drawoct &= 255 - (1 << endoct);
			else						 drawoct |= (1 << endoct);
		}

		/*
		* Update pixels
		*/
		if (df < 0) {
			df += d_e;
			d_e += 2;
			d_se += 2;
		} else {
			df += d_se;
			d_e += 2;
			d_se += 4;
			cy--;
		}
		cx++;
	} while (cx <= cy);

	return (result);
}

/* ----- AA Circle */

/*!
\brief Draw anti-aliased circle with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the aa-circle.
\param y Y coordinate of the center of the aa-circle.
\param rad Radius in pixels of the aa-circle.
\param r The red value of the aa-circle to draw.
\param g The green value of the aa-circle to draw.
\param b The blue value of the aa-circle to draw.
\param a The alpha value of the aa-circle to draw.

\returns Returns 0 on success, -1 on failure.
*/
int aacircleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	/*
	* Draw
	*/
	return aaellipseRGBA(renderer, x, y, rad, rad, r, g, b, a);
}

/* ----- Ellipse */

/*!
\brief Internal function to draw pixels or lines in 4 quadrants.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the quadrant.
\param y Y coordinate of the center of the quadrant.
\param dx X offset in pixels of the corners of the quadrant.
\param dy Y offset in pixels of the corners of the quadrant.
\param f Flag indicating if the quadrant should be filled (1) or not (0).

\returns Returns 0 on success, -1 on failure.
*/
int _drawQuadrants(SDL_Renderer * renderer,  Sint16 x, Sint16 y, Sint16 dx, Sint16 dy, Sint32 f)
{
	int result = 0;
	Sint16 xpdx, xmdx;
	Sint16 ypdy, ymdy;

	if (dx == 0) {
		if (dy == 0) {
			result |= pixel(renderer, x, y);
		} else {
			ypdy = y + dy;
			ymdy = y - dy;
			if (f) {
				result |= vline(renderer, x, ymdy, ypdy);
			} else {
				result |= pixel(renderer, x, ypdy);
				result |= pixel(renderer, x, ymdy);
			}
		}
	} else {
		xpdx = x + dx;
		xmdx = x - dx;
		ypdy = y + dy;
		ymdy = y - dy;
		if (f) {
				result |= vline(renderer, xpdx, ymdy, ypdy);
				result |= vline(renderer, xmdx, ymdy, ypdy);
		} else {
				result |= pixel(renderer, xpdx, ypdy);
				result |= pixel(renderer, xmdx, ypdy);
				result |= pixel(renderer, xpdx, ymdy);
				result |= pixel(renderer, xmdx, ymdy);
		}
	}

	return result;
}

/*!
\brief Internal function to draw ellipse or filled ellipse with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the ellipse.
\param y Y coordinate of the center of the ellipse.
\param rx Horizontal radius in pixels of the ellipse.
\param ry Vertical radius in pixels of the ellipse.
\param r The red value of the ellipse to draw.
\param g The green value of the ellipse to draw.
\param b The blue value of the ellipse to draw.
\param a The alpha value of the ellipse to draw.
\param f Flag indicating if the ellipse should be filled (1) or not (0).

\returns Returns 0 on success, -1 on failure.
*/
#define DEFAULT_ELLIPSE_OVERSCAN	4
int _ellipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a, Sint32 f)
{
	int result;
	Sint32 rxi, ryi;
	Sint32 rx2, ry2, rx22, ry22;
    Sint32 error;
    Sint32 curX, curY, curXp1, curYm1;
	Sint32 scrX, scrY, oldX, oldY;
    Sint32 deltaX, deltaY;
	Sint32 ellipseOverscan;

	/*
	* Sanity check radii
	*/
	if ((rx < 0) || (ry < 0)) {
		return (-1);
	}

	/*
	* Set color
	*/
	result = 0;


	/*
	* Special cases for rx=0 and/or ry=0: draw a hline/vline/pixel
	*/
	if (rx == 0) {
		if (ry == 0) {
			return (pixel(renderer, x, y));
		} else {
			return (vline(renderer, x, y - ry, y + ry));
		}
	} else {
		if (ry == 0) {
			return (hline(renderer, x - rx, x + rx, y));
		}
	}

	/*
 	 * Adjust overscan
	 */
	rxi = rx;
	ryi = ry;
	if (rxi >= 512 || ryi >= 512)
	{
		ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 4;
	}
	else if (rxi >= 256 || ryi >= 256)
	{
		ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 2;
	}
	else
	{
		ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 1;
	}

	/*
	 * Top/bottom center points.
	 */
	oldX = scrX = 0;
	oldY = scrY = ryi;
	result |= _drawQuadrants(renderer, x, y, 0, ry, f);

	/* Midpoint ellipse algorithm with overdraw */
	rxi *= ellipseOverscan;
	ryi *= ellipseOverscan;
	rx2 = rxi * rxi;
	rx22 = rx2 + rx2;
    ry2 = ryi * ryi;
	ry22 = ry2 + ry2;
    curX = 0;
    curY = ryi;
    deltaX = 0;
    deltaY = rx22 * curY;

	/* Points in segment 1 */
    error = ry2 - rx2 * ryi + rx2 / 4;
    while (deltaX <= deltaY)
    {
          curX++;
          deltaX += ry22;

          error +=  deltaX + ry2;
          if (error >= 0)
          {
               curY--;
               deltaY -= rx22;
               error -= deltaY;
          }

		  scrX = curX / ellipseOverscan;
		  scrY = curY / ellipseOverscan;
		  if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY)) {
			result |= _drawQuadrants(renderer, x, y, scrX, scrY, f);
			oldX = scrX;
			oldY = scrY;
		  }
    }

	/* Points in segment 2 */
	if (curY > 0)
	{
		curXp1 = curX + 1;
		curYm1 = curY - 1;
		error = ry2 * curX * curXp1 + ((ry2 + 3) / 4) + rx2 * curYm1 * curYm1 - rx2 * ry2;
		while (curY > 0)
		{
			curY--;
			deltaY -= rx22;

			error += rx2;
			error -= deltaY;

			if (error <= 0)
			{
               curX++;
               deltaX += ry22;
               error += deltaX;
			}

		    scrX = curX / ellipseOverscan;
		    scrY = curY / ellipseOverscan;
		    if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY)) {
				oldY--;
				for (;oldY >= scrY; oldY--) {
					result |= _drawQuadrants(renderer, x, y, scrX, oldY, f);
					/* prevent overdraw */
					if (f) {
						oldY = scrY - 1;
					}
				}
  				oldX = scrX;
				oldY = scrY;
		    }
		}

		/* Remaining points in vertical */
		if (!f) {
			oldY--;
			for (;oldY >= 0; oldY--) {
				result |= _drawQuadrants(renderer, x, y, scrX, oldY, f);
			}
		}
	}

	return (result);
}



/*!
\brief Draw ellipse with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the ellipse.
\param y Y coordinate of the center of the ellipse.
\param rx Horizontal radius in pixels of the ellipse.
\param ry Vertical radius in pixels of the ellipse.
\param r The red value of the ellipse to draw.
\param g The green value of the ellipse to draw.
\param b The blue value of the ellipse to draw.
\param a The alpha value of the ellipse to draw.

\returns Returns 0 on success, -1 on failure.
*/
int ellipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _ellipseRGBA(renderer, x, y, rx, ry, r, g, b, a, 0);
}

/* ----- Filled Circle */


/*!
\brief Draw filled circle with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the filled circle.
\param y Y coordinate of the center of the filled circle.
\param rad Radius in pixels of the filled circle.
\param r The red value of the filled circle to draw.
\param g The green value of the filled circle to draw.
\param b The blue value of the filled circle to draw.
\param a The alpha value of the filled circle to draw.

\returns Returns 0 on success, -1 on failure.
*/
int filledCircleRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _ellipseRGBA(renderer, x, y, rad, rad, r, g ,b, a, 1);
}


/* ----- AA Ellipse */

/* Windows targets do not have lrint, so provide a local inline version */
#if defined(_MSC_VER)
/* Detect 64bit and use intrinsic version */
#ifdef _M_X64
#include <emmintrin.h>
static __inline long
	lrint(float f)
{
	return _mm_cvtss_si32(_mm_load_ss(&f));
}
#elif defined(_M_IX86)
__inline long int
	lrint (double flt)
{
	int intgr;
	_asm
	{
		fld flt
			fistp intgr
	};
	return intgr;
}
#elif defined(_M_ARM)
#include <armintr.h>
#pragma warning(push)
#pragma warning(disable: 4716)
__declspec(naked) long int
	lrint (double flt)
{
	__emit(0xEC410B10); // fmdrr  d0, r0, r1
	__emit(0xEEBD0B40); // ftosid s0, d0
	__emit(0xEE100A10); // fmrs   r0, s0
	__emit(0xE12FFF1E); // bx     lr
}
#pragma warning(pop)
#else
#error lrint needed for MSVC on non X86/AMD64/ARM targets.
#endif
#endif



/*!
\brief Draw anti-aliased ellipse with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the aa-ellipse.
\param y Y coordinate of the center of the aa-ellipse.
\param rx Horizontal radius in pixels of the aa-ellipse.
\param ry Vertical radius in pixels of the aa-ellipse.
\param r The red value of the aa-ellipse to draw.
\param g The green value of the aa-ellipse to draw.
\param b The blue value of the aa-ellipse to draw.
\param a The alpha value of the aa-ellipse to draw.

\returns Returns 0 on success, -1 on failure.
*/
int aaellipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int result;
	int i;
	int a2, b2, ds, dt, dxt, t, s, d;
	Sint16 xp, yp, xs, ys, dyt, od, xx, yy, xc2, yc2;
	float cp;
	double sab;
	Uint8 weight, iweight;

	/*
	* Sanity check radii
	*/
	if ((rx < 0) || (ry < 0)) {
		return (-1);
	}

	/*
	* Special cases for rx=0 and/or ry=0: draw a hline/vline/pixel
	*/
	if (rx == 0) {
		if (ry == 0) {
			return (pixelRGBA(renderer, x, y, r, g, b, a));
		} else {
			return (vlineRGBA(renderer, x, y - ry, y + ry, r, g, b, a));
		}
	} else {
		if (ry == 0) {
			return (hlineRGBA(renderer, x - rx, x + rx, y, r, g, b, a));
		}
	}

	/* Variable setup */
	a2 = rx * rx;
	b2 = ry * ry;

	ds = 2 * a2;
	dt = 2 * b2;

	xc2 = 2 * x;
	yc2 = 2 * y;

	sab = sqrt((double)(a2 + b2));
	od = (Sint16)lrint(sab*0.01) + 1; /* introduce some overdraw */
	dxt = (Sint16)lrint((double)a2 / sab) + od;

	t = 0;
	s = -2 * a2 * ry;
	d = 0;

	xp = x;
	yp = y - ry;

	/* Draw */
	result = 0;


	/* "End points" */
	result |= pixelRGBA(renderer, xp, yp, r, g, b, a);
	result |= pixelRGBA(renderer, xc2 - xp, yp, r, g, b, a);
	result |= pixelRGBA(renderer, xp, yc2 - yp, r, g, b, a);
	result |= pixelRGBA(renderer, xc2 - xp, yc2 - yp, r, g, b, a);

	for (i = 1; i <= dxt; i++) {
		xp--;
		d += t - b2;

		if (d >= 0)
			ys = yp - 1;
		else if ((d - s - a2) > 0) {
			if ((2 * d - s - a2) >= 0)
				ys = yp + 1;
			else {
				ys = yp;
				yp++;
				d -= s + a2;
				s += ds;
			}
		} else {
			yp++;
			ys = yp + 1;
			d -= s + a2;
			s += ds;
		}

		t -= dt;

		/* Calculate alpha */
		if (s != 0) {
			cp = (float) abs(d) / (float) abs(s);
			if (cp > 1.0) {
				cp = 1.0;
			}
		} else {
			cp = 1.0;
		}

		/* Calculate weights */
		weight = (Uint8) (cp * 255);
		iweight = 255 - weight;

		/* Upper half */
		xx = xc2 - xp;
		result |= pixelRGBAWeight(renderer, xp, yp, r, g, b, a, iweight);
		result |= pixelRGBAWeight(renderer, xx, yp, r, g, b, a, iweight);

		result |= pixelRGBAWeight(renderer, xp, ys, r, g, b, a, weight);
		result |= pixelRGBAWeight(renderer, xx, ys, r, g, b, a, weight);

		/* Lower half */
		yy = yc2 - yp;
		result |= pixelRGBAWeight(renderer, xp, yy, r, g, b, a, iweight);
		result |= pixelRGBAWeight(renderer, xx, yy, r, g, b, a, iweight);

		yy = yc2 - ys;
		result |= pixelRGBAWeight(renderer, xp, yy, r, g, b, a, weight);
		result |= pixelRGBAWeight(renderer, xx, yy, r, g, b, a, weight);
	}

	/* Replaces original approximation code dyt = abs(yp - yc); */
	dyt = (Sint16)lrint((double)b2 / sab ) + od;

	for (i = 1; i <= dyt; i++) {
		yp++;
		d -= s + a2;

		if (d <= 0)
			xs = xp + 1;
		else if ((d + t - b2) < 0) {
			if ((2 * d + t - b2) <= 0)
				xs = xp - 1;
			else {
				xs = xp;
				xp--;
				d += t - b2;
				t -= dt;
			}
		} else {
			xp--;
			xs = xp - 1;
			d += t - b2;
			t -= dt;
		}

		s += ds;

		/* Calculate alpha */
		if (t != 0) {
			cp = (float) abs(d) / (float) abs(t);
			if (cp > 1.0) {
				cp = 1.0;
			}
		} else {
			cp = 1.0;
		}

		/* Calculate weight */
		weight = (Uint8) (cp * 255);
		iweight = 255 - weight;

		/* Left half */
		xx = xc2 - xp;
		yy = yc2 - yp;
		result |= pixelRGBAWeight(renderer, xp, yp, r, g, b, a, iweight);
		result |= pixelRGBAWeight(renderer, xx, yp, r, g, b, a, iweight);

		result |= pixelRGBAWeight(renderer, xp, yy, r, g, b, a, iweight);
		result |= pixelRGBAWeight(renderer, xx, yy, r, g, b, a, iweight);

		/* Right half */
		xx = xc2 - xs;
		result |= pixelRGBAWeight(renderer, xs, yp, r, g, b, a, weight);
		result |= pixelRGBAWeight(renderer, xx, yp, r, g, b, a, weight);

		result |= pixelRGBAWeight(renderer, xs, yy, r, g, b, a, weight);
		result |= pixelRGBAWeight(renderer, xx, yy, r, g, b, a, weight);
	}

	return (result);
}

/* ---- Filled Ellipse */


/*!
\brief Draw filled ellipse with blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the filled ellipse.
\param y Y coordinate of the center of the filled ellipse.
\param rx Horizontal radius in pixels of the filled ellipse.
\param ry Vertical radius in pixels of the filled ellipse.
\param r The red value of the filled ellipse to draw.
\param g The green value of the filled ellipse to draw.
\param b The blue value of the filled ellipse to draw.
\param a The alpha value of the filled ellipse to draw.

\returns Returns 0 on success, -1 on failure.
*/
int filledEllipseRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _ellipseRGBA(renderer, x, y, rx, ry, r, g, b, a, 1);
}

/* ----- Pie */

/*!
\brief Internal float (low-speed) pie-calc implementation by drawing polygons.

Note: Determines vertex array and uses polygon or filledPolygon drawing routines to render.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the pie.
\param y Y coordinate of the center of the pie.
\param rad Radius in pixels of the pie.
\param start Starting radius in degrees of the pie.
\param end Ending radius in degrees of the pie.
\param r The red value of the pie to draw.
\param g The green value of the pie to draw.
\param b The blue value of the pie to draw.
\param a The alpha value of the pie to draw.
\param filled Flag indicating if the pie should be filled (=1) or not (=0).

\returns Returns 0 on success, -1 on failure.
*/
/* TODO: rewrite algorithm; pie is not always accurate */
int _pieRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end,  Uint8 filled)
{
	int result;
	double angle, start_angle, end_angle;
	double deltaAngle;
	double dr;
	int numpoints, i;
	Sint16 *vx, *vy;

	/*
	* Sanity check radii
	*/
	if (rad < 0) {
		return (-1);
	}

	/*
	* Fixup angles
	*/
	start = start % 360;
	end = end % 360;

	/*
	* Special case for rad=0 - draw a point
	*/
	if (rad == 0) {
		return (pixelRGBA(renderer, x, y, 0,0,0,0));
	}

	/*
	* Variable setup
	*/
	dr = (double) rad;
	deltaAngle = 3.0 / dr;
	start_angle = (double) start *(2.0 * M_PI / 360.0);
	end_angle = (double) end *(2.0 * M_PI / 360.0);
	if (start > end) {
		end_angle += (2.0 * M_PI);
	}

	/* We will always have at least 2 points */
	numpoints = 2;

	/* Count points (rather than calculating it) */
	angle = start_angle;
	while (angle < end_angle) {
		angle += deltaAngle;
		numpoints++;
	}

	/* Allocate combined vertex array */
	vx = vy = (Sint16 *) DMTMALLOC(2 * sizeof(Uint16) * numpoints);
	if (vx == NULL) {
		return (-1);
	}

	/* Update point to start of vy */
	vy += numpoints;

	/* Center */
	vx[0] = x;
	vy[0] = y;

	/* First vertex */
	angle = start_angle;
	vx[1] = x + (int) (dr * cos(angle));
	vy[1] = y + (int) (dr * sin(angle));

	if (numpoints<3)
	{
		result = lineRGBA(renderer, vx[0], vy[0], vx[1], vy[1],0,0,0,0);
	}
	else
	{
		/* Calculate other vertices */
		i = 2;
		angle = start_angle;
		while (angle < end_angle) {
			angle += deltaAngle;
			if (angle>end_angle)
			{
				angle = end_angle;
			}
			vx[i] = x + (int) (dr * cos(angle));
			vy[i] = y + (int) (dr * sin(angle));
			i++;
		}

		/* Draw */
		if (filled) {
			result = filledPolygonRGBA(renderer, vx, vy, numpoints);
		} else {
			result = aapolygonRGBA(renderer, vx, vy, numpoints);
		}
	}

	/* Free combined vertex array */
	DMTFREE(vx);

	return (result);
}


/*!
\brief Draw pie (outline) with alpha blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the pie.
\param y Y coordinate of the center of the pie.
\param rad Radius in pixels of the pie.
\param start Starting radius in degrees of the pie.
\param end Ending radius in degrees of the pie.
\param r The red value of the pie to draw.
\param g The green value of the pie to draw.
\param b The blue value of the pie to draw.
\param a The alpha value of the pie to draw.

\returns Returns 0 on success, -1 on failure.
*/
int pieRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad,
	Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _pieRGBA(renderer, x, y, rad, start, end, 0);
}



/*!
\brief Draw filled pie with alpha blending.

\param renderer The renderer to draw on.
\param x X coordinate of the center of the filled pie.
\param y Y coordinate of the center of the filled pie.
\param rad Radius in pixels of the filled pie.
\param start Starting radius in degrees of the filled pie.
\param end Ending radius in degrees of the filled pie.
\param r The red value of the filled pie to draw.
\param g The green value of the filled pie to draw.
\param b The blue value of the filled pie to draw.
\param a The alpha value of the filled pie to draw.

\returns Returns 0 on success, -1 on failure.
*/
int filledPieRGBA(SDL_Renderer * renderer, Sint16 x, Sint16 y, Sint16 rad,
	Sint16 start, Sint16 end, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return _pieRGBA(renderer, x, y, rad, start, end, 1);
}

/* ------ Trigon */

/*!
\brief Draw trigon (triangle outline) with alpha blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the trigon.
\param y1 Y coordinate of the first point of the trigon.
\param x2 X coordinate of the second point of the trigon.
\param y2 Y coordinate of the second point of the trigon.
\param x3 X coordinate of the third point of the trigon.
\param y3 Y coordinate of the third point of the trigon.
\param r The red value of the trigon to draw.
\param g The green value of the trigon to draw.
\param b The blue value of the trigon to draw.
\param a The alpha value of the trigon to draw.

\returns Returns 0 on success, -1 on failure.
*/
int trigonRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
	Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	Sint16 vx[3];
	Sint16 vy[3];

	vx[0]=x1;
	vx[1]=x2;
	vx[2]=x3;
	vy[0]=y1;
	vy[1]=y2;
	vy[2]=y3;

	return(aapolygonRGBA(renderer,vx,vy,3));
}

/* ------ AA-Trigon */



/*!
\brief Draw anti-aliased trigon (triangle outline) with alpha blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the aa-trigon.
\param y1 Y coordinate of the first point of the aa-trigon.
\param x2 X coordinate of the second point of the aa-trigon.
\param y2 Y coordinate of the second point of the aa-trigon.
\param x3 X coordinate of the third point of the aa-trigon.
\param y3 Y coordinate of the third point of the aa-trigon.
\param r The red value of the aa-trigon to draw.
\param g The green value of the aa-trigon to draw.
\param b The blue value of the aa-trigon to draw.
\param a The alpha value of the aa-trigon to draw.

\returns Returns 0 on success, -1 on failure.
*/
int aatrigonRGBA(SDL_Renderer * renderer,  Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
	Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	Sint16 vx[3];
	Sint16 vy[3];

	vx[0]=x1;
	vx[1]=x2;
	vx[2]=x3;
	vy[0]=y1;
	vy[1]=y2;
	vy[2]=y3;

	return(aapolygonRGBA(renderer,vx,vy,3));
}

/* ------ Filled Trigon */


/*!
\brief Draw filled trigon (triangle) with alpha blending.

Note: Creates vertex array and uses aapolygon routine to render.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the filled trigon.
\param y1 Y coordinate of the first point of the filled trigon.
\param x2 X coordinate of the second point of the filled trigon.
\param y2 Y coordinate of the second point of the filled trigon.
\param x3 X coordinate of the third point of the filled trigon.
\param y3 Y coordinate of the third point of the filled trigon.
\param r The red value of the filled trigon to draw.
\param g The green value of the filled trigon to draw.
\param b The blue value of the filled trigon to draw.
\param a The alpha value of the filled trigon to draw.

\returns Returns 0 on success, -1 on failure.
*/
int filledTrigonRGBA(SDL_Renderer * renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3,
	Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	Sint16 vx[3];
	Sint16 vy[3];

	vx[0]=x1;
	vx[1]=x2;
	vx[2]=x3;
	vy[0]=y1;
	vy[1]=y2;
	vy[2]=y3;

	return(filledPolygonRGBA(renderer,vx,vy,3));
}

/* ---- Polygon */


int polygon(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy, int n)
{
	/*
	* Draw
	*/
	int result = 0;
	int i, nn;
	SDL_Point* points;

	/*
	* Vertex array NULL check
	*/
	if (vx == NULL) {
		return (-1);
	}
	if (vy == NULL) {
		return (-1);
	}

	/*
	* Sanity check
	*/
	if (n < 3) {
		return (-1);
	}

	/*
	* Create array of points
	*/
	nn = n + 1;
	points = (SDL_Point*)malloc(sizeof(SDL_Point) * nn);
	if (points == NULL)
	{
		return -1;
	}
	for (i=0; i<n; i++)
	{
		points[i].x = vx[i];
		points[i].y = vy[i];
	}
	points[n].x = vx[0];
	points[n].y = vy[0];

	/*
	* Draw
	*/
	result |= SDL_RenderDrawLines(renderer, points, nn);
	free(points);

	return (result);
}



/*!
\brief Draw anti-aliased polygon with alpha blending.

\param renderer The renderer to draw on.
\param vx Vertex array containing X coordinates of the points of the aa-polygon.
\param vy Vertex array containing Y coordinates of the points of the aa-polygon.
\param n Number of points in the vertex array. Minimum number is 3.
\param r The red value of the aa-polygon to draw.
\param g The green value of the aa-polygon to draw.
\param b The blue value of the aa-polygon to draw.
\param a The alpha value of the aa-polygon to draw.

\returns Returns 0 on success, -1 on failure.
*/
int aapolygonRGBA(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy, int n)
{
	int result;
	int i;
	const Sint16 *x1, *y1, *x2, *y2;

	/*
	* Vertex array NULL check
	*/
	if (vx == NULL) {
		return (-1);
	}
	if (vy == NULL) {
		return (-1);
	}

	/*
	* Sanity check
	*/
	if (n < 3) {
		return (-1);
	}

	/*
	* Pointer setup
	*/
	x1 = x2 = vx;
	y1 = y2 = vy;
	x2++;
	y2++;

	/*
	* Draw
	*/
	result = 0;
	for (i = 1; i < n; i++) {
		result |= _aalineRGBA(renderer, *x1, *y1, *x2, *y2, 0,0,0,0, 0);
		x1 = x2;
		y1 = y2;
		x2++;
		y2++;
	}

	result |= _aalineRGBA(renderer, *x1, *y1, *vx, *vy, 0,0,0,0, 0);

	return (result);
}

/* ---- Filled Polygon */

/*!
\brief Internal helper qsort callback functions used in filled polygon drawing.

\param a The surface to draw on.
\param b Vertex array containing X coordinates of the points of the polygon.

\returns Returns 0 if a==b, a negative number if a<b or a positive number if a>b.
*/
int _gfxPrimitivesCompareInt(const void *a, const void *b)
{
	return (*(const int *) a) - (*(const int *) b);
}

/*!
\brief Global vertex array to use if optional parameters are not given in filledPolygonMT calls.

Note: Used for non-multithreaded (default) operation of filledPolygonMT.
*/
static int *gfxPrimitivesPolyIntsGlobal = NULL;

/*!
\brief Flag indicating if global vertex array was already allocated.

Note: Used for non-multithreaded (default) operation of filledPolygonMT.
*/
static int gfxPrimitivesPolyAllocatedGlobal = 0;

/*!
\brief Draw filled polygon with alpha blending (multi-threaded capable).

Note: The last two parameters are optional; but are required for multithreaded operation.

\param renderer The renderer to draw on.
\param vx Vertex array containing X coordinates of the points of the filled polygon.
\param vy Vertex array containing Y coordinates of the points of the filled polygon.
\param n Number of points in the vertex array. Minimum number is 3.
\param r The red value of the filled polygon to draw.
\param g The green value of the filled polygon to draw.
\param b The blue value of the filled polygon to draw.
\param a The alpha value of the filled polygon to draw.
\param polyInts Preallocated, temporary vertex array used for sorting vertices. Required for multithreaded operation; set to NULL otherwise.
\param polyAllocated Flag indicating if temporary vertex array was allocated. Required for multithreaded operation; set to NULL otherwise.

\returns Returns 0 on success, -1 on failure.
*/
int filledPolygonRGBAMT(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy, int n, Uint8 r, Uint8 g, Uint8 b, Uint8 a, int **polyInts, int *polyAllocated)
{
	int result;
	int i;
	int y, xa, xb;
	int miny, maxy;
	int x1, y1;
	int x2, y2;
	int ind1, ind2;
	int ints;
	int *gfxPrimitivesPolyInts = NULL;
	int *gfxPrimitivesPolyIntsNew = NULL;
	int gfxPrimitivesPolyAllocated = 0;

	/*
	* Vertex array NULL check
	*/
	if (vx == NULL) {
		return (-1);
	}
	if (vy == NULL) {
		return (-1);
	}

	/*
	* Sanity check number of edges
	*/
	if (n < 3) {
		return -1;
	}

	/*
	* Map polygon cache
	*/
	if ((polyInts==NULL) || (polyAllocated==NULL)) {
		/* Use global cache */
		gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsGlobal;
		gfxPrimitivesPolyAllocated = gfxPrimitivesPolyAllocatedGlobal;
	} else {
		/* Use local cache */
		gfxPrimitivesPolyInts = *polyInts;
		gfxPrimitivesPolyAllocated = *polyAllocated;
	}

	/*
	* Allocate temp array, only grow array
	*/
	if (!gfxPrimitivesPolyAllocated) {
		gfxPrimitivesPolyInts = (int *) DMTMALLOC(sizeof(int) * n);
		gfxPrimitivesPolyAllocated = n;
	} else {
		if (gfxPrimitivesPolyAllocated < n) {
			gfxPrimitivesPolyIntsNew = (int *) DMTREALLOC(gfxPrimitivesPolyInts, sizeof(int) * n);
			if (!gfxPrimitivesPolyIntsNew) {
				if (!gfxPrimitivesPolyInts) {
					RL_FREE(gfxPrimitivesPolyInts);
					gfxPrimitivesPolyInts = NULL;
				}
				gfxPrimitivesPolyAllocated = 0;
			} else {
				gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsNew;
				gfxPrimitivesPolyAllocated = n;
			}
		}
	}

	/*
	* Check temp array
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		gfxPrimitivesPolyAllocated = 0;
	}

	/*
	* Update cache variables
	*/
	if ((polyInts==NULL) || (polyAllocated==NULL)) {
		gfxPrimitivesPolyIntsGlobal =  gfxPrimitivesPolyInts;
		gfxPrimitivesPolyAllocatedGlobal = gfxPrimitivesPolyAllocated;
	} else {
		*polyInts = gfxPrimitivesPolyInts;
		*polyAllocated = gfxPrimitivesPolyAllocated;
	}

	/*
	* Check temp array again
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		return(-1);
	}

	/*
	* Determine Y maxima
	*/
	miny = vy[0];
	maxy = vy[0];
	for (i = 1; (i < n); i++) {
		if (vy[i] < miny) {
			miny = vy[i];
		} else if (vy[i] > maxy) {
			maxy = vy[i];
		}
	}

	/*
	* Draw, scanning y
	*/
	result = 0;
	for (y = miny; (y <= maxy); y++) {
		ints = 0;
		for (i = 0; (i < n); i++) {
			if (!i) {
				ind1 = n - 1;
				ind2 = 0;
			} else {
				ind1 = i - 1;
				ind2 = i;
			}
			y1 = vy[ind1];
			y2 = vy[ind2];
			if (y1 < y2) {
				x1 = vx[ind1];
				x2 = vx[ind2];
			} else if (y1 > y2) {
				y2 = vy[ind1];
				y1 = vy[ind2];
				x2 = vx[ind1];
				x1 = vx[ind2];
			} else {
				continue;
			}
			if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
				gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
			}
		}

		qsort(gfxPrimitivesPolyInts, ints, sizeof(int), _gfxPrimitivesCompareInt);

		/*
		* Set color
		*/
		result = 0;

		for (i = 0; (i < ints); i += 2) {
			xa = gfxPrimitivesPolyInts[i] + 1;
			xa = (xa >> 16) + ((xa & 32768) >> 15);
			xb = gfxPrimitivesPolyInts[i+1] - 1;
			xb = (xb >> 16) + ((xb & 32768) >> 15);
			result |= hline(renderer, xa, xb, y);
		}
	}

	return (result);
}


/*!
\brief Draw filled polygon with alpha blending.

\param renderer The renderer to draw on.
\param vx Vertex array containing X coordinates of the points of the filled polygon.
\param vy Vertex array containing Y coordinates of the points of the filled polygon.
\param n Number of points in the vertex array. Minimum number is 3.
\param r The red value of the filled polygon to draw.
\param g The green value of the filled polygon to draw.
\param b The blue value of the filed polygon to draw.
\param a The alpha value of the filled polygon to draw.

\returns Returns 0 on success, -1 on failure.
*/
int filledPolygonRGBA(SDL_Renderer * renderer, const Sint16 * vx, const Sint16 * vy, int n)
{
	return filledPolygonRGBAMT(renderer, vx, vy, n,0,0,0,0, NULL, NULL);
}

/* ---- Textured Polygon */

/*!
\brief Internal function to draw a textured horizontal line.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point (i.e. left) of the line.
\param x2 X coordinate of the second point (i.e. right) of the line.
\param y Y coordinate of the points of the line.
\param texture The texture to retrieve color information from.
\param texture_w The width of the texture.
\param texture_h The height of the texture.
\param texture_dx The X offset for the texture lookup.
\param texture_dy The Y offset for the textured lookup.

\returns Returns 0 on success, -1 on failure.
*/
int _HLineTextured(SDL_Renderer *renderer, Sint16 x1, Sint16 x2, Sint16 y, SDL_Texture *texture, int texture_w, int texture_h, int texture_dx, int texture_dy)
{
	Sint16 w;
	Sint16 xtmp;
	int result = 0;
	int texture_x_walker;
	int texture_y_start;
	SDL_Rect source_rect,dst_rect;
	int pixels_written,write_width;

	/*
	* Swap x1, x2 if required to ensure x1<=x2
	*/
	if (x1 > x2) {
		xtmp = x1;
		x1 = x2;
		x2 = xtmp;
	}

	/*
	* Calculate width to draw
	*/
	w = x2 - x1 + 1;

	/*
	* Determine where in the texture we start drawing
	*/
	texture_x_walker =   (x1 - texture_dx)  % texture_w;
	if (texture_x_walker < 0){
		texture_x_walker = texture_w + texture_x_walker ;
	}

	texture_y_start = (y + texture_dy) % texture_h;
	if (texture_y_start < 0){
		texture_y_start = texture_h + texture_y_start;
	}

	/* setup the source rectangle; we are only drawing one horizontal line */
	source_rect.y = texture_y_start;
	source_rect.x = texture_x_walker;
	source_rect.h = 1;

	/* we will draw to the current y */
	dst_rect.y = y;
	dst_rect.h = 1;

	/* if there are enough pixels left in the current row of the texture */
	/* draw it all at once */
	if (w <= texture_w -texture_x_walker){
		source_rect.w = w;
		source_rect.x = texture_x_walker;
		dst_rect.x= x1;
		dst_rect.w = source_rect.w;
		result = (SDL_RenderCopy(renderer, texture, &source_rect, &dst_rect) == 0);
	} else {
		/* we need to draw multiple times */
		/* draw the first segment */
		pixels_written = texture_w  - texture_x_walker;
		source_rect.w = pixels_written;
		source_rect.x = texture_x_walker;
		dst_rect.x= x1;
		dst_rect.w = source_rect.w;
		result |= (SDL_RenderCopy(renderer, texture, &source_rect, &dst_rect) == 0);
		write_width = texture_w;

		/* now draw the rest */
		/* set the source x to 0 */
		source_rect.x = 0;
		while (pixels_written < w){
			if (write_width >= w - pixels_written) {
				write_width =  w - pixels_written;
			}
			source_rect.w = write_width;
			dst_rect.x = x1 + pixels_written;
			dst_rect.w = source_rect.w;
			result |= (SDL_RenderCopy(renderer, texture, &source_rect, &dst_rect) == 0);
			pixels_written += write_width;
		}
	}

	return result;
}

/*!
\brief Draws a polygon filled with the given texture (Multi-Threading Capable).

\param renderer The renderer to draw on.
\param vx array of x vector components
\param vy array of x vector components
\param n the amount of vectors in the vx and vy array
\param texture the sdl surface to use to fill the polygon
\param texture_dx the offset of the texture relative to the screeen. If you move the polygon 10 pixels
to the left and want the texture to apear the same you need to increase the texture_dx value
\param texture_dy see texture_dx
\param polyInts Preallocated temp array storage for vertex sorting (used for multi-threaded operation)
\param polyAllocated Flag indicating oif the temp array was allocated (used for multi-threaded operation)

\returns Returns 0 on success, -1 on failure.
*/
int texturedPolygonMT(SDL_Renderer *renderer, const Sint16 * vx, const Sint16 * vy, int n,
	SDL_Surface * texture, int texture_dx, int texture_dy, int **polyInts, int *polyAllocated)
{
	int result;
	int i;
	int y, xa, xb;
	int minx,maxx,miny, maxy;
	int x1, y1;
	int x2, y2;
	int ind1, ind2;
	int ints;
	int *gfxPrimitivesPolyInts = NULL;
	int *gfxPrimitivesPolyIntsTemp = NULL;
	int gfxPrimitivesPolyAllocated = 0;
	SDL_Texture *textureAsTexture = NULL;

	/*
	* Sanity check number of edges
	*/
	if (n < 3) {
		return -1;
	}

	/*
	* Map polygon cache
	*/
	if ((polyInts==NULL) || (polyAllocated==NULL)) {
		/* Use global cache */
		gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsGlobal;
		gfxPrimitivesPolyAllocated = gfxPrimitivesPolyAllocatedGlobal;
	} else {
		/* Use local cache */
		gfxPrimitivesPolyInts = *polyInts;
		gfxPrimitivesPolyAllocated = *polyAllocated;
	}

	/*
	* Allocate temp array, only grow array
	*/
	if (!gfxPrimitivesPolyAllocated) {
		gfxPrimitivesPolyInts = (int *) DMTMALLOC(sizeof(int) * n);
		gfxPrimitivesPolyAllocated = n;
	} else {
		if (gfxPrimitivesPolyAllocated < n) {
			gfxPrimitivesPolyIntsTemp = (int *) DMTREALLOC(gfxPrimitivesPolyInts, sizeof(int) * n);
			if (gfxPrimitivesPolyIntsTemp == NULL) {
				/* Realloc failed - keeps original memory block, but fails this operation */
				return(-1);
			}
			gfxPrimitivesPolyInts = gfxPrimitivesPolyIntsTemp;
			gfxPrimitivesPolyAllocated = n;
		}
	}

	/*
	* Check temp array
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		gfxPrimitivesPolyAllocated = 0;
	}

	/*
	* Update cache variables
	*/
	if ((polyInts==NULL) || (polyAllocated==NULL)) {
		gfxPrimitivesPolyIntsGlobal =  gfxPrimitivesPolyInts;
		gfxPrimitivesPolyAllocatedGlobal = gfxPrimitivesPolyAllocated;
	} else {
		*polyInts = gfxPrimitivesPolyInts;
		*polyAllocated = gfxPrimitivesPolyAllocated;
	}

	/*
	* Check temp array again
	*/
	if (gfxPrimitivesPolyInts==NULL) {
		return(-1);
	}

	/*
	* Determine X,Y minima,maxima
	*/
	miny = vy[0];
	maxy = vy[0];
	minx = vx[0];
	maxx = vx[0];
	for (i = 1; (i < n); i++) {
		if (vy[i] < miny) {
			miny = vy[i];
		} else if (vy[i] > maxy) {
			maxy = vy[i];
		}
		if (vx[i] < minx) {
			minx = vx[i];
		} else if (vx[i] > maxx) {
			maxx = vx[i];
		}
	}

    /* Create texture for drawing */
	textureAsTexture = SDL_CreateTextureFromSurface(renderer, texture);
	if (textureAsTexture == NULL)
	{
		return -1;
	}


	/*
	* Draw, scanning y
	*/
	result = 0;
	for (y = miny; (y <= maxy); y++) {
		ints = 0;
		for (i = 0; (i < n); i++) {
			if (!i) {
				ind1 = n - 1;
				ind2 = 0;
			} else {
				ind1 = i - 1;
				ind2 = i;
			}
			y1 = vy[ind1];
			y2 = vy[ind2];
			if (y1 < y2) {
				x1 = vx[ind1];
				x2 = vx[ind2];
			} else if (y1 > y2) {
				y2 = vy[ind1];
				y1 = vy[ind2];
				x2 = vx[ind1];
				x1 = vx[ind2];
			} else {
				continue;
			}
			if ( ((y >= y1) && (y < y2)) || ((y == maxy) && (y > y1) && (y <= y2)) ) {
				gfxPrimitivesPolyInts[ints++] = ((65536 * (y - y1)) / (y2 - y1)) * (x2 - x1) + (65536 * x1);
			}
		}

		qsort(gfxPrimitivesPolyInts, ints, sizeof(int), _gfxPrimitivesCompareInt);

		for (i = 0; (i < ints); i += 2) {
			xa = gfxPrimitivesPolyInts[i] + 1;
			xa = (xa >> 16) + ((xa & 32768) >> 15);
			xb = gfxPrimitivesPolyInts[i+1] - 1;
			xb = (xb >> 16) + ((xb & 32768) >> 15);
			result |= _HLineTextured(renderer, xa, xb, y, textureAsTexture, texture->w, texture->h, texture_dx, texture_dy);
		}
	}

	SDL_RenderPresent(renderer);
	SDL_DestroyTexture(textureAsTexture);

	return (result);
}

/*!
\brief Draws a polygon filled with the given texture.

This standard version is calling multithreaded versions with NULL cache parameters.

\param renderer The renderer to draw on.
\param vx array of x vector components
\param vy array of x vector components
\param n the amount of vectors in the vx and vy array
\param texture the sdl surface to use to fill the polygon
\param texture_dx the offset of the texture relative to the screeen. if you move the polygon 10 pixels
to the left and want the texture to apear the same you need to increase the texture_dx value
\param texture_dy see texture_dx

\returns Returns 0 on success, -1 on failure.
*/
int texturedPolygon(SDL_Renderer *renderer, const Sint16 * vx, const Sint16 * vy, int n, SDL_Surface *texture, int texture_dx, int texture_dy)
{
	/*
	* Draw
	*/
	return (texturedPolygonMT(renderer, vx, vy, n, texture, texture_dx, texture_dy, NULL, NULL));
}


/* ---- Bezier curve */

/*!
\brief Internal function to calculate bezier interpolator of data array with ndata values at position 't'.

\param data Array of values.
\param ndata Size of array.
\param t Position for which to calculate interpolated value. t should be between [0, ndata].

\returns Interpolated value at position t, value[0] when t<0, value[n-1] when t>n.
*/
double _evaluateBezier (double *data, int ndata, double t)
{
	double mu, result;
	int n,k,kn,nn,nkn;
	double blend,muk,munk;

	/* Sanity check bounds */
	if (t<0.0) {
		return(data[0]);
	}
	if (t>=(double)ndata) {
		return(data[ndata-1]);
	}

	/* Adjust t to the range 0.0 to 1.0 */
	mu=t/(double)ndata;

	/* Calculate interpolate */
	n=ndata-1;
	result=0.0;
	muk = 1;
	munk = pow(1-mu,(double)n);
	for (k=0;k<=n;k++) {
		nn = n;
		kn = k;
		nkn = n - k;
		blend = muk * munk;
		muk *= mu;
		munk /= (1-mu);
		while (nn >= 1) {
			blend *= nn;
			nn--;
			if (kn > 1) {
				blend /= (double)kn;
				kn--;
			}
			if (nkn > 1) {
				blend /= (double)nkn;
				nkn--;
			}
		}
		result += data[k] * blend;
	}

	return (result);
}


/*!
\brief Draw a bezier curve with alpha blending.

\param renderer The renderer to draw on.
\param vx Vertex array containing X coordinates of the points of the bezier curve.
\param vy Vertex array containing Y coordinates of the points of the bezier curve.
\param n Number of points in the vertex array. Minimum number is 3.
\param s Number of steps for the interpolation. Minimum number is 2.


\returns Returns 0 on success, -1 on failure.
*/
int bezierRGBA(SDL_Renderer * renderer, const double *x,const double *y, int n, int s)
{
	int result;
	int i;
	double t, stepsize;
	Sint16 x1, y1, x2, y2;

	/*
	* Sanity check
	*/
	if (n < 3) {
		return (-1);
	}
	if (s < 2) {
		return (-1);
	}

	/*
	* Variable setup
	*/
	stepsize=(double)1.0/(double)s;


	/*
	* Set color
	*/
	result = 0;



	/*
	* Draw
	*/
	t=0.0;
	x1=(Sint16)lrint(_evaluateBezier(x,n+1,t));
	y1=(Sint16)lrint(_evaluateBezier(y,n+1,t));
	for (i = 0; i <= (n*s); i++) {
		t += stepsize;
		x2=(Sint16)_evaluateBezier(x,n,t);
		y2=(Sint16)_evaluateBezier(y,n,t);
		result |= line(renderer, x1, y1, x2, y2);
		x1 = x2;
		y1 = y2;
	}


	return (result);
}


/*!
\brief Draw a thick line with alpha blending.

\param renderer The renderer to draw on.
\param x1 X coordinate of the first point of the line.
\param y1 Y coordinate of the first point of the line.
\param x2 X coordinate of the second point of the line.
\param y2 Y coordinate of the second point of the line.
\param width Width of the line in pixels. Must be >0.
\param r The red value of the character to draw.
\param g The green value of the character to draw.
\param b The blue value of the character to draw.
\param a The alpha value of the character to draw.

\returns Returns 0 on success, -1 on failure.
*/
int thickLineRGBA(SDL_Renderer *renderer, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 width, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	int wh;
	double dx, dy, dx1, dy1, dx2, dy2;
	double l, wl2, nx, ny, ang, adj;
	Sint16 px[4], py[4];

	if (renderer == NULL) {
		return -1;
	}

	if (width < 1) {
		return -1;
	}

	/* Special case: thick "point" */
	if ((x1 == x2) && (y1 == y2)) {
		wh = width / 2;
		return boxRGBA(renderer, x1 - wh, y1 - wh, x2 + width, y2 + width, r, g, b, a);
	}

	/* Special case: width == 1 */
	if (width == 1) {
		return lineRGBA(renderer, x1, y1, x2, y2, r, g, b, a);
	}

	/* Calculate offsets for sides */
	dx = (double)(x2 - x1);
	dy = (double)(y2 - y1);
	l = SDL_sqrt(dx*dx + dy*dy);
	ang = SDL_atan2(dx, dy);
	adj = 0.1 + 0.9 * SDL_fabs(SDL_cos(2.0 * ang));
	wl2 = ((double)width - adj)/(2.0 * l);
	nx = dx * wl2;
	ny = dy * wl2;

	/* Build polygon */
	dx1 = (double)x1;
	dy1 = (double)y1;
	dx2 = (double)x2;
	dy2 = (double)y2;
	px[0] = (Sint16)(dx1 + ny);
	px[1] = (Sint16)(dx1 - ny);
	px[2] = (Sint16)(dx2 - ny);
	px[3] = (Sint16)(dx2 + ny);
	py[0] = (Sint16)(dy1 - nx);
	py[1] = (Sint16)(dy1 + nx);
	py[2] = (Sint16)(dy2 + nx);
	py[3] = (Sint16)(dy2 - nx);

	/* Draw polygon */
	return filledPolygonRGBA(renderer, px, py, 4);
}

int DrawCircle( Sint16 x, Sint16 y, Sint16 rad, Color color)
{

	return ellipseRGBA(gRenderer, x, y, rad, rad, color.r, color.g,color.b,color.a);
}

int DrawFilledCircle( Sint16 x, Sint16 y, Sint16 rad, Color color)
{

	return filledEllipseRGBA(gRenderer, x, y, rad, rad, color.r, color.g,color.b,color.a);
}


/*returns maximum or minimum of number*/
#define SDL_COLLIDE_MAX(a,b)	((a > b) ? a : b)
#define SDL_COLLIDE_MIN(a,b)	((a < b) ? a : b)

/*
	SDL surface test if offset (u,v) is a transparent pixel
*/
int SDL_CollideTransparentPixel(SDL_Surface *surface , int u , int v)
{
	if(SDL_MUSTLOCK(surface))
		SDL_LockSurface(surface);
	/*assert that (u,v) offsets lie within surface*/
	assert((u < surface->w) && (v < surface->h));

	int bpp = surface->format->BytesPerPixel;
	/*here p is the address to the pixel we want to retrieve*/
	Uint8 *p = (Uint8 *)surface->pixels + v * surface->pitch + u * bpp;

	Uint32 pixelcolor;

	switch(bpp)
	{
		case(1):
			pixelcolor = *p;
		break;

		case(2):
			pixelcolor = *(Uint16 *)p;
		break;

		case(3):
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				pixelcolor = p[0] << 16 | p[1] << 8 | p[2];
			else
				pixelcolor = p[0] | p[1] << 8 | p[2] << 16;
		break;

		case(4):
			pixelcolor = *(Uint32 *)p;
		break;
	}

	if(SDL_MUSTLOCK(surface))
		SDL_UnlockSurface(surface);
	/*test whether pixels color == color of transparent pixels for that surface*/
	return (pixelcolor == 0);//surface->format->colorkey);
}

/*
	SDL pixel perfect collision test
*/
int SDL_CollidePixel(SDL_Surface *as , int ax , int ay ,SDL_Surface *bs , int bx , int by, int skip)
{
	/*a - bottom right co-ordinates in world space*/
	int ax1 = ax + as->w - 1;
	int ay1 = ay + as->h - 1;

	/*b - bottom right co-ordinates in world space*/
	int bx1 = bx + bs->w - 1;
	int by1 = by + bs->h - 1;

	/*check if bounding boxes intersect*/
	if((bx1 < ax) || (ax1 < bx))
		return 0;
	if((by1 < ay) || (ay1 < by))
		return 0;


	int xstart = SDL_COLLIDE_MAX(ax,bx);
	int xend = SDL_COLLIDE_MIN(ax1,bx1);

	int ystart = SDL_COLLIDE_MAX(ay,by);
	int yend = SDL_COLLIDE_MIN(ay1,by1);

	for(int y = ystart ; y <= yend ; y += skip)
	{
		for(int x = xstart ; x <= xend ; x += skip)
		{
			if(!SDL_CollideTransparentPixel(as , x-ax , y-ay)
			&& !SDL_CollideTransparentPixel(bs , x-bx , y-by))
				return 1;
		}
	}

	return 0;
}

/*
	SDL bounding box collision test
*/
int SDL_CollideBoundingBox(SDL_Surface *sa , int ax , int ay ,
                             SDL_Surface *sb , int bx , int by)
{
	if(bx + sb->w < ax)	return 0;	//just checking if their
	if(bx > ax + sa->w)	return 0;	//bounding boxes even touch

	if(by + sb->h < ay)	return 0;
	if(by > ay + sa->h)	return 0;

	return 1;				//bounding boxes intersect
}

/*
	SDL bounding box collision tests (works on SDL_Rect's)
*/
int SDL_CollideBox(SDL_Rect a , SDL_Rect b)
{
	if(b.x + b.w < a.x)	return 0;	//just checking if their
	if(b.x > a.x + a.w)	return 0;	//bounding boxes even touch

	if(b.y + b.h < a.y)	return 0;
	if(b.y > a.y + a.h)	return 0;

	return 1;				//bounding boxes intersect
}

/*
	tests whether 2 circles intersect

	circle1 : centre (x1,y1) with radius r1
	circle2 : centre (x2,y2) with radius r2

	(allow distance between circles of offset)
*/
int SDL_CollideCircle(int x1 , int y1 , int r1 ,
                                 int x2 , int y2 , int r2 , int offset)
{
	int xdiff = x2 - x1;	// x plane difference
	int ydiff = y2 - y1;	// y plane difference

	/* distance between the circles centres squared */
	int dcentre_sq = (ydiff*ydiff) + (xdiff*xdiff);

	/* calculate sum of radiuses squared */
	int r_sum_sq = r1 + r2;	// square on seperate line, so
	r_sum_sq *= r_sum_sq;	// dont recompute r1 + r2

	return (dcentre_sq - r_sum_sq <= (offset*offset));
}

/*
	a circle intersection detection algorithm that will use
	the position of the centre of the surface as the centre of
	the circle and approximate the radius using the width and height
	of the surface (for example a rect of 4x6 would have r = 2.5).
*/
int SDL_CollideBoundingCircle(SDL_Surface *a , int x1 , int y1 ,
                                 SDL_Surface *b , int x2 , int y2 ,
                                   int offset)
{
	/* if radius is not specified
	we approximate them using SDL_Surface's
	width and height average and divide by 2*/
	int r1 = (a->w + a->h) / 4;	// same as / 2) / 2;
	int r2 = (b->w + b->h) / 4;

	x1 += a->w / 2;		// offset x and y
	y1 += a->h / 2;		// co-ordinates into
				// centre of image
	x2 += b->w / 2;
	y2 += b->h / 2;

	return SDL_CollideCircle(x1,y1,r1,x2,y2,r2,offset);
}

int RenderScaleFlip(
	SDL_Renderer* renderer, SDL_Texture* t,
	int x, int y, int w, int h, double angle, double sx, double sy, SDL_RendererFlip flip)
{
	SDL_Rect dst;
	dst.x = x - (w * sx) / 2;
	dst.y = y - (h * sy) / 2;
	dst.w = (int)(w * sx);
	dst.h = (int)(h * sy);
	return SDL_RenderCopyEx(renderer, t, NULL, &dst, angle, NULL, flip);
}
