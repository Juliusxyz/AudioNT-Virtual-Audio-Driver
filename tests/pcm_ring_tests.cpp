#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "../Source/Shared/audiont_pcm_ring.h"

namespace
{
void write_then_read_round_trips_across_wrap()
{
    std::array<std::uint8_t, 8> storage{};
    AudioNtPcmRing ring{};
    AudioNtPcmRingInitialize(&ring, storage.data(), storage.size());

    const std::array<std::uint8_t, 6> first{1, 2, 3, 4, 5, 6};
    AudioNtPcmRingWrite(&ring, first.data(), first.size());

    std::array<std::uint8_t, 4> prefix{};
    assert(AudioNtPcmRingRead(&ring, prefix.data(), prefix.size()) == prefix.size());
    assert((prefix == std::array<std::uint8_t, 4>{1, 2, 3, 4}));

    const std::array<std::uint8_t, 5> second{7, 8, 9, 10, 11};
    AudioNtPcmRingWrite(&ring, second.data(), second.size());

    std::array<std::uint8_t, 7> result{};
    assert(AudioNtPcmRingRead(&ring, result.data(), result.size()) == result.size());
    assert((result == std::array<std::uint8_t, 7>{5, 6, 7, 8, 9, 10, 11}));
}

void overflow_keeps_the_newest_complete_capacity()
{
    std::array<std::uint8_t, 4> storage{};
    AudioNtPcmRing ring{};
    AudioNtPcmRingInitialize(&ring, storage.data(), storage.size());

    const std::array<std::uint8_t, 7> source{1, 2, 3, 4, 5, 6, 7};
    const std::size_t dropped = AudioNtPcmRingWrite(&ring, source.data(), source.size());

    assert(dropped == 3);
    std::array<std::uint8_t, 4> result{};
    assert(AudioNtPcmRingRead(&ring, result.data(), result.size()) == result.size());
    assert((result == std::array<std::uint8_t, 4>{4, 5, 6, 7}));
}

void partial_read_returns_available_bytes_without_fabricating_audio()
{
    std::array<std::uint8_t, 8> storage{};
    AudioNtPcmRing ring{};
    AudioNtPcmRingInitialize(&ring, storage.data(), storage.size());

    const std::array<std::uint8_t, 3> source{21, 22, 23};
    AudioNtPcmRingWrite(&ring, source.data(), source.size());

    std::array<std::uint8_t, 6> result{99, 99, 99, 99, 99, 99};
    assert(AudioNtPcmRingRead(&ring, result.data(), result.size()) == source.size());
    assert(result[0] == 21);
    assert(result[1] == 22);
    assert(result[2] == 23);
    assert(result[3] == 99);
}
}

int main()
{
    write_then_read_round_trips_across_wrap();
    overflow_keeps_the_newest_complete_capacity();
    partial_read_returns_available_bytes_without_fabricating_audio();
    return 0;
}
