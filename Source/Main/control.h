/*
 * AudioNT additions are licensed under the MIT License.
 * See the repository LICENSE and THIRD_PARTY_NOTICES.md files.
 */

#pragma once

#include "definitions.h"

NTSTATUS AudioNtControlInitialize(_In_ PDRIVER_OBJECT driverObject);
void AudioNtControlShutdown(_In_ PDRIVER_OBJECT driverObject);
