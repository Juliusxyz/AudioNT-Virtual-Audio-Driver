#!/usr/bin/env python3
"""Validate the public AudioNT endpoint contract encoded by the driver INF."""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass
from pathlib import Path


EXPECTED_ENDPOINTS = {
    ("render", "AudioNT - Game", "audiont.render.game"),
    ("render", "AudioNT - Chat", "audiont.render.chat"),
    ("render", "AudioNT - Media", "audiont.render.media"),
    ("render", "AudioNT - AUX", "audiont.render.aux"),
    ("capture", "AudioNT - Microphone", "audiont.capture.microphone"),
}


@dataclass(frozen=True)
class Endpoint:
    direction: str
    name: str
    bus_id: str


def parse_sections(text: str) -> dict[str, list[str]]:
    sections: dict[str, list[str]] = {}
    current: list[str] | None = None
    for raw_line in text.splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        match = re.fullmatch(r"\[([^]]+)]", line)
        if match:
            current = sections.setdefault(match.group(1), [])
        elif current is not None:
            current.append(line)
    return sections


def parse_strings(lines: list[str]) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in lines:
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip().strip('"')
    return values


def expand_percent(value: str, strings: dict[str, str]) -> str:
    match = re.fullmatch(r"%([^%]+)%", value.strip())
    if not match or match.group(1) not in strings:
        raise ValueError(f"unresolved string token: {value}")
    return strings[match.group(1)]


def assignment_value(lines: list[str], key: str) -> str:
    prefix = f"{key}="
    matches = [line[len(prefix) :].strip() for line in lines if line.startswith(prefix)]
    if len(matches) != 1:
        raise ValueError(f"expected one {key} assignment, found {len(matches)}")
    return matches[0]


def parse_endpoint(
    direction: str,
    wave_install_section_name: str,
    topology_install_section_name: str,
    sections: dict[str, list[str]],
    strings: dict[str, str],
) -> Endpoint:
    wave_install_lines = sections.get(wave_install_section_name)
    topology_install_lines = sections.get(topology_install_section_name)
    if wave_install_lines is None:
        raise ValueError(f"missing wave install section: {wave_install_section_name}")
    if topology_install_lines is None:
        raise ValueError(f"missing topology install section: {topology_install_section_name}")

    wave_add_reg_name = assignment_value(wave_install_lines, "AddReg")
    topology_add_reg_name = assignment_value(topology_install_lines, "AddReg")
    wave_add_reg_lines = sections.get(wave_add_reg_name)
    topology_add_reg_lines = sections.get(topology_add_reg_name)
    if wave_add_reg_lines is None:
        raise ValueError(f"missing wave AddReg section: {wave_add_reg_name}")
    if topology_add_reg_lines is None:
        raise ValueError(f"missing topology AddReg section: {topology_add_reg_name}")

    wave_friendly_lines = [line for line in wave_add_reg_lines if ",FriendlyName,," in line]
    topology_friendly_lines = [
        line for line in topology_add_reg_lines if ",FriendlyName,," in line
    ]
    bus_lines = [
        line for line in topology_add_reg_lines if "%PKEY_AudioNT_BusId%" in line
    ]
    association_lines = [
        line
        for line in topology_add_reg_lines
        if "%PKEY_AudioEndpoint_Association%" in line
    ]
    event_mode_lines = [
        line
        for line in topology_add_reg_lines
        if "%PKEY_AudioEndpoint_Supports_EventDriven_Mode%" in line
    ]
    misplaced_endpoint_properties = [
        line
        for line in wave_add_reg_lines
        if "PKEY_AudioEndpoint_" in line or "%PKEY_AudioNT_BusId%" in line
    ]
    if (
        len(wave_friendly_lines) != 1
        or len(topology_friendly_lines) != 1
        or len(bus_lines) != 1
        or len(association_lines) != 1
        or len(event_mode_lines) != 1
        or misplaced_endpoint_properties
    ):
        raise ValueError(
            f"{wave_install_section_name}/{topology_install_section_name} require matching "
            "FriendlyName values and all endpoint properties on the topology interface"
        )

    wave_friendly_token = wave_friendly_lines[0].split(",FriendlyName,,", 1)[1]
    topology_friendly_token = topology_friendly_lines[0].split(",FriendlyName,,", 1)[1]
    name = expand_percent(wave_friendly_token, strings)
    topology_name = expand_percent(topology_friendly_token, strings)
    if topology_name != name:
        raise ValueError(
            f"wave/topology FriendlyName mismatch: {name!r} != {topology_name!r}"
        )
    bus_id = bus_lines[0].rsplit(",", 1)[1].strip().strip('"')
    return Endpoint(direction, name, bus_id)


