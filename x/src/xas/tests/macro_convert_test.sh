#!/usr/bin/env bash
# macro_convert_test.sh — tests for macro-aware dialect conversion (--format).
#
# For each case we convert a macro source from one dialect to the other, then:
#   * assert whether the macro survived as a native target-dialect macro
#     (translatable) or was expanded away (incompatible) — proving the
#     conversion is operational, not a no-op;
#   * assemble the converted output and a hand-written expanded reference in
#     the target dialect and require byte-identical objects — proving the
#     converted code actually compiles and is semantically equivalent.
#
# MIT License (see: LICENSE)
# copyright (C) 2026 tomaz stih
set -u

X_ROOT=$(realpath "$(dirname "$0")/../../..")
REPO_ROOT=$(realpath "$X_ROOT/..")
XAS="$REPO_ROOT/bin/x/bin/xas"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0

# convert_case <name> <src_mode> <dst_format> <dst_mode> <ext> \
#              <keep|expand> <src> <expected-expanded-in-target-dialect>
convert_case() {
    local name="$1" smode="$2" dfmt="$3" dmode="$4" ext="$5" kind="$6"
    local src="$7" exp="$8"
    local d="$TMP/$name"; mkdir -p "$d/a" "$d/b"
    printf '%s' "$src" > "$d/src.s"

    if ! "$XAS" --mode="$smode" --format="$dfmt" "$d/src.s" -o "$d/a/t.s" \
            2> "$d/conv.err"; then
        echo "FAIL: $name (conversion failed)"; cat "$d/conv.err"
        fail=$((fail+1)); return
    fi

    # op / no-op assertion on the converted text: a translated construct keeps a
    # native macro/repeat directive; an expanded one has none.
    if grep -qE '\.macro|\.rept|\.irp' "$d/a/t.s"; then haz=keep; else haz=expand; fi
    if [ "$haz" != "$kind" ]; then
        echo "FAIL: $name (expected $kind, converted output was $haz)"
        echo "----- converted -----"; cat "$d/a/t.s"
        fail=$((fail+1)); return
    fi

    printf '%s' "$exp" > "$d/b/t.s"
    if ! "$XAS" --mode="$dmode" "$d/a/t.s" -o "$d/a/t.$ext" 2> "$d/a.err"; then
        echo "FAIL: $name (converted output does not assemble)"
        echo "----- converted -----"; cat "$d/a/t.s"; cat "$d/a.err"
        fail=$((fail+1)); return
    fi
    if ! "$XAS" --mode="$dmode" "$d/b/t.s" -o "$d/b/t.$ext" 2> "$d/b.err"; then
        echo "FAIL: $name (reference does not assemble)"; cat "$d/b.err"
        fail=$((fail+1)); return
    fi
    if cmp -s "$d/a/t.$ext" "$d/b/t.$ext"; then
        echo "PASS: $name ($kind)"; pass=$((pass+1))
    else
        echo "FAIL: $name (converted bytes != expanded reference)"
        echo "----- converted -----"; cat "$d/a/t.s"
        fail=$((fail+1))
    fi
}

# ===========================================================================
# Translatable: macro must survive into the target dialect.
# ===========================================================================

# sdas -> gnu, plain macro
convert_case sdas2gnu-macro sdcc gnu gnu o keep \
"$(printf '        .area _CODE\n        .macro PUSH2 r1, r2\n        push r1\n        push r2\n        .endm\n        PUSH2 bc, de\n        PUSH2 hl, ix\n')" \
"$(printf '        .text\n        push bc\n        push de\n        push hl\n        push ix\n')"

# gnu -> sdas, plain macro
convert_case gnu2sdas-macro gnu sdcc sdcc rel keep \
"$(printf '        .section .text\n        .macro PUSH2 r1 r2\n        push \\r1\n        push \\r2\n        .endm\n        PUSH2 bc, de\n        PUSH2 hl, ix\n')" \
"$(printf '        .area text\n        push bc\n        push de\n        push hl\n        push ix\n')"

# sdas -> gnu, .rept (terminator .endm -> .endr)
convert_case sdas2gnu-rept sdcc gnu gnu o keep \
"$(printf '        .area _CODE\n        .rept 3\n        nop\n        .endm\n')" \
"$(printf '        .text\n        nop\n        nop\n        nop\n')"

# gnu -> sdas, .irp (terminator .endr -> .endm; \\reg -> reg)
convert_case gnu2sdas-irp gnu sdcc sdcc rel keep \
"$(printf '        .section .text\n        .irp reg, bc, de, hl\n        push \\reg\n        .endr\n')" \
"$(printf '        .area text\n        push bc\n        push de\n        push hl\n')"

# gnu -> sdas, keyword + default args lowered to positional at the call site
convert_case gnu2sdas-defaults gnu sdcc sdcc rel keep \
"$(printf '        .section .text\n        .macro mov2 dst=a, src=b\n        ld \\dst, \\src\n        .endm\n        mov2\n        mov2 c\n        mov2 src=e\n')" \
"$(printf '        .area text\n        ld a, b\n        ld c, b\n        ld a, e\n')"

# sdas -> gnu, data directive spelling in the body (.dw -> .word)
convert_case sdas2gnu-data sdcc gnu gnu o keep \
"$(printf '        .area _CODE\n        .macro EMIT v\n        .dw v\n        .endm\n        EMIT 0x1234\n        EMIT 0x5678\n')" \
"$(printf '        .text\n        .word 0x1234\n        .word 0x5678\n')"

# ===========================================================================
# Incompatible: macro must be expanded so the converted output still compiles.
# ===========================================================================

# gnu -> sdas, body uses \\@ (no sdas equivalent) -> expand
convert_case gnu2sdas-at gnu sdcc sdcc rel expand \
"$(printf '        .section .text\n        .macro skip\n        jr .L\\@\n        nop\n.L\\@:\n        .endm\n        skip\n        skip\n')" \
"$(printf '        .area text\n        jr .L1\n        nop\n.L1:\n        jr .L2\n        nop\n.L2:\n')"

# sdas -> gnu, body uses ' concatenation (no gas equivalent) -> expand
convert_case sdas2gnu-concat sdcc gnu gnu o expand \
"$(printf '        .area _CODE\n        .macro LBL a, b\na'"'"'b:\n        nop\n        .endm\n        LBL foo, 1\n        jr foo1\n')" \
"$(printf '        .text\nfoo1:\n        nop\n        jr foo1\n')"

# gnu -> sdas, macro with a nested .rept in its body -> expand (terminator clash)
convert_case gnu2sdas-nested gnu sdcc sdcc rel expand \
"$(printf '        .section .text\n        .macro fill n\n        .rept \\n\n        nop\n        .endr\n        .endm\n        fill 3\n')" \
"$(printf '        .area text\n        nop\n        nop\n        nop\n')"

echo "------------------------------------------"
echo "macro conversion tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
