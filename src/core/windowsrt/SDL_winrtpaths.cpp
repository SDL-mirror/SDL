/* TODO, WinRT: include copyright info in SDL_winrtpaths.cpp
   TODO, WinRT: add note to SDL_winrtpaths.cpp mentioning that /ZW must be used when compiling the file
*/

#include "SDL_config.h"

#ifdef __WINRT__

extern "C" {
#include "SDL_system.h"
#include "../windows/SDL_windows.h"
}

using namespace Windows::Storage;

extern "C" const char * SDL_WinRTGetLocalFolderPath()
{
    static const char * path = nullptr;
    if (!path) {
        path = WIN_StringToUTF8(ApplicationData::Current->LocalFolder->Path->Data());
    }
    return path;
}

extern "C" const char * SDL_WinRTGetRoamingFolderPath()
{
    // TODO, WinRT: make SDL_WinRTGetRoamingFolderPath return NULL on Windows Phone 8
    static const char * path = nullptr;
    if (!path) {
        path = WIN_StringToUTF8(ApplicationData::Current->RoamingFolder->Path->Data());
    }
    return path;
}

extern "C" const char * SDL_WinRTGetTemporaryFolderPath()
{
    // TODO, WinRT: make SDL_WinRTGetTemporaryFolderPath return NULL on Windows Phone 8
    static const char * path = nullptr;
    if (!path) {
        path = WIN_StringToUTF8(ApplicationData::Current->TemporaryFolder->Path->Data());
    }
    return path;
}

#endif /* __WINRT__ */
