#!/usr/bin/env bash
# library_format_roundtrip_test.sh
#
# Convert every library assembly source SDCC -> GNU -> SDCC, assemble the GNU
# form with GNU as and xas, assemble the SDCC round-trip form with sdasz80 and
# xas, normalize every object through ELF, then compare their canonical section
# payloads and relocations.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

X_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
REPO_ROOT="$(cd "$X_ROOT/.." && pwd)"
XAS="${XAS:-$REPO_ROOT/bin/x/bin/xas}"
XOBJCOPY="${XOBJCOPY:-$REPO_ROOT/bin/x/bin/xobjcopy}"
SDAS="${SDAS:-sdasz80}"
GNU_PREFIX="${Z80_GNU_PREFIX:-/usr/local/z80-elf/bin/z80-unknown-elf-}"
GAS="${GAS:-${GNU_PREFIX}as}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[0;33m'
RESET=$'\033[0m'

LIMIT="${LIMIT:-0}"
LIBC_DEFINES=(
    -D__XCC_LIBC_FLOAT="${LIBC_FLOAT:-1}"
    -D__XCC_LIBC_DOUBLE="${LIBC_DOUBLE:-1}"
    -D__XCC_LIBC_LONG="${LIBC_LONG:-1}"
    -D__XCC_LIBC_LONGLONG="${LIBC_LONGLONG:-1}"
)

if ! command -v "$SDAS" >/dev/null 2>&1; then
    echo "${YELLOW}SKIP${RESET}: sdasz80 not found"
    exit 0
fi
if ! command -v "$GAS" >/dev/null 2>&1; then
    echo "${YELLOW}SKIP${RESET}: GNU Z80 assembler not found: $GAS"
    exit 0
fi
if [[ ! -x "$XAS" ]]; then
    echo "${RED}FAIL${RESET}: xas not found: $XAS"
    exit 1
fi
if [[ ! -x "$XOBJCOPY" ]]; then
    echo "${RED}FAIL${RESET}: xobjcopy not found: $XOBJCOPY"
    exit 1
fi

TMPDIR="$(mktemp -d /tmp/xas_libfmt_XXXXXX)"
trap 'rm -rf "$TMPDIR"' EXIT

SRCROOT="$TMPDIR/src"
WORKROOT="$TMPDIR/work"
mkdir -p "$SRCROOT" "$WORKROOT"

copy_tree() {
    local root="$1"
    (cd "$X_ROOT" && find "$root" -name '*.s' -print0) | while IFS= read -r -d '' file; do
        mkdir -p "$SRCROOT/$(dirname "$file")"
        cp "$X_ROOT/$file" "$SRCROOT/$file"
    done
}

copy_tree "libc/src"
copy_tree "runtime"
copy_tree "platforms"

canonicalize_elf() {
    local elf_file="$1"
    local out_file="$2"
    python3 - "$elf_file" "$out_file" <<'PY'
import struct
import sys

elf_path, out_path = sys.argv[1], sys.argv[2]
data = open(elf_path, "rb").read()

ALIASES = {
    "_CODE": ".text",
    "text": ".text",
    ".text": ".text",
    "_DATA": ".data",
    "data": ".data",
    ".data": ".data",
    "_BSS": ".bss",
    "bss": ".bss",
    ".bss": ".bss",
    "_RODATA": ".rodata",
    "_CONST": ".rodata",
    "rodata": ".rodata",
    ".rodata": ".rodata",
    ".rdata": ".rodata",
    "tdata": ".tdata",
    ".tdata": ".tdata",
}

def canon_section(name):
    return ALIASES.get(name, name)

if data[:4] != b"\x7fELF":
    raise SystemExit(f"not an ELF file: {elf_path}")
if data[4] != 1 or data[5] != 1:
    raise SystemExit(f"unsupported ELF class/data encoding: {elf_path}")

EHDR_FMT = "<16sHHIIIIIHHHHHH"
SHDR_FMT = "<IIIIIIIIII"
SYMTAB_FMT = "<IIIBBH"

ehdr = struct.unpack_from(EHDR_FMT, data, 0)
e_shoff = ehdr[6]
e_shentsize = ehdr[11]
e_shnum = ehdr[12]
e_shstrndx = ehdr[13]

sections = []
for i in range(e_shnum):
    off = e_shoff + i * e_shentsize
    sh = struct.unpack_from(SHDR_FMT, data, off)
    sections.append({
        "name_off": sh[0],
        "type": sh[1],
        "flags": sh[2],
        "addr": sh[3],
        "offset": sh[4],
        "size": sh[5],
        "link": sh[6],
        "info": sh[7],
        "addralign": sh[8],
        "entsize": sh[9],
    })

shstr = sections[e_shstrndx]
shstrtab = data[shstr["offset"]:shstr["offset"] + shstr["size"]]

def cstring(blob, start):
    end = blob.find(b"\0", start)
    if end < 0:
        end = len(blob)
    return blob[start:end].decode("utf-8", "replace")

for sec in sections:
    sec["name"] = cstring(shstrtab, sec["name_off"]) if sec["name_off"] < len(shstrtab) else ""
    sec["canon_name"] = canon_section(sec["name"])

symbols = {}
for sec_index, sec in enumerate(sections):
    if sec["type"] != 2:  # SHT_SYMTAB
        continue
    strtab = sections[sec["link"]]
    strtab_blob = data[strtab["offset"]:strtab["offset"] + strtab["size"]]
    count = 0 if sec["entsize"] == 0 else sec["size"] // sec["entsize"]
    for idx in range(count):
        off = sec["offset"] + idx * sec["entsize"]
        st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
            SYMTAB_FMT, data, off)
        if st_name < len(strtab_blob):
            name = cstring(strtab_blob, st_name)
        else:
            name = ""
        if not name and st_shndx < len(sections):
            name = f"SECTION:{sections[st_shndx]['canon_name']}"
        symbols[(sec_index, idx)] = name

