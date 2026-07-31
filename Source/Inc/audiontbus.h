/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

enum eDeviceType : unsigned long
{
    eAudioNtGameDevice = 0,
    eAudioNtChatDevice,
    eAudioNtMediaDevice,
    eAudioNtAuxDevice,
    eAudioNtMicrophoneDevice,
    eMaxDeviceType,
};

enum class AudioNtBusId : unsigned long
{
    Game = 0,
    Chat,
    Media,
    Aux,
    Microphone,
    Count,
};

constexpr bool AudioNtIsRenderDevice(eDeviceType deviceType)
{
    return deviceType <= eAudioNtAuxDevice;
}

constexpr AudioNtBusId AudioNtBusIdForDevice(eDeviceType deviceType)
{
    switch (deviceType)
    {
    case eAudioNtGameDevice:
        return AudioNtBusId::Game;
    case eAudioNtChatDevice:
        return AudioNtBusId::Chat;
    case eAudioNtMediaDevice:
        return AudioNtBusId::Media;
    case eAudioNtAuxDevice:
        return AudioNtBusId::Aux;
    case eAudioNtMicrophoneDevice:
        return AudioNtBusId::Microphone;
    default:
        return AudioNtBusId::Count;
    }
}
