#!/usr/bin/env python3
"""Validate the fixed AudioNT microphone endpoint format contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    source = (
        Path(__file__).resolve().parents[1] / "Source/Filters/micarraywavtable.h"
    ).read_text(encoding="utf-8")
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

    for error in errors:
        print(f"FAILED: {error}", file=sys.stderr)
    if errors:
        return 1

    print("PASS: microphone endpoint exposes fixed 48 kHz 32-bit mono PCM")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