def validate(inf_path: Path) -> list[str]:
    raw = inf_path.read_bytes()
    encoding = "utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig"
    text = raw.decode(encoding)
    sections = parse_sections(text)
    strings = parse_strings(sections.get("Strings", []))
    errors: list[str] = []

    interfaces = sections.get("VIRTUALAUDIODRIVER_SA.NT.Interfaces", [])
    topology_declarations: dict[str, str] = {}
    for declaration in interfaces:
        if not declaration.startswith("AddInterface=") or "%KSCATEGORY_TOPOLOGY%" not in declaration:
            continue
        fields = [field.strip() for field in declaration.split(",")]
        if len(fields) != 3:
            errors.append(f"invalid AddInterface declaration: {declaration}")
            continue
        topology_declarations[expand_percent(fields[1], strings)] = fields[2]

    endpoints: list[Endpoint] = []
    for direction, category in (
        ("render", "%KSCATEGORY_RENDER%"),
        ("capture", "%KSCATEGORY_CAPTURE%"),
    ):
        declarations = [
            line
            for line in interfaces
            if line.startswith("AddInterface=") and category in line
        ]
        for declaration in declarations:
            fields = [field.strip() for field in declaration.split(",")]
            if len(fields) != 3:
                errors.append(f"invalid AddInterface declaration: {declaration}")
                continue
            try:
                wave_name = expand_percent(fields[1], strings)
                topology_name = f"Topology{wave_name.removeprefix('Wave')}"
                topology_install_section_name = topology_declarations.get(topology_name)
                if topology_install_section_name is None:
                    raise ValueError(f"missing topology interface for {wave_name}")
                endpoints.append(
                    parse_endpoint(
                        direction,
                        fields[2],
                        topology_install_section_name,
                        sections,
                        strings,
                    )
                )
            except ValueError as exception:
                errors.append(str(exception))

    actual = {(item.direction, item.name, item.bus_id) for item in endpoints}
    if actual != EXPECTED_ENDPOINTS:
        errors.append(
            "endpoint set mismatch: "
            f"expected={sorted(EXPECTED_ENDPOINTS)!r} actual={sorted(actual)!r}"
        )

    model_lines = sections.get("VIRTUALAUDIODRIVER.NT$ARCH$.10.0...22000", [])
    if not any("ROOT\\AudioNTVirtualAudio" in line for line in model_lines):
        errors.append("hardware ID ROOT\\AudioNTVirtualAudio is missing")
    if "ROOT\\AudioNTPrototype" in text:
        errors.append("legacy hardware ID ROOT\\AudioNTPrototype remains")

    service_lines = sections.get("VIRTUALAUDIODRIVER_SA.NT.Services", [])
    if not any(line.startswith("AddService=AudioNTVirtualAudio,") for line in service_lines):
        errors.append("driver service AudioNTVirtualAudio is missing")

    return errors


def main() -> int:
    inf_path = Path(__file__).resolve().parents[1] / "Source/Main/VirtualAudioDriver.inx"
    errors = validate(inf_path)
    for error in errors:
        print(f"FAILED: {error}", file=sys.stderr)
    if errors:
        return 1
    print("PASS: five public AudioNT endpoints match the stable bus contract")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
