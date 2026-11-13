#!/usr/bin/env python3
"""Build a ROM-bootable Partner P disk with a valid CP/M 3 filesystem."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path


SECTOR_SIZE = 256
SECTORS_PER_TRACK = 18
LOGICAL_TRACKS = 154
BOOT_TRACKS = 2
DISK_SECTORS = LOGICAL_TRACKS * SECTORS_PER_TRACK
DISK_SIZE = DISK_SECTORS * SECTOR_SIZE
CPM_DIRECTORY_LBA = BOOT_TRACKS * SECTORS_PER_TRACK
CPM3_LABEL_STATUS = 0x20
CPM3_LABEL_NAME = b"BAREMETL   "

LOADER_LBA = 22
LOADER_SLOT_SECTORS = 8
LOADER_SLOT_SIZE = LOADER_SLOT_SECTORS * SECTOR_SIZE
ROM_PRELOAD_SECTORS = 31
MAX_PAYLOAD_SECTORS = 192
LOADER_ADDRESS = 0xF600
BANKED_END = 0xC000

MANIFEST_MAGIC = b"BMOS"
MANIFEST_VERSION = 3
MANIFEST_FLAG_BANK2 = 0x01
MANIFEST_FLAG_START_BANK2 = 0x02
MANIFEST_BANK1_OFFSET = 9
MANIFEST_BANK2_OFFSET = 26
MANIFEST_DESCRIPTOR_SIZE = 17
MANIFEST_CHECKSUM_OFFSET = 43

CPMDISK_GEOMETRY = (
    "--seclen", "256",
    "--tracks", "154",
    "--sectrk", "18",
    "--blocksize", "2048",
    "--maxdir", "128",
    "--skew", "0",
    "--boottrk", "2",
)


def parse_u16(value: str) -> int:
    """Parse a decimal or 0x-prefixed unsigned 16-bit value."""
    parsed = int(value, 0)
    if not 0 <= parsed <= 0xFFFF:
        raise argparse.ArgumentTypeError("value must fit in 16 bits")
    return parsed


def canonical_cpm_name(value: str) -> tuple[str, bytes]:
    """Return display and space-padded 8+3 forms for a CP/M filename."""
    name = Path(value).name.upper()
    if name.count(".") > 1:
        raise ValueError(f"{value!r}: CP/M filename may contain one dot")
    stem, dot, suffix = name.partition(".")
    if not stem or len(stem) > 8 or len(suffix) > 3:
        raise ValueError(f"{value!r}: filename must fit CP/M 8.3")
    invalid = set("<>.,;:=?*[]/\\|")
    if any(ord(char) < 0x21 or ord(char) > 0x7E or char in invalid
           for char in stem + suffix):
        raise ValueError(f"{value!r}: filename contains unsupported characters")
    display = stem + (("." + suffix) if dot else "")
    canonical = stem.ljust(8).encode("ascii") + suffix.ljust(3).encode("ascii")
    return display, canonical


def build_manifest(
    payload_name: bytes,
    payload_sectors: int,
    load_address: int,
    entry_address: int,
    bank2_name: bytes | None = None,
    bank2_sectors: int = 0,
    bank2_load_address: int = 0,
    bank2_entry_address: int = 0,
    start_bank: int = 1,
) -> bytes:
    """Create the sector-zero record checked by the ROM and loader."""
    record = bytearray(SECTOR_SIZE)
    record[0:3] = bytes((0xC3, 0x00, 0xE0))  # JP E000: accepted ROM gate
    record[3:7] = MANIFEST_MAGIC
    record[7] = MANIFEST_VERSION
    if bank2_name is not None:
        record[8] |= MANIFEST_FLAG_BANK2
        if start_bank == 2:
            record[8] |= MANIFEST_FLAG_START_BANK2

    write_manifest_descriptor(
        record,
        MANIFEST_BANK1_OFFSET,
        payload_name,
        payload_sectors,
        load_address,
        entry_address,
    )
    if bank2_name is not None:
        write_manifest_descriptor(
            record,
            MANIFEST_BANK2_OFFSET,
            bank2_name,
            bank2_sectors,
            bank2_load_address,
            bank2_entry_address,
        )
    record[MANIFEST_CHECKSUM_OFFSET] = (
        -sum(record[3:MANIFEST_CHECKSUM_OFFSET])
    ) & 0xFF
    return bytes(record)


def write_manifest_descriptor(
    record: bytearray,
    offset: int,
    name: bytes,
    sectors: int,
    load_address: int,
    entry_address: int,
) -> None:
    """Write one packed user/name/size/load/entry descriptor."""
    descriptor = bytearray(MANIFEST_DESCRIPTOR_SIZE)
    descriptor[0] = 0  # CP/M user area
    descriptor[1:12] = name
    descriptor[12] = sectors
    descriptor[13:15] = load_address.to_bytes(2, "little")
    descriptor[15:17] = entry_address.to_bytes(2, "little")
    record[offset : offset + MANIFEST_DESCRIPTOR_SIZE] = descriptor


def validate_payload_layout(
    description: str,
    payload: bytes,
    load_address: int,
    entry_address: int,
    limit: int,
) -> int:
    """Validate one payload and return its padded physical-sector count."""
    if not payload:
        raise SystemExit(f"{description} must not be empty")
    if load_address & 0xFF:
        raise SystemExit(f"{description} load address must be 256-byte aligned")

    sectors = (len(payload) + SECTOR_SIZE - 1) // SECTOR_SIZE
    if sectors > MAX_PAYLOAD_SECTORS:
        raise SystemExit(
            f"{description} requires {sectors} sectors; loader maximum is "
            f"{MAX_PAYLOAD_SECTORS}"
        )
    load_end = load_address + sectors * SECTOR_SIZE
    if load_end > limit:
        raise SystemExit(
            f"{description} ends at 0x{load_end:04x}; it must end at or below "
            f"0x{limit:04x}"
        )
    if not load_address <= entry_address < load_end:
        raise SystemExit(
            f"{description} entry address must lie inside the loaded payload"
        )
    return sectors


def run_cpmdisk(cpmdisk: Path, *arguments: str) -> None:
    """Run cpmdisk with an argument list and surface failures unchanged."""
    subprocess.run((str(cpmdisk), *arguments), check=True)


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Create a 77x2x18x256 Partner P boot disk with a CP/M 3 filesystem."
        )
    )
    parser.add_argument("--cpmdisk", required=True, type=Path)
    parser.add_argument("--loader", required=True, type=Path)
    parser.add_argument("--payload", required=True, type=Path)
    parser.add_argument("--payload-name", default="SYSTEM.BIN")
    parser.add_argument("--bank2-payload", type=Path)
    parser.add_argument("--bank2-payload-name", default="BANK2.BIN")
    parser.add_argument("--extra-file", action="append", default=[], type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--load-address", type=parse_u16, default=0x0100)
    parser.add_argument("--entry-address", type=parse_u16)
    parser.add_argument("--bank2-load-address", type=parse_u16, default=0x0100)
    parser.add_argument("--bank2-entry-address", type=parse_u16)
    parser.add_argument("--start-bank", type=int, choices=(1, 2), default=1)
    args = parser.parse_args()

    loader = args.loader.read_bytes()
    payload = args.payload.read_bytes()
    bank2_payload = (
        args.bank2_payload.read_bytes()
        if args.bank2_payload is not None else None
    )
    entry_address = (
        args.load_address if args.entry_address is None else args.entry_address
    )
    bank2_entry_address = (
        args.bank2_load_address
        if args.bank2_entry_address is None else args.bank2_entry_address
    )

    if len(loader) != LOADER_SLOT_SIZE:
        raise SystemExit(
            f"loader must be the fixed {LOADER_SLOT_SIZE}-byte slot; got "
            f"{len(loader)} bytes"
        )
    if args.start_bank == 2 and bank2_payload is None:
        raise SystemExit("start bank 2 requires --bank2-payload")

    try:
        payload_display, payload_canonical = canonical_cpm_name(args.payload_name)
        bank2_display, bank2_canonical = (
            canonical_cpm_name(args.bank2_payload_name)
            if bank2_payload is not None else (None, None)
        )
        extra_names = [
            canonical_cpm_name(path.name)[0] for path in args.extra_file
        ]
    except ValueError as error:
        raise SystemExit(str(error)) from error

    all_names = [payload_display]
    if bank2_display is not None:
        all_names.append(bank2_display)
    all_names.extend(extra_names)
    if len(set(all_names)) != len(all_names):
        raise SystemExit("all payload and extra CP/M filenames must be unique")
    for path in args.extra_file:
        if not path.is_file():
            raise SystemExit(f"extra file does not exist: {path}")

    payload_limit = BANKED_END
    payload_sectors = validate_payload_layout(
        "bank-1 payload",
        payload,
        args.load_address,
        entry_address,
        payload_limit,
    )
    bank2_sectors = 0
    if bank2_payload is not None:
        bank2_sectors = validate_payload_layout(
            "bank-2 payload",
            bank2_payload,
            args.bank2_load_address,
            bank2_entry_address,
            BANKED_END,
        )

    args.output.parent.mkdir(parents=True, exist_ok=True)

    # Keep staging below the output directory so every intermediate remains
    # within the project's container-visible bind mount.
    with tempfile.TemporaryDirectory(
        prefix="baremetal-cpm-", dir=args.output.parent
    ) as temporary:
        staging = Path(temporary)
        staged_image = staging / "disk.img"
        staged_payload = staging / payload_display
        staged_payload.write_bytes(
            payload.ljust(payload_sectors * SECTOR_SIZE, b"\x00")
        )
        staged_bank2 = (
            staging / bank2_display if bank2_display is not None else None
        )
        if staged_bank2 is not None and bank2_payload is not None:
            staged_bank2.write_bytes(
                bank2_payload.ljust(bank2_sectors * SECTOR_SIZE, b"\x00")
            )

        run_cpmdisk(
            args.cpmdisk,
            "create", str(staged_image), "--cpm3", "--label", "BAREMETL",
            *CPMDISK_GEOMETRY,
        )
        run_cpmdisk(
            args.cpmdisk,
            "add", str(staged_image), "-u", "0", str(staged_payload),
            *CPMDISK_GEOMETRY,
        )
        if staged_bank2 is not None:
            run_cpmdisk(
                args.cpmdisk,
                "add", str(staged_image), "-u", "0", str(staged_bank2),
                *CPMDISK_GEOMETRY,
            )
        for source in args.extra_file:
            staged_extra = staging / source.name.upper()
            shutil.copyfile(source, staged_extra)
            run_cpmdisk(
                args.cpmdisk,
                "add", str(staged_image), "-u", "0", str(staged_extra),
                *CPMDISK_GEOMETRY,
            )

        image = bytearray(staged_image.read_bytes())
        if len(image) != DISK_SIZE:
            raise SystemExit(
                f"cpmdisk created {len(image)} bytes; expected Partner P size "
                f"{DISK_SIZE}"
            )

        # CP/M 2.2 and CP/M 3 deliberately share the ordinary file-entry and
        # allocation formats.  Require the CP/M 3-only disk-label entry so
        # --cpm3 can never be removed or ignored without failing the build.
        directory_offset = CPM_DIRECTORY_LBA * SECTOR_SIZE
        label = image[directory_offset : directory_offset + 32]
        if (label[0] != CPM3_LABEL_STATUS or
                bytes(label[1:12]) != CPM3_LABEL_NAME):
            raise SystemExit(
                "cpmdisk did not create the required CP/M 3 BAREMETL label"
            )

        image[0:SECTOR_SIZE] = build_manifest(
            payload_canonical,
            payload_sectors,
            args.load_address,
            entry_address,
            bank2_canonical,
            bank2_sectors,
            args.bank2_load_address,
            bank2_entry_address,
            args.start_bank,
        )
        loader_offset = LOADER_LBA * SECTOR_SIZE
        image[loader_offset : loader_offset + LOADER_SLOT_SIZE] = loader
        args.output.write_bytes(image)

    bank2_summary = ""
    if bank2_display is not None:
        bank2_summary = (
            f"; bank2=0:{bank2_display} ({bank2_sectors} sectors), "
            f"load=0x{args.bank2_load_address:04x}, "
            f"entry=0x{bank2_entry_address:04x}"
        )
    print(
        f"{args.output}: {len(image)} bytes; CP/M 3 Partner P; "
        f"ROM preload=LBAs 0..{ROM_PRELOAD_SECTORS - 1}; "
        f"loader=LBA {LOADER_LBA} ({LOADER_SLOT_SECTORS} sectors); "
        f"payload=0:{payload_display} ({payload_sectors} sectors), "
        f"load=0x{args.load_address:04x}, entry=0x{entry_address:04x}"
        f"{bank2_summary}; start=bank{args.start_bank}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
