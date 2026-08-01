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

constexpr wchar_t AudioNtAsciiLower(wchar_t value)
{
    return value >= L'A' && value <= L'Z' ? value + (L'a' - L'A') : value;
}

constexpr bool AudioNtHardwareIdEquals(const wchar_t* left, const wchar_t* right)
{
    if (left == nullptr || right == nullptr)
    {
        return false;
    }

    while (*left != L'\0' && *right != L'\0')
    {
        if (AudioNtAsciiLower(*left) != AudioNtAsciiLower(*right))
        {
            return false;
        }

        ++left;
        ++right;
    }

    return *left == L'\0' && *right == L'\0';
}

constexpr eDeviceType AudioNtDeviceTypeForHardwareId(const wchar_t* hardwareId)
{
    return AudioNtHardwareIdEquals(hardwareId, L"ROOT\\AudioNTVirtualAudioGame")
        ? eAudioNtGameDevice
        : AudioNtHardwareIdEquals(hardwareId, L"ROOT\\AudioNTVirtualAudioChat")
            ? eAudioNtChatDevice
            : AudioNtHardwareIdEquals(hardwareId, L"ROOT\\AudioNTVirtualAudioMedia")
                ? eAudioNtMediaDevice
                : AudioNtHardwareIdEquals(hardwareId, L"ROOT\\AudioNTVirtualAudioAux")
                    ? eAudioNtAuxDevice
                    : AudioNtHardwareIdEquals(hardwareId, L"ROOT\\AudioNTVirtualAudioMicrophone")
                        ? eAudioNtMicrophoneDevice
                        : eMaxDeviceType;
}
