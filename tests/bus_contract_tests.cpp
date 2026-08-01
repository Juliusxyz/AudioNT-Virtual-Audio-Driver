#include <cassert>

#include "../Source/Inc/audiontbus.h"

int main()
{
    static_assert(AudioNtBusIdForDevice(eAudioNtGameDevice) == AudioNtBusId::Game);
    static_assert(AudioNtBusIdForDevice(eAudioNtChatDevice) == AudioNtBusId::Chat);
    static_assert(AudioNtBusIdForDevice(eAudioNtMediaDevice) == AudioNtBusId::Media);
    static_assert(AudioNtBusIdForDevice(eAudioNtAuxDevice) == AudioNtBusId::Aux);
    static_assert(AudioNtBusIdForDevice(eAudioNtMicrophoneDevice) == AudioNtBusId::Microphone);
    static_assert(AudioNtBusIdForDevice(eMaxDeviceType) == AudioNtBusId::Count);

    static_assert(AudioNtDeviceTypeForHardwareId(L"ROOT\\AudioNTVirtualAudioGame") == eAudioNtGameDevice);
    static_assert(AudioNtDeviceTypeForHardwareId(L"root\\audiontvirtualaudiochat") == eAudioNtChatDevice);
    static_assert(AudioNtDeviceTypeForHardwareId(L"ROOT\\AudioNTVirtualAudioMedia") == eAudioNtMediaDevice);
    static_assert(AudioNtDeviceTypeForHardwareId(L"ROOT\\AudioNTVirtualAudioAux") == eAudioNtAuxDevice);
    static_assert(AudioNtDeviceTypeForHardwareId(L"ROOT\\AudioNTVirtualAudioMicrophone") == eAudioNtMicrophoneDevice);
    static_assert(AudioNtDeviceTypeForHardwareId(L"ROOT\\AudioNTVirtualAudio") == eMaxDeviceType);
    static_assert(AudioNtDeviceTypeForHardwareId(nullptr) == eMaxDeviceType);

    assert(AudioNtIsRenderDevice(eAudioNtGameDevice));
    assert(AudioNtIsRenderDevice(eAudioNtChatDevice));
    assert(AudioNtIsRenderDevice(eAudioNtMediaDevice));
    assert(AudioNtIsRenderDevice(eAudioNtAuxDevice));
    assert(!AudioNtIsRenderDevice(eAudioNtMicrophoneDevice));
    assert(!AudioNtIsRenderDevice(eMaxDeviceType));
    return 0;
}
