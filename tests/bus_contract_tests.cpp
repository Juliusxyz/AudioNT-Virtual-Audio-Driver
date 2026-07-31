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

    assert(AudioNtIsRenderDevice(eAudioNtGameDevice));
    assert(AudioNtIsRenderDevice(eAudioNtChatDevice));
    assert(AudioNtIsRenderDevice(eAudioNtMediaDevice));
    assert(AudioNtIsRenderDevice(eAudioNtAuxDevice));
    assert(!AudioNtIsRenderDevice(eAudioNtMicrophoneDevice));
    assert(!AudioNtIsRenderDevice(eMaxDeviceType));
    return 0;
}
