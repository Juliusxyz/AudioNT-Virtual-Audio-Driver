/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef AUDIONT_PCM_COPY
#include <string.h>
#define AUDIONT_PCM_COPY(destination, source, byteCount) \
    memcpy((destination), (source), (byteCount))
#endif

struct AudioNtPcmRing
{
    uint8_t* Data;
    size_t Capacity;
    size_t ReadOffset;
    size_t WriteOffset;
    size_t Available;
};

static inline void AudioNtPcmRingInitialize(
    AudioNtPcmRing* ring,
    uint8_t* storage,
    size_t capacity)
{
    if (ring == nullptr)
    {
        return;
    }

    ring->Data = storage;
    ring->Capacity = storage == nullptr ? 0 : capacity;
    ring->ReadOffset = 0;
    ring->WriteOffset = 0;
    ring->Available = 0;
}

static inline size_t AudioNtPcmRingWrite(
    AudioNtPcmRing* ring,
    const uint8_t* source,
    size_t byteCount)
{
    if (
        ring == nullptr ||
        ring->Data == nullptr ||
        ring->Capacity == 0 ||
        source == nullptr ||
        byteCount == 0)
    {
        return 0;
    }

    size_t dropped = 0;
    if (byteCount >= ring->Capacity)
    {
        dropped = ring->Available + byteCount - ring->Capacity;
        source += byteCount - ring->Capacity;
        byteCount = ring->Capacity;
        ring->ReadOffset = 0;
        ring->WriteOffset = 0;
        ring->Available = 0;
    }
    else
    {
        const size_t freeBytes = ring->Capacity - ring->Available;
        if (byteCount > freeBytes)
        {
            dropped = byteCount - freeBytes;
            ring->ReadOffset = (ring->ReadOffset + dropped) % ring->Capacity;
            ring->Available -= dropped;
        }
    }

    const size_t firstWrite =
        byteCount < ring->Capacity - ring->WriteOffset
            ? byteCount
            : ring->Capacity - ring->WriteOffset;
    AUDIONT_PCM_COPY(ring->Data + ring->WriteOffset, source, firstWrite);

    const size_t secondWrite = byteCount - firstWrite;
    if (secondWrite > 0)
    {
        AUDIONT_PCM_COPY(ring->Data, source + firstWrite, secondWrite);
    }

    ring->WriteOffset = (ring->WriteOffset + byteCount) % ring->Capacity;
    ring->Available += byteCount;
    return dropped;
}

static inline size_t AudioNtPcmRingRead(
    AudioNtPcmRing* ring,
    uint8_t* destination,
    size_t requestedBytes)
{
    if (
        ring == nullptr ||
        ring->Data == nullptr ||
        ring->Capacity == 0 ||
        destination == nullptr ||
        requestedBytes == 0)
    {
        return 0;
    }

    const size_t byteCount =
        requestedBytes < ring->Available ? requestedBytes : ring->Available;
    const size_t firstRead =
        byteCount < ring->Capacity - ring->ReadOffset
            ? byteCount
            : ring->Capacity - ring->ReadOffset;
    AUDIONT_PCM_COPY(destination, ring->Data + ring->ReadOffset, firstRead);

    const size_t secondRead = byteCount - firstRead;
    if (secondRead > 0)
    {
        AUDIONT_PCM_COPY(destination + firstRead, ring->Data, secondRead);
    }

    ring->ReadOffset = (ring->ReadOffset + byteCount) % ring->Capacity;
    ring->Available -= byteCount;
    return byteCount;
}
