/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include <stddef.h>

#if defined(_MSC_VER)
using AudioNtUInt32 = unsigned __int32;
using AudioNtUInt64 = unsigned __int64;
#else
using AudioNtUInt32 = __UINT32_TYPE__;
using AudioNtUInt64 = __UINT64_TYPE__;
#endif

static_assert(sizeof(AudioNtUInt32) == 4, "AudioNT wire uint32 must be 4 bytes");
static_assert(sizeof(AudioNtUInt64) == 8, "AudioNT wire uint64 must be 8 bytes");

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
    AudioNtUInt32 Size;
    AudioNtUInt32 Version;
    AudioNtUInt32 SampleRate;
    AudioNtUInt32 Channels;
    AudioNtUInt32 BitsPerSample;
    AudioNtUInt32 MaxFramesPerWrite;
};

struct AudioNtMicrophoneWriteHeader
{
    AudioNtUInt32 HeaderSize;
    AudioNtUInt32 Version;
    AudioNtUInt64 Sequence;
    AudioNtUInt32 FrameCount;
    AudioNtUInt32 PayloadBytes;
};

struct AudioNtMicrophoneStats
{
    AudioNtUInt32 Size;
    AudioNtUInt32 Version;
    AudioNtUInt64 LastSequence;
    AudioNtUInt64 DroppedBytes;
    AudioNtUInt64 UnderflowBytes;
};

enum class AudioNtProtocolValidation : AudioNtUInt32
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
