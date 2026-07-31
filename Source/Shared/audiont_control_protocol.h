/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#define AUDIONT_CONTROL_PROTOCOL_VERSION 1u
#define AUDIONT_MIC_SAMPLE_RATE 48000u
#define AUDIONT_MIC_CHANNELS 1u
#define AUDIONT_MIC_BITS_PER_SAMPLE 32u
#define AUDIONT_MIC_MAX_FRAMES_PER_WRITE 480u

#define AUDIONT_CONTROL_NT_DEVICE_NAME L"\\Device\\AudioNTVirtualAudio"
#define AUDIONT_CONTROL_DOS_DEVICE_NAME L"\\DosDevices\\AudioNTVirtualAudio"

#if defined(CTL_CODE) && defined(FILE_DEVICE_SOUND)
#define IOCTL_AUDIONT_QUERY_CAPABILITIES \
    CTL_CODE(FILE_DEVICE_SOUND, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_AUDIONT_RESET_MICROPHONE \
    CTL_CODE(FILE_DEVICE_SOUND, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_AUDIONT_WRITE_MICROPHONE \
    CTL_CODE(FILE_DEVICE_SOUND, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_AUDIONT_GET_MICROPHONE_STATS \
    CTL_CODE(FILE_DEVICE_SOUND, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

struct AudioNtControlCapabilities
{
    uint32_t Size;
    uint32_t Version;
    uint32_t SampleRate;
    uint32_t Channels;
    uint32_t BitsPerSample;
    uint32_t MaxFramesPerWrite;
};

struct AudioNtMicrophoneWriteHeader
{
    uint32_t HeaderSize;
    uint32_t Version;
    uint64_t Sequence;
    uint32_t FrameCount;
    uint32_t PayloadBytes;
};

struct AudioNtMicrophoneStats
{
    uint32_t Size;
    uint32_t Version;
    uint64_t LastSequence;
    uint64_t DroppedBytes;
    uint64_t UnderflowBytes;
};

enum class AudioNtProtocolValidation : uint32_t
{
    Valid = 0,
    NullHeader,
    InvalidHeaderSize,
    UnsupportedVersion,
    InvalidFrameCount,
    InvalidPayloadSize,
};

constexpr AudioNtProtocolValidation AudioNtValidateMicrophoneWrite(
    const AudioNtMicrophoneWriteHeader* header,
    size_t inputLength)
{
    if (header == nullptr)
    {
        return AudioNtProtocolValidation::NullHeader;
    }
    if (inputLength < sizeof(AudioNtMicrophoneWriteHeader) ||
        header->HeaderSize != sizeof(AudioNtMicrophoneWriteHeader))
    {
        return AudioNtProtocolValidation::InvalidHeaderSize;
    }
    if (header->Version != AUDIONT_CONTROL_PROTOCOL_VERSION)
    {
        return AudioNtProtocolValidation::UnsupportedVersion;
    }
    if (header->FrameCount == 0 ||
        header->FrameCount > AUDIONT_MIC_MAX_FRAMES_PER_WRITE)
    {
        return AudioNtProtocolValidation::InvalidFrameCount;
    }

    const size_t expectedPayloadBytes =
        static_cast<size_t>(header->FrameCount) * sizeof(float);
    if (header->PayloadBytes != expectedPayloadBytes ||
        inputLength != sizeof(AudioNtMicrophoneWriteHeader) + expectedPayloadBytes)
    {
        return AudioNtProtocolValidation::InvalidPayloadSize;
    }
    return AudioNtProtocolValidation::Valid;
}
