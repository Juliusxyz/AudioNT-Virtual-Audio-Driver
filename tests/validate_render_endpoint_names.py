#!/usr/bin/env python3
"""Validate unique Windows endpoint pin categories and registered names."""

from __future__ import annotations

import re
import sys
from pathlib import Path


EXPECTED = {
    "Game": ("AUDIONT_GAME_ENDPOINT_CATEGORY", "D4BE6922-C928-421B-A0F3-4580106EA67D"),
    "Chat": ("AUDIONT_CHAT_ENDPOINT_CATEGORY", "B4E176EC-D673-4431-88B1-165C9FDF04AF"),
    "Media": ("AUDIONT_MEDIA_ENDPOINT_CATEGORY", "BCF8D816-5786-47EE-9096-46A40C7425C8"),
    "Aux": ("AUDIONT_AUX_ENDPOINT_CATEGORY", "260DD4B5-CC8D-478B-A150-0B9C28022BA5"),
}

DISPLAY_NAMES = {
    "Game": "AudioNT - Game",
    "Chat": "AudioNT - Chat",
    "Media": "AudioNT - Media",
    "Aux": "AudioNT - AUX",
}


def read_inf(path: Path) -> str:
    raw = path.read_bytes()
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    return raw.decode(encoding)


def read_guid(source: str, symbol: str) -> str | None:
    match = re.search(rf"DEFINE_GUID\({symbol},(.*?)\);", source, flags=re.DOTALL)
    if match is None:
        return None
    values = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]+)", match.group(1))]
    if len(values) != 11:
        return None
    return (
        f"{values[0]:08X}-{values[1]:04X}-{values[2]:04X}-"
        f"{values[3]:02X}{values[4]:02X}-"
        + "".join(f"{value:02X}" for value in values[5:])
    )


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    topology = (root / "Source/Filters/speakertoptable.h").read_text(encoding="utf-8")
    minipairs = (root / "Source/Filters/minipairs.h").read_text(encoding="utf-8")
    inf = read_inf(root / "Source/Main/VirtualAudioDriver.inx")
    errors: list[str] = []

    for endpoint, (symbol, guid) in EXPECTED.items():
        if f"AUDIONT_RENDER_TOPOLOGY_DESCRIPTOR({endpoint}, {symbol})" not in topology:
            errors.append(f"{endpoint} does not use its unique topology category")
        if read_guid(topology, symbol) != guid:
            errors.append(f"{endpoint} category GUID is missing from topology")
        registration = (
            f"HKR,%MediaCategories%\\%GUID.AudioNt{endpoint}Endpoint%,Name,,"
            f"%VIRTUALAUDIODRIVER.Wave{endpoint}.szPname%"
        )
        if registration not in inf:
            errors.append(f"{endpoint} device-specific friendly name is not registered")
        if f'GUID.AudioNt{endpoint}Endpoint="{{{guid}}}"' not in inf:
            errors.append(f"{endpoint} INF category GUID is missing or mismatched")
        if f'AUDIONT_RENDER_INTERFACE_PROPERTIES({endpoint}, L"{DISPLAY_NAMES[endpoint]}")' not in minipairs:
            errors.append(f"{endpoint} render interface friendly name is missing")
        if f"{endpoint}Miniports,\n    {endpoint}," not in minipairs:
            errors.append(f"{endpoint} render interface properties are not bound to the minipair")

    if "DEVPKEY_DeviceInterface_FriendlyName" not in minipairs:
        errors.append("render interfaces do not publish a Windows adapter friendly name")
    if "endpoint##RenderInterfaceProperties" not in minipairs:
        errors.append("render minipairs do not bind their endpoint-specific interface properties")
    wave_binding = (
        "&SpeakerWaveMiniportFilterDescriptor,                              \\\n"
        "    SIZEOF_ARRAY(endpoint##RenderInterfaceProperties),"
    )
    if wave_binding not in minipairs:
        errors.append("render WaveRT interfaces do not bind their endpoint-specific friendly names")

    if "&KSNODETYPE_SPEAKER" in topology:
        errors.append("hard-coded speaker endpoint category remains")

    for error in errors:
        print(f"FAILED: {error}", file=sys.stderr)
    if errors:
        return 1

    print("PASS: four render endpoints have unique registered Windows names")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
