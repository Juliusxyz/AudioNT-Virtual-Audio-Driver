/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include "definitions.h"

NTSTATUS AudioNtPcmBridgeInitialize();
void AudioNtPcmBridgeShutdown();
void AudioNtPcmBridgeReset();
void AudioNtPcmBridgeWrite(_In_reads_bytes_(byteCount) const BYTE* source, ULONG byteCount);
ULONG AudioNtPcmBridgeRead(_Out_writes_bytes_(byteCount) BYTE* destination, ULONG byteCount);

