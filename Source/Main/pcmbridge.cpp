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
constexpr ULONG AudioNtPcmBridgeCapacity = 48000 * 2 * 4;
constexpr ULONG AudioNtPcmBridgePoolTag = 'BrNA';

struct AudioNtPcmBridgeState
{
    KSPIN_LOCK Lock;
    AudioNtPcmRing Ring;
    BYTE* Storage;
    volatile LONG64 DroppedBytes;
    volatile LONG64 UnderflowBytes;
};

AudioNtPcmBridgeState g_AudioNtPcmBridge{};
}

#pragma code_seg("INIT")
NTSTATUS AudioNtPcmBridgeInitialize()
{
    KeInitializeSpinLock(&g_AudioNtPcmBridge.Lock);
    g_AudioNtPcmBridge.Storage = static_cast<BYTE*>(ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        AudioNtPcmBridgeCapacity,
        AudioNtPcmBridgePoolTag));
    if (g_AudioNtPcmBridge.Storage == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(g_AudioNtPcmBridge.Storage, AudioNtPcmBridgeCapacity);
    AudioNtPcmRingInitialize(
        &g_AudioNtPcmBridge.Ring,
        g_AudioNtPcmBridge.Storage,
        AudioNtPcmBridgeCapacity);
    g_AudioNtPcmBridge.DroppedBytes = 0;
    g_AudioNtPcmBridge.UnderflowBytes = 0;
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
void AudioNtPcmBridgeShutdown()
{
    PAGED_CODE();
    if (g_AudioNtPcmBridge.Storage != nullptr)
    {
        ExFreePoolWithTag(g_AudioNtPcmBridge.Storage, AudioNtPcmBridgePoolTag);
        g_AudioNtPcmBridge.Storage = nullptr;
    }
    AudioNtPcmRingInitialize(&g_AudioNtPcmBridge.Ring, nullptr, 0);
}

#pragma code_seg()
void AudioNtPcmBridgeReset()
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AudioNtPcmBridge.Lock, &oldIrql);
    AudioNtPcmRingInitialize(
        &g_AudioNtPcmBridge.Ring,
        g_AudioNtPcmBridge.Storage,
        g_AudioNtPcmBridge.Storage == nullptr ? 0 : AudioNtPcmBridgeCapacity);
    KeReleaseSpinLock(&g_AudioNtPcmBridge.Lock, oldIrql);
}

void AudioNtPcmBridgeWrite(const BYTE* source, ULONG byteCount)
{
    if (source == nullptr || byteCount == 0 || g_AudioNtPcmBridge.Storage == nullptr)
    {
        return;
    }

    KIRQL oldIrql;
    KeAcquireSpinLock(&g_AudioNtPcmBridge.Lock, &oldIrql);
    const size_t dropped = AudioNtPcmRingWrite(
        &g_AudioNtPcmBridge.Ring,
        source,
        byteCount);
    KeReleaseSpinLock(&g_AudioNtPcmBridge.Lock, oldIrql);
    if (dropped > 0)
    {
        InterlockedAdd64(
            &g_AudioNtPcmBridge.DroppedBytes,
            static_cast<LONG64>(dropped));
    }
}

ULONG AudioNtPcmBridgeRead(BYTE* destination, ULONG byteCount)
{
    if (destination == nullptr || byteCount == 0)
    {
        return 0;
    }

    size_t bytesRead = 0;
    if (g_AudioNtPcmBridge.Storage != nullptr)
    {
        KIRQL oldIrql;
        KeAcquireSpinLock(&g_AudioNtPcmBridge.Lock, &oldIrql);
        bytesRead = AudioNtPcmRingRead(
            &g_AudioNtPcmBridge.Ring,
            destination,
            byteCount);
        KeReleaseSpinLock(&g_AudioNtPcmBridge.Lock, oldIrql);
    }

    if (bytesRead < byteCount)
    {
        const ULONG missingBytes = byteCount - static_cast<ULONG>(bytesRead);
        RtlZeroMemory(destination + bytesRead, missingBytes);
        InterlockedAdd64(
            &g_AudioNtPcmBridge.UnderflowBytes,
            static_cast<LONG64>(missingBytes));
    }

    return static_cast<ULONG>(bytesRead);
}
