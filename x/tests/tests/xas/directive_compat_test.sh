#!/usr/bin/env bash
#
# directive_compat_test.sh
#
# Verify common assembler directive aliases that xas accepts for source
# compatibility with GNU as, ASxxxx/sdasz80, and generated compiler output.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
XAS="${XAS:-$REPO_ROOT/bin/x/bin/xas}"
XLD="${XLD:-$REPO_ROOT/bin/x/bin/xld}"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
RESET=$'\033[0m'

fail() {
    echo "${RED}FAIL${RESET}: $1" >&2
    exit 1
}

need_line() {
    local pattern="$1"
    local file="$2"
    if ! grep -Fqx "$pattern" "$file"; then
        echo "expected line: $pattern" >&2
        echo "--- $file ---" >&2
        cat "$file" >&2
        fail "missing expected formatted line"
    fi
}

tmpdir="$(mktemp -d /tmp/xas_directives_XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/compat.s" <<'EOF'
            .globl _start
            .globl after_even
            .globl after_balign
            .globl after_p2align
            .area _CODE
            .title "ignored title"
            .list
_start:
            .short 0x1234
            .hword 0x5678
            .4byte 0x9abcdef0
            .dword 0x01020304
            .int 0x05060708
            .skip 2,0xee
            .zero 1
            .even
after_even:
            .byte 0xaa
            .balign 4,0xcc
after_balign:
            .byte 0xbb
            .p2align 2,0xdd
after_p2align:
            ret
            .end
EOF

"$XAS" --mode=sdcc -o "$tmpdir/compat.rel" "$tmpdir/compat.s" \
    || fail "compatibility directives were rejected"
"$XLD" -nostdlib -e _start --oformat=ihx \
       --section-start=_CODE=0x9000 \
       "$tmpdir/compat.rel" -o "$tmpdir/compat.ihx"
grep -qi "34127856f0debc9a0403020108070605" "$tmpdir/compat.ihx" \
    || fail "data-size directive aliases emitted unexpected bytes"
grep -qi "eeee0000aaccccccbbddddddc9" "$tmpdir/compat.ihx" \
    || fail "compatibility directives emitted unexpected bytes"
grep -Fq "S after_even Def00000014" "$tmpdir/compat.rel" \
    || fail ".even did not align the following label to offset 0x14"
grep -Fq "S after_balign Def00000018" "$tmpdir/compat.rel" \
    || fail ".balign did not align the following label to offset 0x18"
grep -Fq "S after_p2align Def0000001C" "$tmpdir/compat.rel" \
    || fail ".p2align did not align the following label to offset 0x1c"

cat > "$tmpdir/extern.s" <<'EOF'
            .extern _callee
            .area _CODE
_start:
            call _callee
            ret
EOF
"$XAS" --mode=sdcc -o "$tmpdir/extern.rel" "$tmpdir/extern.s" \
    || fail ".extern was rejected"
grep -Fq "S _callee Ref00000000" "$tmpdir/extern.rel" \
    || fail ".extern did not declare an external symbol"

cat > "$tmpdir/format.gnu" <<'EOF'
            .section .text
            .extern target
            .short 1
            .skip 2
            .balign 4
            .ident "ignored"
EOF
"$XAS" --mode=gnu --format=sdcc -o "$tmpdir/format.sdcc" "$tmpdir/format.gnu" \
    || fail "format conversion rejected compatibility directives"
need_line ".globl target" "$tmpdir/format.sdcc"
need_line ".dw 1" "$tmpdir/format.sdcc"
need_line ".ds 2" "$tmpdir/format.sdcc"
need_line ".align 4" "$tmpdir/format.sdcc"
need_line ".ident ignored" "$tmpdir/format.sdcc"

cat > "$tmpdir/metadata.s" <<'EOF'
            .section .text
            .global func
            .type func,@function
func:
            nop
            ret
            .size func, . - func
            .global var
            .type var,@object
var:
            .byte 1
            .size var, 1
EOF
"$XAS" --mode=gnu -o "$tmpdir/metadata.o" "$tmpdir/metadata.s" \
    || fail ".type/.size metadata source was rejected"
if command -v readelf >/dev/null 2>&1; then
    readelf -s "$tmpdir/metadata.o" > "$tmpdir/metadata.sym"
    grep -Eq '[[:space:]]2[[:space:]]+FUNC[[:space:]]+GLOBAL.*[[:space:]]func$' \
        "$tmpdir/metadata.sym" \
        || fail ".type/.size did not emit func as a 2-byte FUNC symbol"
    grep -Eq '[[:space:]]1[[:space:]]+OBJECT[[:space:]]+GLOBAL.*[[:space:]]var$' \
        "$tmpdir/metadata.sym" \
        || fail ".type/.size did not emit var as a 1-byte OBJECT symbol"
fi

# Explicit dotted names and shorthand must address the same ELF section.
# Bare names and differently cased names remain distinct sections.
cat > "$tmpdir/sections.s" <<'EOF'
            .text
            .byte 0x11
            .section .text
            .byte 0x22
            .area .text
            .byte 0x33
            .rodata
            .byte 0x41
            .section .rodata
            .byte 0x42
            .area .rodata
            .byte 0x43
            .DATA
            .byte 0x51
            .SECTION .data
            .byte 0x52
            .AREA .data
            .byte 0x53
            .section rodata
            .byte 0x61
            .area rodata
            .byte 0x62
            .section .MiXeD.Name
            .byte 0x71
            .area .MiXeD.Name
            .byte 0x72
            .section MiXeD.Name
            .byte 0x81
            .area MiXeD.Name
            .byte 0x82
            .section .RoData
            .byte 0x91
            .area .RoData
            .byte 0x92
            .bss
            .ds 3
            .section .bss
            .ds 2
            .area .bss
            .ds 1
EOF
"$XAS" --mode=gnu -o "$tmpdir/sections.o" "$tmpdir/sections.s" \
    || fail "explicit ELF section names were rejected"
python3 - "$tmpdir/sections.o" <<'PY'
from pathlib import Path
import struct
import sys

data = Path(sys.argv[1]).read_bytes()
assert data[:6] == b"\x7fELF\x01\x01", "expected little-endian ELF32"
header = struct.unpack_from("<16sHHIIIIIHHHHHH", data)
offset, entry_size, count, names_index = header[6], *header[11:14]
headers = [struct.unpack_from("<10I", data, offset + i * entry_size)
           for i in range(count)]
names_header = headers[names_index]
names = data[names_header[4]:names_header[4] + names_header[5]]
sections = {}
for section in headers:
    name = names[section[0]:].split(b"\0", 1)[0].decode("ascii")
    sections[name] = section

expected = {".text": "112233", ".rodata": "414243", ".data": "515253",
            "rodata": "6162", ".MiXeD.Name": "7172",
            "MiXeD.Name": "8182", ".RoData": "9192"}
for name, content in expected.items():
    assert name in sections, f"missing ELF section {name!r}: {list(sections)}"
    section = sections[name]
    actual = data[section[4]:section[4] + section[5]]
    assert actual == bytes.fromhex(content), (name, actual.hex(), content)
assert not {"text", "data", "bss", ".mixed.name"} & sections.keys()
assert ".bss" in sections, "missing ELF .bss"
assert sections[".bss"][1] == 8, ".bss must remain SHT_NOBITS"
assert sections[".bss"][5] == 6, "explicit .bss must extend shorthand .bss"
PY

echo "${GREEN}PASS${RESET}: xas directive compatibility"
