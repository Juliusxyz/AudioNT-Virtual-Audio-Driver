/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#include "pcmbridge.h"

#define AUDIONT_PCM_COPY(destination, source, byteCount) \
    RtlCopyMemory((destination), (source), (byteCount))
#include "../Shared/audiont_pcm_ring.h"

namespace
{
constexpr ULONG AudioNtMicrophoneBridgeCapacity =
    AUDIONT_MIC_SAMPLE_RATE * sizeof(float) / 5;
constexpr ULONG AudioNtPcmBridgePoolTag = 'BrNA';

struct AudioNtMicrophoneBridgeState
{
    KSPIN_LOCK Lock;
    AudioNtPcmRing Ring;
    BYTE* Storage;
    volatile LONG64 LastSequence;
    volatile LONG64 DroppedBytes;
    volatile LONG64 UnderflowBytes;
};

AudioNtMicrophoneBridgeState g_AudioNtMicrophoneBridge{};
}

#pragma code_seg("INIT")
NTSTATUS AudioNtMicrophoneBridgeInitialize()
{
    KeInitializeSpinLock(&g_AudioNtMicrophoneBridge.Lock);
    g_AudioNtMicrophoneBridge.Storage = static_cast<BYTE*>(ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        AudioNtMicrophoneBridgeCapacity,
        AudioNtPcmBridgePoolTag));
    if (g_AudioNtMicrophoneBridge.Storage == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        g_AudioNtMicrophoneBridge.Storage,
        AudioNtMicrophoneBridgeCapacity);
    AudioNtPcmRingInitialize(
        &g_AudioNtMicrophoneBridge.Ring,
        g_AudioNtMicrophoneBridge.Storage,
        AudioNtMicrophoneBridgeCapacity);
    g_AudioNtMicrophoneBridge.LastSequence = 0;
    g_AudioNtMicrophoneBridge.DroppedBytes = 0;
    g_AudioNtMicrophoneBridge.UnderflowBytes = 0;
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
void AudioNtMicrophoneBridgeShutdown()
{
    PAGED_CODE();
    if (g_AudioNtMicrophoneBridge.Storage != nullptr)
    {
        ExFreePoolWithTag(
            g_AudioNtMicrophoneBridge.Storage,
            AudioNtPcmBridgePoolTag);
        g_AudioNtMicrophoneBridge.Storage = nullptr;
    }
    AudioNtPcmRingInitialize(&g_AudioNtMicrophoneBridge.Ring, nullptr, 0);
}

#pragma code_seg()
void AudioNtMicrophoneBridgeReset()
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AudioNtMicrophoneBridge.Lock, &oldIrql);
    AudioNtPcmRingInitialize(
        &g_AudioNtMicrophoneBridge.Ring,
        g_AudioNtMicrophoneBridge.Storage,
        g_AudioNtMicrophoneBridge.Storage == nullptr
            ? 0
            : AudioNtMicrophoneBridgeCapacity);
    g_AudioNtMicrophoneBridge.LastSequence = 0;
    g_AudioNtMicrophoneBridge.DroppedBytes = 0;
    g_AudioNtMicrophoneBridge.UnderflowBytes = 0;
    KeReleaseSpinLock(&g_AudioNtMicrophoneBridge.Lock, oldIrql);
}

void AudioNtMicrophoneBridgeWrite(
    const BYTE* source,
    ULONG byteCount,
    ULONGLONG sequence)
{
    if (source == nullptr ||
        byteCount == 0 ||
        g_AudioNtMicrophoneBridge.Storage == nullptr)
    {
        return;
    }

    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AudioNtMicrophoneBridge.Lock, &oldIrql);
    const size_t dropped = AudioNtPcmRingWrite(
        &g_AudioNtMicrophoneBridge.Ring,
        source,
        byteCount);
    g_AudioNtMicrophoneBridge.LastSequence = static_cast<LONG64>(sequence);
    KeReleaseSpinLock(&g_AudioNtMicrophoneBridge.Lock, oldIrql);
    if (dropped > 0)
    {
        InterlockedAdd64(
            &g_AudioNtMicrophoneBridge.DroppedBytes,
            static_cast<LONG64>(dropped));
    }
}

ULONG AudioNtMicrophoneBridgeRead(BYTE* destination, ULONG byteCount)
{
    if (destination == nullptr || byteCount == 0)
    {
        return 0;
    }

    size_t bytesRead = 0;
    if (g_AudioNtMicrophoneBridge.Storage != nullptr)
    {
        KIRQL oldIrql;
        KeAcquireSpinLock(&g_AudioNtMicrophoneBridge.Lock, &oldIrql);
        bytesRead = AudioNtPcmRingRead(
            &g_AudioNtMicrophoneBridge.Ring,
            destination,
            byteCount);
        KeReleaseSpinLock(&g_AudioNtMicrophoneBridge.Lock, oldIrql);
    }

    if (bytesRead < byteCount)
    {
        const ULONG missingBytes = byteCount - static_cast<ULONG>(bytesRead);
        RtlZeroMemory(destination + bytesRead, missingBytes);
        InterlockedAdd64(
            &g_AudioNtMicrophoneBridge.UnderflowBytes,
            static_cast<LONG64>(missingBytes));
    }

    return static_cast<ULONG>(bytesRead);
}

void AudioNtMicrophoneBridgeGetStats(AudioNtMicrophoneStats* stats)
{
    if (stats == nullptr)
    {
        return;
    }

    stats->Size = sizeof(AudioNtMicrophoneStats);
    stats->Version = AUDIONT_CONTROL_PROTOCOL_VERSION;
    stats->LastSequence = static_cast<AudioNtUInt64>(InterlockedCompareExchange64(
        &g_AudioNtMicrophoneBridge.LastSequence,
        0,
        0));
    stats->DroppedBytes = static_cast<AudioNtUInt64>(InterlockedCompareExchange64(
        &g_AudioNtMicrophoneBridge.DroppedBytes,
        0,
        0));
    stats->UnderflowBytes = static_cast<AudioNtUInt64>(InterlockedCompareExchange64(
        &g_AudioNtMicrophoneBridge.UnderflowBytes,
        0,
        0));
}
