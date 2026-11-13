#!/usr/bin/env python3
"""Repair logical CP/M extents emitted separately on the Partner hard disk."""

import argparse
from pathlib import Path


DIRECTORY_OFFSET = 8192
DIRECTORY_ENTRIES = 1024
ENTRY_SIZE = 32
DELETED_ENTRY = 0xE5


def _name_field(filename: str) -> bytes:
    """Return an upper-case, space-padded CP/M 8.3 directory name."""
    stem, dot, suffix = filename.upper().partition(".")
    if len(stem) > 8 or len(suffix) > 3 or (not stem) or (not dot):
        raise ValueError(f"not a CP/M 8.3 filename: {filename}")
    return stem.encode("ascii").ljust(8) + suffix.encode("ascii").ljust(3)


def _allocation_words(entry: memoryview) -> list[int]:
    """Read the eight 16-bit allocation block numbers in an entry."""
    return [int.from_bytes(entry[index:index + 2], "little")
            for index in range(16, 32, 2)]


def _set_allocation_words(entry: memoryview, blocks: list[int]) -> None:
    """Replace an entry's allocation block list."""
    entry[16:32] = b"".join(block.to_bytes(2, "little") for block in blocks)


def repair_extents(image_path: Path, filename: str) -> int:
    """Combine pairs of 16 KiB entries into Partner 32 KiB extents."""
    image = bytearray(image_path.read_bytes())
    target = _name_field(filename)
    entries: dict[int, tuple[int, memoryview]] = {}

    for slot in range(DIRECTORY_ENTRIES):
        offset = DIRECTORY_OFFSET + slot * ENTRY_SIZE
        entry = memoryview(image)[offset:offset + ENTRY_SIZE]
        if entry[0] <= 15 and bytes(entry[1:12]) == target:
            extent = ((entry[14] & 0x3F) << 5) | entry[12]
            entries[extent] = (slot, entry)

    repaired = 0
    for extent in sorted(entries):
        if extent & 1 or extent + 1 not in entries:
            continue

        _, first = entries[extent]
        second_slot, second = entries[extent + 1]
        first_blocks = _allocation_words(first)
        second_blocks = _allocation_words(second)

        # A correct Partner entry already carries both logical extents.  The
        # current cpmdisk output instead leaves its upper four words empty and
        # creates a second directory entry, which CP/M masks to the same
        # physical extent and consequently never loads.
        if any(first_blocks[4:]) or any(second_blocks[4:]):
            continue

        combined = first_blocks[:4] + second_blocks[:4]
        first[12] = second[12]
        first[13] = second[13]
        first[14] = second[14]
        first[15] = second[15]
        _set_allocation_words(first, combined)
        image[DIRECTORY_OFFSET + second_slot * ENTRY_SIZE] = DELETED_ENTRY
        repaired += 1

    if repaired:
        image_path.write_bytes(image)
    return repaired


def main() -> None:
    """Parse command-line arguments and repair the selected disk image."""
    parser = argparse.ArgumentParser()
    parser.add_argument("image", type=Path)
    parser.add_argument("filename")
    args = parser.parse_args()
    repaired = repair_extents(args.image, args.filename)
    print(f"Repaired {repaired} Partner CP/M extent pair(s).")


if __name__ == "__main__":
    main()
