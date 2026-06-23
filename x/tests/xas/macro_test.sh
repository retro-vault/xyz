#!/usr/bin/env bash
# macro_test.sh — end-to-end tests for the xas macro preprocessor.
#
# Strategy: assemble a macro-using source and a hand-expanded equivalent and
# require the assembler output to be byte-identical.  Both inputs use the same
# basename so the recorded module name matches.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
XAS="${XAS:-$REPO_ROOT/bin/x/bin/xas}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0

# compare_obj <mode> <ext> <macro-src> <expected-src>
compare_obj() {
    local name="$1" mode="$2" ext="$3" macro="$4" expected="$5"
    local da="$TMP/$name/macro" db="$TMP/$name/expected"
    mkdir -p "$da" "$db"
    printf '%s' "$macro"    > "$da/t.s"
    printf '%s' "$expected" > "$db/t.s"

    if ! "$XAS" --mode="$mode" "$da/t.s" -o "$da/t.$ext" 2> "$da/err"; then
        echo "FAIL: $name (macro source failed to assemble)"; cat "$da/err"
        fail=$((fail + 1)); return
    fi
    if ! "$XAS" --mode="$mode" "$db/t.s" -o "$db/t.$ext" 2> "$db/err"; then
        echo "FAIL: $name (expected source failed to assemble)"; cat "$db/err"
        fail=$((fail + 1)); return
    fi
    if cmp -s "$da/t.$ext" "$db/t.$ext"; then
        echo "PASS: $name"; pass=$((pass + 1))
    else
        echo "FAIL: $name (output differs from hand-expanded reference)"
        fail=$((fail + 1))
    fi
}

# expect_error <name> <mode> <src>  — assembly must fail.
expect_error() {
    local name="$1" mode="$2" src="$3"
    local d="$TMP/$name"; mkdir -p "$d"
    printf '%s' "$src" > "$d/t.s"
    if "$XAS" --mode="$mode" "$d/t.s" -o "$d/t.out" 2> "$d/err"; then
        echo "FAIL: $name (expected an error but assembly succeeded)"
        fail=$((fail + 1))
    else
        echo "PASS: $name"; pass=$((pass + 1))
    fi
}

# ---------------------------------------------------------------------------
# SDCC / ASxxxx dialect
# ---------------------------------------------------------------------------

read -r -d '' SDAS_MACRO <<'EOF'
        .module t
        .area _CODE
        .macro PUSH2 r1, r2
        push r1
        push r2
        .endm
        PUSH2 bc, de
        .rept 2
        nop
        .endm
        .irp reg, bc, de, hl
        push reg
        .endm
        .irpc ch, abc
        nop
        .endm
EOF
read -r -d '' SDAS_EXP <<'EOF'
        .module t
        .area _CODE
        push bc
        push de
        nop
        nop
        push bc
        push de
        push hl
        nop
        nop
        nop
EOF
compare_obj sdas-basic sdcc rel "$SDAS_MACRO" "$SDAS_EXP"

# Auto-locals (?arg): two calls must produce two distinct labels (no 'duplicate
# symbol' error), and the byte stream must match explicit unique labels.
read -r -d '' SDAS_LOCAL <<'EOF'
        .module t
        .area _CODE
        .macro SKIP ?lab
        jr lab
        nop
lab:
        .endm
        SKIP
        SKIP
EOF
read -r -d '' SDAS_LOCAL_EXP <<'EOF'
        .module t
        .area _CODE
        jr l1
        nop
l1:
        jr l2
        nop
l2:
EOF
compare_obj sdas-autolocal sdcc rel "$SDAS_LOCAL" "$SDAS_LOCAL_EXP"

# .narg attribute directive.
read -r -d '' SDAS_NARG <<'EOF'
        .module t
        .area _CODE
        .macro CNT a, b, c
        .narg n
        .byte n
        .endm
        CNT x, y
        CNT x, , z
EOF
read -r -d '' SDAS_NARG_EXP <<'EOF'
        .module t
        .area _CODE
        .byte 2
        .byte 3
EOF
compare_obj sdas-narg sdcc rel "$SDAS_NARG" "$SDAS_NARG_EXP"

# Argument concatenation with the single quote.
read -r -d '' SDAS_CAT <<'EOF'
        .module t
        .area _CODE
        .macro LBL a, b
a'b:
        nop
        .endm
        LBL foo, 1
        jr foo1
EOF
read -r -d '' SDAS_CAT_EXP <<'EOF'
        .module t
        .area _CODE
foo1:
        nop
        jr foo1
EOF
compare_obj sdas-concat sdcc rel "$SDAS_CAT" "$SDAS_CAT_EXP"

# Unterminated macro is an error.
expect_error sdas-unterminated sdcc "$(printf '        .area _CODE\n        .macro M\n        nop\n')"

# ---------------------------------------------------------------------------
# GNU as dialect
# ---------------------------------------------------------------------------

read -r -d '' GNU_MACRO <<'EOF'
        .section .text
        .macro PUSH2 r1 r2
        push \r1
        push \r2
        .endm
        PUSH2 bc, de
        .rept 2
        nop
        .endr
        .irp reg, bc, de, hl
        push \reg
        .endr
        .irpc c, abc
        nop
        .endr
EOF
read -r -d '' GNU_EXP <<'EOF'
        .section .text
        push bc
        push de
        nop
        nop
        push bc
        push de
        push hl
        nop
        nop
        nop
