
#include <xaudio2.h>
#include "SDL_xaudio2_winrthelpers.h"

using Windows::Devices::Enumeration::DeviceClass;
using Windows::Devices::Enumeration::DeviceInformation;
using Windows::Devices::Enumeration::DeviceInformationCollection;

HRESULT IXAudio2_GetDeviceCount(IXAudio2 * ixa2, UINT32 * devcount)
{
    auto operation = DeviceInformation::FindAllAsync(DeviceClass::AudioRender);
    while (operation->Status != Windows::Foundation::AsyncStatus::Completed)
    {
    }
 
    DeviceInformationCollection^ devices = operation->GetResults();
    *devcount = devices->Size;
    return S_OK;
}

HRESULT IXAudio2_GetDeviceDetails(IXAudio2 * unused, UINT32 index, XAUDIO2_DEVICE_DETAILS * details)
{
    auto operation = DeviceInformation::FindAllAsync(DeviceClass::AudioRender);
    while (operation->Status != Windows::Foundation::AsyncStatus::Completed)
    {
    }
 
    DeviceInformationCollection^ devices = operation->GetResults();
    if (index >= devices->Size)
    {
        return XAUDIO2_E_INVALID_CALL;
    }

    DeviceInformation^ d = devices->GetAt(index);
    if (details)
    {
        wcsncpy_s(details->DeviceID, ARRAYSIZE(details->DeviceID), d->Id->Data(), _TRUNCATE);
        wcsncpy_s(details->DisplayName, ARRAYSIZE(details->DisplayName), d->Name->Data(), _TRUNCATE);
    }
    return S_OK;
}