section_chunks = []
reloc_chunks = []

for sec in sections:
    if sec["name"] in ("", ".symtab", ".strtab", ".shstrtab"):
        continue
    if sec["flags"] & 0x2 and sec["type"] in (1, 8) and sec["size"] != 0:
        lines = [f"SECTION {sec['canon_name']} type={sec['type']} size={sec['size']}"]
        if sec["type"] == 1:
            blob = data[sec["offset"]:sec["offset"] + sec["size"]]
            lines.append(blob.hex())
        else:
            lines.append(f"NOBITS {sec['size']}")
        section_chunks.append((sec["canon_name"], "\n".join(lines)))

for sec_index, sec in enumerate(sections):
    if sec["type"] not in (4, 9):  # SHT_RELA / SHT_REL
        continue
    target_name = sections[sec["info"]]["canon_name"] if sec["info"] < len(sections) else "<bad>"
    count = 0 if sec["entsize"] == 0 else sec["size"] // sec["entsize"]
    lines = [f"RELOCS target={target_name} count={count}"]
    rows = []
    for idx in range(count):
        off = sec["offset"] + idx * sec["entsize"]
        if sec["type"] == 4:
            r_offset, r_info, r_addend = struct.unpack_from("<III", data, off)
        else:
            r_offset, r_info = struct.unpack_from("<II", data, off)
            r_addend = 0
        sym_index = r_info >> 8
        rel_type = r_info & 0xff
        sym_name = symbols.get((sec["link"], sym_index), f"SYMIDX:{sym_index}")
        rows.append((r_offset, f"{target_name} off=0x{r_offset:x} type={rel_type} sym={sym_name} addend={r_addend}"))
    for _, row in sorted(rows):
        lines.append(row)
    reloc_chunks.append((target_name, "\n".join(lines)))

with open(out_path, "w", encoding="utf-8") as out:
    for _, chunk in sorted(section_chunks):
        out.write(chunk + "\n")
    for _, chunk in sorted(reloc_chunks):
        out.write(chunk + "\n")
PY
}