EOF
compare_obj gnu-basic gnu o "$GNU_MACRO" "$GNU_EXP"

# Default values + keyword args + nested .rept inside a macro.
read -r -d '' GNU_KW <<'EOF'
        .section .text
        .macro fill val=0xff, n=2
        .rept \n
        .byte \val
        .endr
        .endm
        fill
        fill 0xaa
        fill n=3
EOF
read -r -d '' GNU_KW_EXP <<'EOF'
        .section .text
        .byte 0xff
        .byte 0xff
        .byte 0xaa
        .byte 0xaa
        .byte 0xff
        .byte 0xff
        .byte 0xff
EOF
compare_obj gnu-keyword gnu o "$GNU_KW" "$GNU_KW_EXP"

# \@ unique macro-instance counter used to build labels.
read -r -d '' GNU_AT <<'EOF'
        .section .text
        .macro skip
        jr .Ls\@
        nop
.Ls\@:
        .endm
        skip
        skip
EOF
read -r -d '' GNU_AT_EXP <<'EOF'
        .section .text
        jr .Ls1
        nop
.Ls1:
        jr .Ls2
        nop
.Ls2:
EOF
compare_obj gnu-at gnu o "$GNU_AT" "$GNU_AT_EXP"

# A macro that calls another macro (nested expansion).
compare_obj gnu-nested gnu o \
"$(printf '        .section .text\n        .macro inner r\n        push \\r\n        .endm\n        .macro outer a, b\n        inner \\a\n        inner \\b\n        .endm\n        outer bc, de\n')" \
"$(printf '        .section .text\n        push bc\n        push de\n')"

# .exitm stops expansion early.
compare_obj gnu-exitm gnu o \
"$(printf '        .section .text\n        .macro m\n        nop\n        .exitm\n        halt\n        .endm\n        m\n')" \
"$(printf '        .section .text\n        nop\n')"

# :vararg parameter soaks up the remaining call arguments.
compare_obj gnu-vararg gnu o \
"$(printf '        .section .text\n        .macro bytes first, rest:vararg\n        .byte \\first\n        .byte \\rest\n        .endm\n        bytes 1, 2, 3\n')" \
"$(printf '        .section .text\n        .byte 1\n        .byte 2,3\n')"

# \\() separator concatenates a parameter with adjacent text to build a label.
compare_obj gnu-sep gnu o \
"$(printf '        .section .text\n        .macro defb name, val\n\\name\\(): .byte \\val\n        .endm\n        defb foo, 7\n        jr foo\n')" \
"$(printf '        .section .text\nfoo:\n        .byte 7\n        jr foo\n')"

# Double-quoted call argument containing a comma is one argument.
compare_obj gnu-quoted gnu o \
"$(printf '        .section .text\n        .macro one a\n        \\a\n        .endm\n        one \"ld a, b\"\n')" \
"$(printf '        .section .text\n        ld a, b\n')"

# Macro with no parameters, multiple calls.
compare_obj gnu-noargs gnu o \
"$(printf '        .section .text\n        .macro nl\n        nop\n        .endm\n        nl\n        nl\n        nl\n')" \
"$(printf '        .section .text\n        nop\n        nop\n        nop\n')"

# ---------------------------------------------------------------------------
# More SDCC / ASxxxx coverage
# ---------------------------------------------------------------------------

# .mexit stops expansion early.
compare_obj sdas-mexit sdcc rel \
"$(printf '        .area _CODE\n        .macro m\n        nop\n        .mexit\n        halt\n        .endm\n        m\n')" \
"$(printf '        .area _CODE\n        nop\n')"

# Macro that calls another macro (nested expansion).
compare_obj sdas-nested sdcc rel \
"$(printf '        .area _CODE\n        .macro inner r\n        push r\n        .endm\n        .macro outer a, b\n        inner a\n        inner b\n        .endm\n        outer bc, de\n')" \
"$(printf '        .area _CODE\n        push bc\n        push de\n')"

# \\arg inserts the argument's numeric value in the current radix.
compare_obj sdas-numeric sdcc rel \
"$(printf '        .area _CODE\n        .macro byt v\n        .byte \\v\n        .endm\n        byt 0x10\n')" \
"$(printf '        .area _CODE\n        .byte 16\n')"

# .nval equates a symbol to an expression value.
compare_obj sdas-nval sdcc rel \
"$(printf '        .area _CODE\n        .macro v e\n        .nval x, e\n        .byte x\n        .endm\n        v 5+2\n')" \
"$(printf '        .area _CODE\n        .byte 7\n')"

# .nchr counts characters of a (delimited) string argument.
compare_obj sdas-nchr sdcc rel \
"$(printf '        .area _CODE\n        .macro s str\n        .nchr c, str\n        .byte c\n        .endm\n        s ^/hello/\n')" \
"$(printf '        .area _CODE\n        .byte 5\n')"

# ^/ / delimits a call argument that itself contains separators.
compare_obj sdas-caret sdcc rel \
"$(printf '        .area _CODE\n        .macro emit a\n        a\n        .endm\n        emit ^/ld a, b/\n')" \
"$(printf '        .area _CODE\n        ld a, b\n')"

# .irp with a single value; .irpc over a string of digits.
compare_obj sdas-irp-one sdcc rel \
"$(printf '        .area _CODE\n        .irp r, hl\n        push r\n        .endm\n')" \
"$(printf '        .area _CODE\n        push hl\n')"

echo "------------------------------------------"
echo "macro tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
