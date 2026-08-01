#!/usr/bin/env python3
"""Validate the fixed AudioNT microphone endpoint format contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path


MICROPHONE_NAME_GUID = "6AE81FF4-203E-4FE1-88AA-F2D57775CD4A"


def read_inf(path: Path) -> str:
    raw = path.read_bytes()
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    return raw.decode(encoding)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    source = (root / "Source/Filters/micarraywavtable.h").read_text(encoding="utf-8")
    inf = read_inf(root / "Source/Main/VirtualAudioDriver.inx")
    code = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    code = re.sub(r"//.*", "", code)
    required = {
        "mono channel contract": "#define MICARRAY_RAW_CHANNELS                   1",
        "32-bit PCM contract": "#define MICARRAY_32_BITS_PER_SAMPLE_PCM         32",
        "48 kHz contract": "#define MICARRAY_RAW_SAMPLE_RATE                48000",
        "PCM host-pin subtype": "STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)",
        "PCM extensible subtype": "STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM)",
        "mono channel mask": "KSAUDIO_SPEAKER_MONO",
    }

    errors = [label for label, token in required.items() if token not in code]
    if "KSDATAFORMAT_SUBTYPE_IEEE_FLOAT" in code:
        errors.append("float-only capture format remains")
    if (
        "HKR,%MediaCategories%\\%GUID.AudioNtMicrophoneEndpoint%,Name,,"
        "%VIRTUALAUDIODRIVER.WaveMicrophone.szPname%"
    ) not in inf:
        errors.append("microphone device-specific friendly name is not registered")
    if f'GUID.AudioNtMicrophoneEndpoint="{{{MICROPHONE_NAME_GUID}}}"' not in inf:
        errors.append("microphone INF name GUID is missing or mismatched")

    for error in errors:
        print(f"FAILED: {error}", file=sys.stderr)
    if errors:
        return 1

    print("PASS: microphone endpoint exposes fixed 48 kHz 32-bit mono PCM")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
