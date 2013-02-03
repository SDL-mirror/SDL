/* TODO, WinRT: include copyright info in SDL_winrtpaths.cpp
   TODO, WinRT: add note to SDL_winrtpaths.cpp mentioning that /ZW must be used when compiling the file
*/

#include "SDL_config.h"

#ifdef __WINRT__

extern "C" {
#include "SDL_error.h"
#include "SDL_stdinc.h"
#include "SDL_system.h"
#include "../windows/SDL_windows.h"
}

#include <string>

using namespace std;
using namespace Windows::Storage;

extern "C" const wchar_t *
SDL_WinRTGetInstalledLocationPath()
{
    static wstring path;
    if (path.empty()) {
        path = Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data();
    }
    return path.c_str();
}

extern "C" const wchar_t *
SDL_WinRTGetLocalFolderPath()
{
    static wstring path;
    if (path.empty()) {
        path = ApplicationData::Current->LocalFolder->Path->Data();
    }
    return path.c_str();
}

extern "C" const wchar_t *
SDL_WinRTGetRoamingFolderPath()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    SDL_Unsupported();
    return NULL;
#else
    static wstring path;
    if (path.empty()) {
        path = ApplicationData::Current->RoamingFolder->Path->Data();
    }
    return path.c_str();
#endif
}

extern "C" const wchar_t *
SDL_WinRTGetTemporaryFolderPath()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    SDL_Unsupported();
    return NULL;
#else
    static wstring path;
    if (path.empty()) {
        path = ApplicationData::Current->TemporaryFolder->Path->Data();
    }
    return path.c_str();
#endif
}

#endif /* __WINRT__ */
