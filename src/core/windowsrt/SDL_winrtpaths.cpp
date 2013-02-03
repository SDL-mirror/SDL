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

using namespace Windows::Storage;

static const wchar_t *
WINRT_CopySystemPath(Windows::Storage::StorageFolder ^ folder)
{
    const wchar_t * srcPath = folder->Path->Data();
    const size_t srcPathLen = SDL_wcslen(srcPath);
    wchar_t * destPath = (wchar_t *) SDL_calloc(srcPathLen + 1, sizeof(wchar_t));
    if (!destPath) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_wcslcpy(destPath, srcPath, srcPathLen + 1);
    return destPath;
}

extern "C" const wchar_t *
SDL_WinRTGetInstalledLocationPath()
{
    static const wchar_t * path = nullptr;
    if (!path) {
        path = WINRT_CopySystemPath(Windows::ApplicationModel::Package::Current->InstalledLocation);
    }
    return path;
}

extern "C" const wchar_t *
SDL_WinRTGetLocalFolderPath()
{
    static const wchar_t * path = nullptr;
    if (!path) {
        path = WINRT_CopySystemPath(ApplicationData::Current->LocalFolder);
    }
    return path;
}

extern "C" const wchar_t *
SDL_WinRTGetRoamingFolderPath()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    SDL_Unsupported();
    return NULL;
#else
    static const wchar_t * path = nullptr;
    if (!path) {
        path = WINRT_CopySystemPath(ApplicationData::Current->RoamingFolder);
    }
    return path;
#endif
}

extern "C" const wchar_t *
SDL_WinRTGetTemporaryFolderPath()
{
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    SDL_Unsupported();
    return NULL;
#else
    static const wchar_t * path = nullptr;
    if (!path) {
        path = WINRT_CopySystemPath(ApplicationData::Current->TemporaryFolder);
    }
    return path;
#endif
}

#endif /* __WINRT__ */