compare_one() {
    local src_rel="$1"
    local rel_path="${src_rel#$SRCROOT/}"
    local stem="${rel_path%.s}"
    local workdir="$WORKROOT/$stem"
    mkdir -p "$workdir"

    local gnu_src="$workdir/$(basename "$stem").gnu.s"
    local sdcc_src="$workdir/$(basename "$stem").roundtrip.sdcc.s"
    local gnu_as_o="$workdir/gnu_as.o"
    local xas_gnu_o="$workdir/xas_gnu.o"
    local sdas_rel="$workdir/sdas.rel"
    local xas_sdcc_rel="$workdir/xas_sdcc.rel"
    local sdas_o="$workdir/sdas.o"
    local xas_sdcc_o="$workdir/xas_sdcc.o"
    local gnu_as_canon="$workdir/gnu_as.canon"
    local xas_gnu_canon="$workdir/xas_gnu.canon"
    local sdas_canon="$workdir/sdas.canon"
    local xas_sdcc_canon="$workdir/xas_sdcc.canon"

    if ! "$XAS" --mode=sdcc --format=gnu "${LIBC_DEFINES[@]}" -o "$gnu_src" "$src_rel" \
            >"$workdir/xas_fmt_gnu.out" 2>"$workdir/xas_fmt_gnu.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xas sdcc->gnu format]"
        sed -n '1,20p' "$workdir/xas_fmt_gnu.err"
        return 1
    fi
    if ! "$GAS" -o "$gnu_as_o" "$gnu_src" \
            >"$workdir/gas.out" 2>"$workdir/gas.err"; then
        echo "${RED}FAIL${RESET} $rel_path [GNU as assemble]"
        sed -n '1,20p' "$workdir/gas.err"
        return 1
    fi
    if ! "$XAS" --mode=gnu -o "$xas_gnu_o" "$gnu_src" \
            >"$workdir/xas_gnu.out" 2>"$workdir/xas_gnu.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xas GNU assemble]"
        sed -n '1,20p' "$workdir/xas_gnu.err"
        return 1
    fi

    if ! "$XAS" --mode=gnu --format=sdcc -o "$sdcc_src" "$gnu_src" \
            >"$workdir/xas_fmt_sdcc.out" 2>"$workdir/xas_fmt_sdcc.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xas gnu->sdcc format]"
        sed -n '1,20p' "$workdir/xas_fmt_sdcc.err"
        return 1
    fi
    if ! "$SDAS" -o "$sdas_rel" "$sdcc_src" \
            >"$workdir/sdas.out" 2>"$workdir/sdas.err"; then
        echo "${RED}FAIL${RESET} $rel_path [sdasz80 assemble]"
        sed -n '1,20p' "$workdir/sdas.err"
        return 1
    fi
    if ! "$XAS" --mode=sdcc -o "$xas_sdcc_rel" "$sdcc_src" \
            >"$workdir/xas_sdcc.out" 2>"$workdir/xas_sdcc.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xas SDCC assemble]"
        sed -n '1,20p' "$workdir/xas_sdcc.err"
        return 1
    fi

    if ! "$XOBJCOPY" -I rel -O elf "$sdas_rel" "$sdas_o" \
            >"$workdir/xobjcopy_sdas.out" 2>"$workdir/xobjcopy_sdas.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xobjcopy sdas rel->elf]"
        sed -n '1,20p' "$workdir/xobjcopy_sdas.err"
        return 1
    fi
    if ! "$XOBJCOPY" -I rel -O elf "$xas_sdcc_rel" "$xas_sdcc_o" \
            >"$workdir/xobjcopy_xas_sdcc.out" 2>"$workdir/xobjcopy_xas_sdcc.err"; then
        echo "${RED}FAIL${RESET} $rel_path [xobjcopy xas rel->elf]"
        sed -n '1,20p' "$workdir/xobjcopy_xas_sdcc.err"
        return 1
    fi

    canonicalize_elf "$gnu_as_o" "$gnu_as_canon"
    canonicalize_elf "$xas_gnu_o" "$xas_gnu_canon"
    canonicalize_elf "$sdas_o" "$sdas_canon"
    canonicalize_elf "$xas_sdcc_o" "$xas_sdcc_canon"

    if ! cmp -s "$gnu_as_canon" "$xas_gnu_canon"; then
        echo "${RED}FAIL${RESET} $rel_path [GNU as vs xas GNU]"
        diff -u "$gnu_as_canon" "$xas_gnu_canon" | sed -n '1,80p'
        return 1
    fi
    if ! cmp -s "$sdas_canon" "$xas_sdcc_canon"; then
        echo "${RED}FAIL${RESET} $rel_path [sdasz80 vs xas SDCC]"
        diff -u "$sdas_canon" "$xas_sdcc_canon" | sed -n '1,80p'
        return 1
    fi
    if ! cmp -s "$gnu_as_canon" "$sdas_canon"; then
        echo "${RED}FAIL${RESET} $rel_path [GNU as vs sdasz80]"
        diff -u "$gnu_as_canon" "$sdas_canon" | sed -n '1,80p'
        return 1
    fi

    echo "${GREEN}PASS${RESET} $rel_path"
    return 0
}

mapfile -d '' FILES < <(find "$SRCROOT" -name '*.s' -print0 | sort -z)

pass=0
fail=0
tested=0

for src in "${FILES[@]}"; do
    if [[ "$LIMIT" != "0" && "$tested" -ge "$LIMIT" ]]; then
        break
    fi
    tested=$((tested + 1))
    if compare_one "$src"; then
        pass=$((pass + 1))
    else
        fail=$((fail + 1))
    fi
done

echo
echo "Results: ${GREEN}${pass} passed${RESET}  ${RED}${fail} failed${RESET}  (${tested} tested)"
[[ $fail -eq 0 ]]
