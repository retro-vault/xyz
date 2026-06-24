#!/usr/bin/env python3
"""
Split the stable core libc host harness into smaller translation units so each
linked Z80 image stays within flat-memory limits.
"""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path


TEST_START_RE = re.compile(r"^TEST\(", re.M)


def find_matching_brace(text: str, open_idx: int) -> int:
    depth = 0
    i = open_idx
    in_line_comment = False
    in_block_comment = False
    in_string = False
    in_char = False
    escape = False
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""

        if in_line_comment:
            if ch == "\n":
                in_line_comment = False
        elif in_block_comment:
            if ch == "*" and nxt == "/":
                in_block_comment = False
                i += 1
        elif in_string:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                in_string = False
        elif in_char:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == "'":
                in_char = False
        else:
            if ch == "/" and nxt == "/":
                in_line_comment = True
                i += 1
            elif ch == "/" and nxt == "*":
                in_block_comment = True
                i += 1
            elif ch == '"':
                in_string = True
            elif ch == "'":
                in_char = True
            elif ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    return i
        i += 1
    raise SystemExit("could not find matching closing brace for TEST block")


def extract_test_blocks(text: str) -> list[tuple[int, int, str]]:
    blocks: list[tuple[int, int, str]] = []
    for m in TEST_START_RE.finditer(text):
        start = m.start()
        open_brace = text.find("{", m.end())
        if open_brace < 0:
            raise SystemExit("could not find opening brace for TEST block")
        close_brace = find_matching_brace(text, open_brace)
        end = close_brace + 1
        while end < len(text) and text[end] in "\r\n\t ":
            end += 1
        blocks.append((start, end, text[start:end]))
    return blocks


def main() -> int:
    if len(sys.argv) != 5:
        print(
            f"usage: {sys.argv[0]} source.cpp total_parts part_index out.cpp",
            file=sys.stderr,
        )
        return 1

    src = Path(sys.argv[1])
    total_parts = int(sys.argv[2])
    part_index = int(sys.argv[3])
    out = Path(sys.argv[4])

    text = src.read_text()
    first_test = re.search(r"^TEST\(", text, re.M)
    main_match = re.search(r"^int main\(", text, re.M)
    if not first_test or not main_match:
        raise SystemExit("could not locate TEST blocks or main() in source")

    header = text[: first_test.start()]
    tests_region = text[first_test.start() : main_match.start()]
    footer = text[main_match.start() :]
    blocks = extract_test_blocks(tests_region)
    block_texts = [block for _, _, block in blocks]
    support_segments: list[str] = []
    cursor = 0
    for start_pos, end_pos, _ in blocks:
        gap = tests_region[cursor:start_pos]
        if gap.strip():
            support_segments.append(gap)
        cursor = end_pos
    tail = tests_region[cursor:]
    if tail.strip():
        support_segments.append(tail)

    if len(block_texts) == 0:
        raise SystemExit("no TEST blocks found")

    chunk = math.ceil(len(block_texts) / total_parts)
    start = (part_index - 1) * chunk
    end = min(start + chunk, len(block_texts))
    selected = block_texts[start:end]
    if not selected:
        raise SystemExit(f"part {part_index} selected no TEST blocks")

    unit = header
    if support_segments:
        unit += "\n".join(support_segments)
        unit += "\n"
    unit += "\n".join(selected)
    unit += "\n"
    unit += footer
    unit = unit.replace(
        '#include "libc_symbols.hpp"',
        f'#include "libc_core_symbols_{part_index}.hpp"\n#define LIBC_CORE_PART {part_index}',
    )
    out.write_text(unit)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
