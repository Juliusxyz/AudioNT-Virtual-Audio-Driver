/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include "definitions.h"
#include "../Shared/audiont_control_protocol.h"

NTSTATUS AudioNtMicrophoneBridgeInitialize();
void AudioNtMicrophoneBridgeShutdown();
void AudioNtMicrophoneBridgeReset();
void AudioNtMicrophoneBridgeWrite(
    _In_reads_bytes_(byteCount) const BYTE* source,
    ULONG byteCount,
    ULONGLONG sequence);
ULONG AudioNtMicrophoneBridgeRead(
    _Out_writes_bytes_(byteCount) BYTE* destination,
    ULONG byteCount);
void AudioNtMicrophoneBridgeGetStats(_Out_ AudioNtMicrophoneStats* stats);
