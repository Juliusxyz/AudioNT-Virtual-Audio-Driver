#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../Source/Shared/audiont_control_protocol.h"
#include "../Source/Shared/audiont_pcm_ring.h"

namespace
{
std::vector<std::uint8_t> make_write_request(
    std::uint32_t version,
    std::uint32_t frameCount,
    std::uint32_t payloadBytes)
{
    std::vector<std::uint8_t> bytes(
        sizeof(AudioNtMicrophoneWriteHeader) + payloadBytes,
        0);
    auto* header = reinterpret_cast<AudioNtMicrophoneWriteHeader*>(bytes.data());
    header->HeaderSize = sizeof(AudioNtMicrophoneWriteHeader);
    header->Version = version;
    header->Sequence = 42;
    header->FrameCount = frameCount;
    header->PayloadBytes = payloadBytes;
    return bytes;
}

void valid_ten_millisecond_write_is_accepted()
{
    auto request = make_write_request(
        AUDIONT_CONTROL_PROTOCOL_VERSION,
        AUDIONT_MIC_MAX_FRAMES_PER_WRITE,
        AUDIONT_MIC_MAX_FRAMES_PER_WRITE * sizeof(float));
    const auto* header =
        reinterpret_cast<const AudioNtMicrophoneWriteHeader*>(request.data());
    assert(AudioNtValidateMicrophoneWrite(header, request.size()) ==
           AudioNtProtocolValidation::Valid);
}

void mismatched_version_is_rejected()
{
    auto request = make_write_request(
        AUDIONT_CONTROL_PROTOCOL_VERSION + 1,
        1,
        sizeof(float));
    const auto* header =
        reinterpret_cast<const AudioNtMicrophoneWriteHeader*>(request.data());
    assert(AudioNtValidateMicrophoneWrite(header, request.size()) ==
           AudioNtProtocolValidation::UnsupportedVersion);
}

void oversized_write_is_rejected()
{
    auto request = make_write_request(
        AUDIONT_CONTROL_PROTOCOL_VERSION,
        AUDIONT_MIC_MAX_FRAMES_PER_WRITE + 1,
        (AUDIONT_MIC_MAX_FRAMES_PER_WRITE + 1) * sizeof(float));
    const auto* header =
        reinterpret_cast<const AudioNtMicrophoneWriteHeader*>(request.data());
    assert(AudioNtValidateMicrophoneWrite(header, request.size()) ==
           AudioNtProtocolValidation::InvalidFrameCount);
}

void truncated_payload_is_rejected()
{
    auto request = make_write_request(
        AUDIONT_CONTROL_PROTOCOL_VERSION,
        4,
        4 * sizeof(float));
    request.pop_back();
    const auto* header =
        reinterpret_cast<const AudioNtMicrophoneWriteHeader*>(request.data());
    assert(AudioNtValidateMicrophoneWrite(header, request.size()) ==
           AudioNtProtocolValidation::InvalidPayloadSize);
}

void microphone_ring_overflow_keeps_newest_data()
{
    std::array<std::uint8_t, 8> storage{};
    AudioNtPcmRing ring{};
    AudioNtPcmRingInitialize(&ring, storage.data(), storage.size());

    const std::array<std::uint8_t, 12> input{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    assert(AudioNtPcmRingWrite(&ring, input.data(), input.size()) == 4);

    std::array<std::uint8_t, 8> output{};
    assert(AudioNtPcmRingRead(&ring, output.data(), output.size()) == output.size());
    assert((output == std::array<std::uint8_t, 8>{4, 5, 6, 7, 8, 9, 10, 11}));
}
}

int main()
{
    static_assert(AUDIONT_CONTROL_PROTOCOL_VERSION == 1);
    static_assert(AUDIONT_MIC_SAMPLE_RATE == 48000);
    static_assert(AUDIONT_MIC_CHANNELS == 1);
    static_assert(AUDIONT_MIC_BITS_PER_SAMPLE == 32);
    static_assert(AUDIONT_MIC_MAX_FRAMES_PER_WRITE == 480);

    valid_ten_millisecond_write_is_accepted();
    mismatched_version_is_rejected();
    oversized_write_is_rejected();
    truncated_payload_is_rejected();
    microphone_ring_overflow_keeps_newest_data();
    return 0;
}
