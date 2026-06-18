#!/usr/bin/env bash
set -euo pipefail

XOPT="${1:-../../../bin/x/bin/xopt}"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

cat >"$TMPDIR/in.s" <<'ASM'
	.area	_CODE
_demo:
	ld	a, #0
	ld	a, #0
	push	hl
	pop	hl
	ret
ASM

"$XOPT" -O2 "$TMPDIR/in.s" -o "$TMPDIR/out.s"

if grep -q 'push	hl' "$TMPDIR/out.s"; then
    echo "xopt smoke: push/pop was not optimized" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti.s" <<'ASM'
_demo:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	xor	b
	ld	e,a
	ret
_other:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	xor	b
	ld	e,a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti.s" -o "$TMPDIR/spaghetti.out.s"
grep -q '__xopt_spaghetti_' "$TMPDIR/spaghetti.out.s"
grep -q '__xopt_spaghetti_0:' "$TMPDIR/spaghetti.out.s"
if [[ "$(grep -c 'ld	a,#1' "$TMPDIR/spaghetti.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti did not extract exactly one duplicate kernel" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_no_branch.s" <<'ASM'
_demo:
	ld	a,#1
	jr	z,_skip
	ld	b,#2
_skip:
	ret
_other:
	ld	a,#1
	jr	z,_skip2
	ld	b,#2
_skip2:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_no_branch.s" -o "$TMPDIR/spaghetti_no_branch.out.s"
if grep -q '__xopt_spaghetti_' "$TMPDIR/spaghetti_no_branch.out.s"; then
    echo "xopt smoke: Spaghetti outlined a branch-containing kernel" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_tail.s" <<'ASM'
_demo:
	call	__xopt_spaghetti_0
	jr	_done
_other:
	call	__xopt_spaghetti_0
	jr	_done
_done:
	ret
__xopt_spaghetti_0:
	ld	a,#1
	ld	b,#2
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_tail.s" -o "$TMPDIR/spaghetti_tail.out.s"
grep -Eq '^[[:space:]]+j[pr][[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail.out.s"
grep -Eq '^[[:space:]]+j[pr][[:space:]]+_done' "$TMPDIR/spaghetti_tail.out.s"
if grep -q 'call	__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail.out.s"; then
    echo "xopt smoke: Spaghetti tail threading left helper calls behind" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_tail_mixed.s" <<'ASM'
_demo:
	call	__xopt_spaghetti_0
	jr	_done
_other:
	call	__xopt_spaghetti_0
	jr	_elsewhere
_done:
	ret
_elsewhere:
	ret
__xopt_spaghetti_0:
	ld	a,#1
	ld	b,#2
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_tail_mixed.s" -o "$TMPDIR/spaghetti_tail_mixed.out.s"
grep -q 'call	__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail_mixed.out.s"
if grep -Eq '^[[:space:]]+j[pr][[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail_mixed.out.s"; then
    echo "xopt smoke: Spaghetti tail threading crossed mixed continuations" >&2
    exit 1
fi

cat >"$TMPDIR/conditional_ret.s" <<'ASM'
_demo:
	call	_maybe
	ret	c
	ld	a,#1
	ret
ASM

"$XOPT" -O3 "$TMPDIR/conditional_ret.s" -o "$TMPDIR/conditional_ret.out.s"
if ! grep -q 'ret	c' "$TMPDIR/conditional_ret.out.s"; then
    echo "xopt smoke: conditional ret after call must be preserved" >&2
    exit 1
fi

cat >"$TMPDIR/bc_sbc.s" <<'ASM'
_demo:
	ld	d,b
	ld	e,c
	or	a,a
	sbc	hl,de
	ld	e,#0
	ld	d,#0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/bc_sbc.s" -o "$TMPDIR/bc_sbc.out.s"
grep -q 'sbc	hl, bc' "$TMPDIR/bc_sbc.out.s"
if grep -q 'ld	d, b' "$TMPDIR/bc_sbc.out.s"; then
    echo "xopt smoke: BC->DE sbc forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/de_store_forward.s" <<'ASM'
_demo:
	ld	h,d
	ld	l,e
	ld	b,h
	ld	c,l
	ld	-3(ix),l
	ld	-2(ix),h
	ld	l,#0
	ld	h,#0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/de_store_forward.s" -o "$TMPDIR/de_store_forward.out.s"
grep -q 'ld	b, d' "$TMPDIR/de_store_forward.out.s"
grep -q 'ld	-3(ix), e' "$TMPDIR/de_store_forward.out.s"
if grep -q 'ld	h, d' "$TMPDIR/de_store_forward.out.s"; then
    echo "xopt smoke: dead HL DE-store forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/de_pop_dead.s" <<'ASM'
_demo:
	ld	h,d
	ld	l,e
	pop	de
	ret
ASM

"$XOPT" -O3 "$TMPDIR/de_pop_dead.s" -o "$TMPDIR/de_pop_dead.out.s"
grep -q 'ex	de, hl' "$TMPDIR/de_pop_dead.out.s"
if grep -q 'ld	h, d' "$TMPDIR/de_pop_dead.out.s"; then
    echo "xopt smoke: DE->HL dead-DE pop forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/xor_forward.s" <<'ASM'
_demo:
	ld	l,c
	ld	a,e
	xor	l
	ld	l,a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/xor_forward.s" -o "$TMPDIR/xor_forward.out.s"
grep -q 'xor	c' "$TMPDIR/xor_forward.out.s"
if grep -q 'ld	l, c' "$TMPDIR/xor_forward.out.s"; then
    echo "xopt smoke: low-byte xor forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/dead_bc_xor_hl.s" <<'ASM'
_demo:
	ld	a,c
	xor	l
	ld	c,a
	ld	a,b
	xor	h
	ld	b,a
	ld	l,c
	ld	h,b
	ld	c,#1
	ld	b,#2
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_bc_xor_hl.s" -o "$TMPDIR/dead_bc_xor_hl.out.s"
grep -q 'xor	c' "$TMPDIR/dead_bc_xor_hl.out.s"
grep -q 'xor	b' "$TMPDIR/dead_bc_xor_hl.out.s"
if grep -q 'ld	c, a' "$TMPDIR/dead_bc_xor_hl.out.s"; then
    echo "xopt smoke: dead-BC XOR forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/dead_c_xor_hl.s" <<'ASM'
_demo:
	ld	a,c
	xor	l
	ld	c,a
	ld	a,b
	xor	h
	ld	b,a
	ld	l,c
	ld	h,b
	add	hl,hl
	ld	a,b
	ld	c,#1
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_c_xor_hl.s" -o "$TMPDIR/dead_c_xor_hl.out.s"
grep -q 'ld	l, a' "$TMPDIR/dead_c_xor_hl.out.s"
grep -q 'ld	h, a' "$TMPDIR/dead_c_xor_hl.out.s"
grep -q 'ld	a, b' "$TMPDIR/dead_c_xor_hl.out.s"
if grep -q 'ld	c, a' "$TMPDIR/dead_c_xor_hl.out.s"; then
    echo "xopt smoke: dead-C XOR forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/pair_imm_fold.s" <<'ASM'
_demo:
	ld	b,#16
	ld	a,#1
	inc	a
	ld	c,#32
	ld	e,#52
	inc	e
	ld	d,#18
	ld	h,#86
	ld	a,#1
	ld	l,#120
	ret
ASM

"$XOPT" -O3 "$TMPDIR/pair_imm_fold.s" -o "$TMPDIR/pair_imm_fold.out.s"
grep -q 'ld	bc, #4128' "$TMPDIR/pair_imm_fold.out.s"
grep -q 'ld	de, #4660' "$TMPDIR/pair_imm_fold.out.s"
grep -q 'ld	hl, #22136' "$TMPDIR/pair_imm_fold.out.s"
if grep -q 'ld	c,#32' "$TMPDIR/pair_imm_fold.out.s"; then
    echo "xopt smoke: separated BC immediate fold did not fire" >&2
    exit 1
fi
if grep -q 'ld	d,#18' "$TMPDIR/pair_imm_fold.out.s"; then
    echo "xopt smoke: separated DE immediate fold did not fire" >&2
    exit 1
fi
if grep -q 'ld	l,#120' "$TMPDIR/pair_imm_fold.out.s"; then
    echo "xopt smoke: separated HL immediate fold did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/pair_imm_no_fold.s" <<'ASM'
_demo:
	ld	b,#16
	ld	a,c
	ld	c,#32
	ret
ASM

"$XOPT" -O3 "$TMPDIR/pair_imm_no_fold.s" -o "$TMPDIR/pair_imm_no_fold.out.s"
grep -q 'ld	b,#16' "$TMPDIR/pair_imm_no_fold.out.s"
grep -q 'ld	c,#32' "$TMPDIR/pair_imm_no_fold.out.s"
if grep -q 'ld	bc, #4128' "$TMPDIR/pair_imm_no_fold.out.s"; then
    echo "xopt smoke: separated pair fold crossed a C read" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_inc_direct.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	inc	hl
	ld	-4(ix),l
	ld	-3(ix),h
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_word_inc_direct.s" -o "$TMPDIR/ix_word_inc_direct.out.s"
grep -q 'inc	-4(ix)' "$TMPDIR/ix_word_inc_direct.out.s"
grep -q 'jr	nz, __xopt_inc16_' "$TMPDIR/ix_word_inc_direct.out.s"
grep -q 'inc	-3(ix)' "$TMPDIR/ix_word_inc_direct.out.s"
if grep -q 'inc	hl' "$TMPDIR/ix_word_inc_direct.out.s"; then
    echo "xopt smoke: direct IX word increment did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_inc_flags_live.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	inc	hl
	ld	-4(ix),l
	ld	-3(ix),h
	jr	z,_done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_word_inc_flags_live.s" -o "$TMPDIR/ix_word_inc_flags_live.out.s"
grep -q 'inc	hl' "$TMPDIR/ix_word_inc_flags_live.out.s"
if grep -q 'inc	-4(ix)' "$TMPDIR/ix_word_inc_flags_live.out.s"; then
    echo "xopt smoke: direct IX word increment clobbered live flags" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_inc_direct.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	add	a,#1
	ld	-4(ix),a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_inc_direct.s" -o "$TMPDIR/ix_byte_inc_direct.out.s"
grep -q 'inc	-4(ix)' "$TMPDIR/ix_byte_inc_direct.out.s"
if grep -q 'add	a,#1' "$TMPDIR/ix_byte_inc_direct.out.s"; then
    echo "xopt smoke: direct IX byte increment did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_inc_a_live.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	add	a,#1
	ld	-4(ix),a
	ld	b,a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_inc_a_live.s" -o "$TMPDIR/ix_byte_inc_a_live.out.s"
grep -q 'add	a,#1' "$TMPDIR/ix_byte_inc_a_live.out.s"
if grep -q 'inc	-4(ix)' "$TMPDIR/ix_byte_inc_a_live.out.s"; then
    echo "xopt smoke: direct IX byte increment clobbered live A" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_inc_flags_live.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	add	a,#1
	ld	-4(ix),a
	ld	a,#0
	jr	c,_done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_inc_flags_live.s" -o "$TMPDIR/ix_byte_inc_flags_live.out.s"
grep -q 'add	a,#1' "$TMPDIR/ix_byte_inc_flags_live.out.s"
if grep -q 'inc	-4(ix)' "$TMPDIR/ix_byte_inc_flags_live.out.s"; then
    echo "xopt smoke: direct IX byte increment clobbered live flags" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_load_forward.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	ld	c,a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_load_forward.s" -o "$TMPDIR/ix_byte_load_forward.out.s"
grep -q 'ld	c, -4(ix)' "$TMPDIR/ix_byte_load_forward.out.s"
if grep -q 'ld	a,-4(ix)' "$TMPDIR/ix_byte_load_forward.out.s"; then
    echo "xopt smoke: IX byte load forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_load_forward_cross_addhl.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	ld	c,a
	ld	b,#0
	add	hl,bc
	ld	a,#7
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_load_forward_cross_addhl.s" -o "$TMPDIR/ix_byte_load_forward_cross_addhl.out.s"
grep -q 'ld	c, -4(ix)' "$TMPDIR/ix_byte_load_forward_cross_addhl.out.s"
grep -q 'add	hl,bc' "$TMPDIR/ix_byte_load_forward_cross_addhl.out.s"
if grep -q 'ld	a,-4(ix)' "$TMPDIR/ix_byte_load_forward_cross_addhl.out.s"; then
    echo "xopt smoke: IX byte load forwarding stopped at 16-bit add" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_load_forward_a_live.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	ld	c,a
	ld	b,a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_load_forward_a_live.s" -o "$TMPDIR/ix_byte_load_forward_a_live.out.s"
grep -q 'ld	a,-4(ix)' "$TMPDIR/ix_byte_load_forward_a_live.out.s"
if grep -q 'ld	c, -4(ix)' "$TMPDIR/ix_byte_load_forward_a_live.out.s"; then
    echo "xopt smoke: IX byte load forwarding clobbered live A" >&2
    exit 1
fi

cat >"$TMPDIR/hl_byte_load_forward.s" <<'ASM'
_demo:
	ld	a,(hl)
	ld	c,a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/hl_byte_load_forward.s" -o "$TMPDIR/hl_byte_load_forward.out.s"
grep -q 'ld	c, (hl)' "$TMPDIR/hl_byte_load_forward.out.s"
if grep -q 'ld	a,(hl)' "$TMPDIR/hl_byte_load_forward.out.s"; then
    echo "xopt smoke: HL byte load forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/hl_byte_load_forward_a_live.s" <<'ASM'
_demo:
	ld	a,(hl)
	ld	c,a
	ld	b,a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/hl_byte_load_forward_a_live.s" -o "$TMPDIR/hl_byte_load_forward_a_live.out.s"
grep -q 'ld	a,(hl)' "$TMPDIR/hl_byte_load_forward_a_live.out.s"
if grep -q 'ld	c, (hl)' "$TMPDIR/hl_byte_load_forward_a_live.out.s"; then
    echo "xopt smoke: HL byte load forwarding clobbered live A" >&2
    exit 1
fi

cat >"$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.s" <<'ASM'
_demo:
	ld	a,(hl)
	ld	l,a
	ld	h,#0
	ld	b,h
	ld	c,l
	ld	h,b
	ld	l,c
	ld	a,h
	or	l
	jr	z,_done
	ld	hl,#1
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.s" -o "$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.out.s"
grep -q 'ld	a,(hl)' "$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.out.s"
grep -q 'or	a, a' "$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.out.s"
if grep -q 'ld	l, (hl)' "$TMPDIR/hl_byte_load_forward_keeps_zero_extend_truth.out.s"; then
    echo "xopt smoke: HL byte load forwarding blocked zero-extend truth cleanup" >&2
    exit 1
fi

cat >"$TMPDIR/compare_fallthrough_reload.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	cp	#1
	jr	z,_hit
_fall:
	ld	a,-4(ix)
	cp	#2
_hit:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/compare_fallthrough_reload.s" -o "$TMPDIR/compare_fallthrough_reload.out.s"
if [[ "$(grep -c 'ld	a,-4(ix)' "$TMPDIR/compare_fallthrough_reload.out.s")" != "1" ]]; then
    echo "xopt smoke: compare fallthrough reload cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/compare_fallthrough_reload_referenced.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	cp	#1
	jr	z,_hit
_fall:
	ld	a,-4(ix)
	cp	#2
	ret
_hit:
	xor	a
	jp	_fall
ASM

"$XOPT" -O3 "$TMPDIR/compare_fallthrough_reload_referenced.s" -o "$TMPDIR/compare_fallthrough_reload_referenced.out.s"
if [[ "$(grep -c 'ld	a,-4(ix)' "$TMPDIR/compare_fallthrough_reload_referenced.out.s")" != "2" ]]; then
    echo "xopt smoke: compare reload cleanup crossed a referenced label" >&2
    exit 1
fi

cat >"$TMPDIR/xor_compare_fallthrough_reload.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	xor	#128
	cp	#176
	jr	c,_fail
_fall:
	ld	a,-4(ix)
	xor	#128
	cp	#185
	jr	c,_pass
_fail:
	ret
_pass:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/xor_compare_fallthrough_reload.s" -o "$TMPDIR/xor_compare_fallthrough_reload.out.s"
if [[ "$(grep -c 'ld	a,-4(ix)' "$TMPDIR/xor_compare_fallthrough_reload.out.s")" != "1" ]]; then
    echo "xopt smoke: transformed compare reload cleanup did not remove the reload" >&2
    exit 1
fi
if [[ "$(grep -c 'xor	#128' "$TMPDIR/xor_compare_fallthrough_reload.out.s")" != "1" ]]; then
    echo "xopt smoke: transformed compare reload cleanup did not remove the repeat transform" >&2
    exit 1
fi

cat >"$TMPDIR/xor_compare_fallthrough_reload_referenced.s" <<'ASM'
_demo:
	ld	a,-4(ix)
	xor	#128
	cp	#176
	jr	c,_fail
_fall:
	ld	a,-4(ix)
	xor	#128
	cp	#185
	jr	c,_pass
_fail:
	ret
_pass:
	xor	a
	jp	_fall
ASM

"$XOPT" -O3 "$TMPDIR/xor_compare_fallthrough_reload_referenced.s" -o "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s"
if [[ "$(grep -c 'ld	a,-4(ix)' "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s")" != "2" ]]; then
    echo "xopt smoke: transformed compare reload cleanup crossed a referenced label" >&2
    exit 1
fi
if [[ "$(grep -c 'xor	#128' "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s")" != "2" ]]; then
    echo "xopt smoke: transformed compare reload cleanup crossed a referenced label transform" >&2
    exit 1
fi

cat >"$TMPDIR/zero_extend_src_to_bc.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,#0
	ld	b,h
	ld	c,l
	ld	hl,#1234
	ret
ASM

"$XOPT" -O3 "$TMPDIR/zero_extend_src_to_bc.s" -o "$TMPDIR/zero_extend_src_to_bc.out.s"
grep -q 'ld	c, -4(ix)' "$TMPDIR/zero_extend_src_to_bc.out.s"
grep -q 'ld	b, #0' "$TMPDIR/zero_extend_src_to_bc.out.s"
if grep -q 'ld	l,-4(ix)' "$TMPDIR/zero_extend_src_to_bc.out.s"; then
    echo "xopt smoke: generalized zero-extend to BC did not remove L load" >&2
    exit 1
fi
if grep -q 'ld	h,#0' "$TMPDIR/zero_extend_src_to_bc.out.s"; then
    echo "xopt smoke: generalized zero-extend to BC did not remove H zero" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/zero_extend_src_to_bc.s" -o "$TMPDIR/zero_extend_src_to_bc.os.s"
grep -q 'ld	l,-4(ix)' "$TMPDIR/zero_extend_src_to_bc.os.s"
grep -q 'ld	h,#0' "$TMPDIR/zero_extend_src_to_bc.os.s"

cat >"$TMPDIR/zero_extend_pair_test.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,#0
	ld	b,h
	ld	c,l
	ld	h,b
	ld	l,c
	ld	a,h
	or	a,l
	jr	z,_done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/zero_extend_pair_test.s" -o "$TMPDIR/zero_extend_pair_test.out.s"
grep -q 'ld	a, l' "$TMPDIR/zero_extend_pair_test.out.s"
grep -q 'or	a, a' "$TMPDIR/zero_extend_pair_test.out.s"
if grep -q 'ld	h,b' "$TMPDIR/zero_extend_pair_test.out.s"; then
    echo "xopt smoke: zero-extend pair test shortcut left copy-back high byte" >&2
    exit 1
fi
if grep -q 'or	a,l' "$TMPDIR/zero_extend_pair_test.out.s"; then
    echo "xopt smoke: zero-extend pair test shortcut left wide OR test" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/zero_extend_pair_test.s" -o "$TMPDIR/zero_extend_pair_test.os.s"
grep -q 'ld	h,b' "$TMPDIR/zero_extend_pair_test.os.s"
grep -q 'or	a,l' "$TMPDIR/zero_extend_pair_test.os.s"

cat >"$TMPDIR/zero_store_chain.s" <<'ASM'
_demo:
	xor	a
	ld	-2(ix),a
	xor	a
	ld	-1(ix),a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/zero_store_chain.s" -o "$TMPDIR/zero_store_chain.out.s"
if [[ "$(grep -c 'xor	a' "$TMPDIR/zero_store_chain.out.s")" != "1" ]]; then
    echo "xopt smoke: redundant zero-store chain cleanup did not keep exactly one xor" >&2
    exit 1
fi
grep -q 'ld	-2(ix),a' "$TMPDIR/zero_store_chain.out.s"
grep -q 'ld	-1(ix),a' "$TMPDIR/zero_store_chain.out.s"

cat >"$TMPDIR/de_xor_right5.s" <<'ASM'
_demo:
	ld	a,e
	ld	l,d
	srl	l
	rr	a
	srl	l
	rr	a
	srl	l
	rr	a
	srl	l
	rr	a
	srl	l
	rr	a
	xor	e
	ld	e,a
	ld	a,l
	xor	d
	ld	d,a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/de_xor_right5.s" -o "$TMPDIR/de_xor_right5.out.s"
grep -q 'and	#248' "$TMPDIR/de_xor_right5.out.s"
grep -q 'and	#7' "$TMPDIR/de_xor_right5.out.s"
if grep -q 'srl	l' "$TMPDIR/de_xor_right5.out.s"; then
    echo "xopt smoke: DE right5 xorshift shortcut did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/hl_xor_right5_stack.s" <<'ASM'
_demo:
	ld	-2(ix),l
	ld	-1(ix),h
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	srl	h
	rr	l
	ld	b,h
	ld	c,l
	ld	l,-2(ix)
	ld	h,-1(ix)
	ld	a,l
	xor	c
	ld	l,a
	ld	a,h
	xor	b
	ld	h,a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/hl_xor_right5_stack.s" -o "$TMPDIR/hl_xor_right5_stack.out.s"
grep -q 'ld	-2(ix),l' "$TMPDIR/hl_xor_right5_stack.out.s"
grep -q 'and	#248' "$TMPDIR/hl_xor_right5_stack.out.s"
grep -q 'or	c' "$TMPDIR/hl_xor_right5_stack.out.s"
if grep -q 'srl	h' "$TMPDIR/hl_xor_right5_stack.out.s"; then
    echo "xopt smoke: HL stack right5 xorshift shortcut did not fire" >&2
    exit 1
fi
if grep -q 'ld	l,-2(ix)' "$TMPDIR/hl_xor_right5_stack.out.s"; then
    echo "xopt smoke: HL stack right5 reload was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/modern_const_return.s" <<'ASM'
_modern:
	; sdcccall(1) prologue: modern (locals=0, temp_frame=0, stack_params=0)
	ld	hl,#42
	ex	de,hl
__modern_end:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/modern_const_return.s" -o "$TMPDIR/modern_const_return.out.s"
grep -q 'ld	de, #42' "$TMPDIR/modern_const_return.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/modern_const_return.out.s"; then
    echo "xopt smoke: modern constant return did not remove ex de,hl" >&2
    exit 1
fi

cat >"$TMPDIR/legacy_const_return.s" <<'ASM'
_legacy:
	ld	hl,#42
	ex	de,hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/legacy_const_return.s" -o "$TMPDIR/legacy_const_return.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/legacy_const_return.out.s"
if grep -q 'ld	de, #42' "$TMPDIR/legacy_const_return.out.s"; then
    echo "xopt smoke: constant return fired without sdcccall(1) marker" >&2
    exit 1
fi

cat >"$TMPDIR/exx_cancel.s" <<'ASM'
_demo:
	ld	a,#1
	exx
	exx
	ret
ASM

"$XOPT" -O3 "$TMPDIR/exx_cancel.s" -o "$TMPDIR/exx_cancel.out.s"
if grep -Eq '^[[:space:]]+exx([[:space:]]|$)' "$TMPDIR/exx_cancel.out.s"; then
    echo "xopt smoke: adjacent exx pair was not cancelled in O3" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/exx_cancel.s" -o "$TMPDIR/exx_cancel.os.s"
if [[ "$(grep -Ec '^[[:space:]]+exx([[:space:]]|$)' "$TMPDIR/exx_cancel.os.s")" != "2" ]]; then
    echo "xopt smoke: adjacent exx pair should stay outside O3/Of dragons" >&2
    exit 1
fi

cat >"$TMPDIR/call_arg_de_direct.s" <<'ASM'
_demo:
	ld	hl,#1
	push	hl
	ld	hl,#2
	ex	de,hl
	ld	hl,#3
	call	_foo
	ret
ASM

"$XOPT" -O3 "$TMPDIR/call_arg_de_direct.s" -o "$TMPDIR/call_arg_de_direct.out.s"
grep -q 'ld	de, #2' "$TMPDIR/call_arg_de_direct.out.s"
grep -q 'ld	hl,#1' "$TMPDIR/call_arg_de_direct.out.s"
grep -q 'push	hl' "$TMPDIR/call_arg_de_direct.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/call_arg_de_direct.out.s"; then
    echo "xopt smoke: call argument DE direct load did not remove ex de,hl" >&2
    exit 1
fi

cat >"$TMPDIR/call_arg_de_same.s" <<'ASM'
_demo:
	ld	hl,#0
	push	hl
	ld	hl,#0
	ex	de,hl
	ld	hl,#4
	call	_foo
	ret
ASM

"$XOPT" -O3 "$TMPDIR/call_arg_de_same.s" -o "$TMPDIR/call_arg_de_same.out.s"
grep -q 'ld	de, #0' "$TMPDIR/call_arg_de_same.out.s"
grep -q 'push	de' "$TMPDIR/call_arg_de_same.out.s"
if grep -Eq 'push[[:space:]]+hl|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/call_arg_de_same.out.s"; then
    echo "xopt smoke: same-immediate call argument rewrite left old HL path" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/call_arg_de_same.s" -o "$TMPDIR/call_arg_de_same.os.s"
grep -Eq 'push[[:space:]]+hl' "$TMPDIR/call_arg_de_same.os.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/call_arg_de_same.os.s"

cat >"$TMPDIR/dead_hl_exchange.s" <<'ASM'
_demo:
	ld	hl,#9
	ex	de,hl
	pop	hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_hl_exchange.s" -o "$TMPDIR/dead_hl_exchange.out.s"
grep -q 'ld	de, #9' "$TMPDIR/dead_hl_exchange.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/dead_hl_exchange.out.s"; then
    echo "xopt smoke: dead-HL exchange did not become direct DE load" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/dead_hl_exchange.s" -o "$TMPDIR/dead_hl_exchange.os.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/dead_hl_exchange.os.s"

cat >"$TMPDIR/dead_hl_exchange_live.s" <<'ASM'
_demo:
	ld	hl,#9
	ex	de,hl
	or	a,a
	sbc	hl,de
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_hl_exchange_live.s" -o "$TMPDIR/dead_hl_exchange_live.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/dead_hl_exchange_live.out.s"

cat >"$TMPDIR/equal_de_hl_exchange.s" <<'ASM'
_direct:
	ld	h,d
	ld	l,e
	ex	de,hl
	ret
_via_bc:
	ld	b,d
	ld	c,e
	ld	h,b
	ld	l,c
	ex	de,hl
	ret
_via_stack:
	ld	-2(ix), e
	ld	-1(ix), d
	ld	l, -2(ix)
	ld	h, -1(ix)
	ex	de,hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/equal_de_hl_exchange.s" -o "$TMPDIR/equal_de_hl_exchange.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/equal_de_hl_exchange.out.s"; then
    echo "xopt smoke: equal DE/HL exchange cleanup did not remove no-op exchange" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/equal_de_hl_exchange.s" -o "$TMPDIR/equal_de_hl_exchange.os.s"
if [[ "$(grep -Ec 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/equal_de_hl_exchange.os.s")" != "3" ]]; then
    echo "xopt smoke: equal DE/HL exchange cleanup should stay outside -Os" >&2
    exit 1
fi

cat >"$TMPDIR/equal_de_hl_exchange_label.s" <<'ASM'
_guard:
	ld	h,d
	ld	l,e
_entry:
	ex	de,hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/equal_de_hl_exchange_label.s" -o "$TMPDIR/equal_de_hl_exchange_label.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/equal_de_hl_exchange_label.out.s"

cat >"$TMPDIR/exchange_sandwich_de_load.s" <<'ASM'
_demo:
	ex	de,hl
	ld	hl,#1234
	ex	de,hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/exchange_sandwich_de_load.s" -o "$TMPDIR/exchange_sandwich_de_load.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+de, #1234' "$TMPDIR/exchange_sandwich_de_load.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/exchange_sandwich_de_load.out.s"; then
    echo "xopt smoke: exchange sandwich did not become direct DE load" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/exchange_sandwich_de_load.s" -o "$TMPDIR/exchange_sandwich_de_load.os.s"
if [[ "$(grep -Ec 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/exchange_sandwich_de_load.os.s")" != "2" ]]; then
    echo "xopt smoke: exchange sandwich cleanup should stay outside -Os" >&2
    exit 1
fi

cat >"$TMPDIR/exchange_sandwich_de_load_label.s" <<'ASM'
_demo:
	ex	de,hl
_entry:
	ld	hl,#1234
	ex	de,hl
	ret
ASM

"$XOPT" -O3 "$TMPDIR/exchange_sandwich_de_load_label.s" -o "$TMPDIR/exchange_sandwich_de_load_label.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/exchange_sandwich_de_load_label.out.s"

cat >"$TMPDIR/dead_bc_stack_discard.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	ld	b,#1
	ld	c,#2
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_bc_stack_discard.s" -o "$TMPDIR/dead_bc_stack_discard.out.s"
grep -Eq '^[[:space:]]+pop[[:space:]]+bc' "$TMPDIR/dead_bc_stack_discard.out.s"
if grep -Eq '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/dead_bc_stack_discard.out.s"; then
    echo "xopt smoke: dead-BC stack discard did not become pop bc" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/dead_bc_stack_discard.s" -o "$TMPDIR/dead_bc_stack_discard.os.s"
if [[ "$(grep -Ec '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/dead_bc_stack_discard.os.s")" != "2" ]]; then
    echo "xopt smoke: dead-BC stack discard should stay outside O3/Of dragons" >&2
    exit 1
fi

cat >"$TMPDIR/dead_de_stack_discard.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	ld	c,b
	ld	de,#0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_de_stack_discard.s" -o "$TMPDIR/dead_de_stack_discard.out.s"
grep -Eq '^[[:space:]]+pop[[:space:]]+de' "$TMPDIR/dead_de_stack_discard.out.s"
if grep -Eq '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/dead_de_stack_discard.out.s"; then
    echo "xopt smoke: dead-DE stack discard did not become pop de" >&2
    exit 1
fi

cat >"$TMPDIR/dead_bc_stack_discard_live.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	ld	a,b
	ld	b,#1
	ld	c,#2
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_bc_stack_discard_live.s" -o "$TMPDIR/dead_bc_stack_discard_live.out.s"
if [[ "$(grep -Ec '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/dead_bc_stack_discard_live.out.s")" != "2" ]]; then
    echo "xopt smoke: dead-BC stack discard crossed a live BC read" >&2
    exit 1
fi

cat >"$TMPDIR/dead_pair_pop_push.s" <<'ASM'
_demo:
	pop	hl
	push	hl
	ld	hl,#0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_pair_pop_push.s" -o "$TMPDIR/dead_pair_pop_push.out.s"
if grep -Eq '^[[:space:]]+(pop|push)[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push.out.s"; then
    echo "xopt smoke: dead pair pop/push cleanup did not fire" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/dead_pair_pop_push.s" -o "$TMPDIR/dead_pair_pop_push.os.s"
grep -Eq '^[[:space:]]+pop[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push.os.s"
grep -Eq '^[[:space:]]+push[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push.os.s"

cat >"$TMPDIR/dead_pair_pop_push_live.s" <<'ASM'
_demo:
	pop	hl
	push	hl
	ld	a,h
	ld	hl,#0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_pair_pop_push_live.s" -o "$TMPDIR/dead_pair_pop_push_live.out.s"
grep -Eq '^[[:space:]]+pop[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push_live.out.s"
grep -Eq '^[[:space:]]+push[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push_live.out.s"

cat >"$TMPDIR/long_inc_sp.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	ld	hl,#0
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/long_inc_sp.s" -o "$TMPDIR/long_inc_sp.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+hl,#8' "$TMPDIR/long_inc_sp.out.s"
grep -Eq '^[[:space:]]+add[[:space:]]+hl,sp' "$TMPDIR/long_inc_sp.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+sp,hl' "$TMPDIR/long_inc_sp.out.s"
if grep -Eq '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/long_inc_sp.out.s"; then
    echo "xopt smoke: long inc-sp run did not collapse" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/long_inc_sp.s" -o "$TMPDIR/long_inc_sp.os.s"
if [[ "$(grep -Ec '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/long_inc_sp.os.s")" != "8" ]]; then
    echo "xopt smoke: long inc-sp run should stay outside O3/Of dragons" >&2
    exit 1
fi

cat >"$TMPDIR/long_inc_sp_live_hl.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	ld	c,b
	ld	e,d
	ld	a,h
	ld	hl,#0
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/long_inc_sp_live_hl.s" -o "$TMPDIR/long_inc_sp_live_hl.out.s"
if [[ "$(grep -Ec '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/long_inc_sp_live_hl.out.s")" != "6" ]]; then
    echo "xopt smoke: long inc-sp run crossed a live HL read" >&2
    exit 1
fi

cat >"$TMPDIR/long_inc_sp_live_flags.s" <<'ASM'
_demo:
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	inc	sp
	ld	c,b
	ld	e,d
	ld	l,h
	ld	hl,#0
	jr	c,_done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/long_inc_sp_live_flags.s" -o "$TMPDIR/long_inc_sp_live_flags.out.s"
if [[ "$(grep -Ec '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/long_inc_sp_live_flags.out.s")" != "6" ]]; then
    echo "xopt smoke: long inc-sp run crossed a live flag read" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_load_helper_abi.s" <<'ASM'
_demo:
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc,#-2
	call	__xopt_spaghetti_0
	pop	bc
	pop	af
	push	hl
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc,#-4
	call	__xopt_spaghetti_0
	pop	bc
	pop	af
	ret
__xopt_spaghetti_0:
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_load_helper_abi.s" -o "$TMPDIR/spaghetti_load_helper_abi.out.s"
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+af' "$TMPDIR/spaghetti_load_helper_abi.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti load helper ABI rewrite did not sink AF preservation" >&2
    exit 1
fi
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+bc' "$TMPDIR/spaghetti_load_helper_abi.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti load helper ABI rewrite did not sink BC preservation" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+pop[[:space:]]+bc' "$TMPDIR/spaghetti_load_helper_abi.out.s"
grep -Eq '^[[:space:]]+pop[[:space:]]+af' "$TMPDIR/spaghetti_load_helper_abi.out.s"

cat >"$TMPDIR/spaghetti_load_helper_abi_unwrapped.s" <<'ASM'
_demo:
	push	af
	push	bc
	push	ix
	pop	hl
	ld	bc,#-2
	call	__xopt_spaghetti_0
	pop	bc
	pop	af
	push	ix
	pop	hl
	ld	bc,#-4
	call	__xopt_spaghetti_0
	ret
__xopt_spaghetti_0:
	add	hl, bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	ld	l, c
	ld	h, b
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_load_helper_abi_unwrapped.s" -o "$TMPDIR/spaghetti_load_helper_abi_unwrapped.out.s"
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+af' "$TMPDIR/spaghetti_load_helper_abi_unwrapped.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti load helper ABI rewrite crossed an unwrapped call" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_store_helper_abi.s" <<'ASM'
_demo:
	push	af
	push	bc
	push	de
	ld	d,h
	ld	e,l
	push	ix
	pop	hl
	call	__xopt_spaghetti_0
	pop	de
	pop	bc
	pop	af
	ld	hl,#1
	push	af
	push	bc
	push	de
	ld	d,h
	ld	e,l
	push	ix
	pop	hl
	call	__xopt_spaghetti_0
	pop	de
	pop	bc
	pop	af
	ret
__xopt_spaghetti_0:
	ld	bc,#-2
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_store_helper_abi.s" -o "$TMPDIR/spaghetti_store_helper_abi.out.s"
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+af' "$TMPDIR/spaghetti_store_helper_abi.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti store helper ABI rewrite did not sink AF preservation" >&2
    exit 1
fi
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+de' "$TMPDIR/spaghetti_store_helper_abi.out.s")" != "1" ]]; then
    echo "xopt smoke: Spaghetti store helper ABI rewrite did not sink DE preservation" >&2
    exit 1
fi
if awk '$1=="call" && $2=="__xopt_spaghetti_0" {seen=1; next} seen && $1=="pop" && $2=="de" {found=1} {seen=0} END {exit found ? 0 : 1}' "$TMPDIR/spaghetti_store_helper_abi.out.s"; then
    echo "xopt smoke: Spaghetti store helper ABI rewrite left call-site DE restore" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_store_helper_abi_unwrapped.s" <<'ASM'
_demo:
	push	af
	push	bc
	push	de
	ld	d,h
	ld	e,l
	push	ix
	pop	hl
	call	__xopt_spaghetti_0
	pop	de
	pop	bc
	pop	af
	call	__xopt_spaghetti_0
	ret
__xopt_spaghetti_0:
	ld	bc,#-2
	add	hl, bc
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_store_helper_abi_unwrapped.s" -o "$TMPDIR/spaghetti_store_helper_abi_unwrapped.out.s"
awk '$1=="call" && $2=="__xopt_spaghetti_0" {seen=1; next} seen && $1=="pop" && $2=="de" {found=1} {seen=0} END {exit found ? 0 : 1}' "$TMPDIR/spaghetti_store_helper_abi_unwrapped.out.s"

cat >"$TMPDIR/spaghetti_word_flag_inline.s" <<'ASM'
_demo:
	ld	hl,#_word
	call	__xopt_spaghetti_0
	jr	z,_zero
	ld	de,#1
	jr	_done
_zero:
	ld	de,#0
_done:
	ld	bc,#0
	ld	hl,#0
	ret
__xopt_spaghetti_0:
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	ld	a, h
	or	a, l
	ret
_word:
	.dw	0
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_word_flag_inline.s" -o "$TMPDIR/spaghetti_word_flag_inline.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+a, \(hl\)' "$TMPDIR/spaghetti_word_flag_inline.out.s"
grep -Eq '^[[:space:]]+or[[:space:]]+a, \(hl\)' "$TMPDIR/spaghetti_word_flag_inline.out.s"
if grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_spaghetti_0|^__xopt_spaghetti_0:' "$TMPDIR/spaghetti_word_flag_inline.out.s"; then
    echo "xopt smoke: Spaghetti word flag helper was not fully inlined" >&2
    exit 1
fi

"$XOPT" -Os "$TMPDIR/spaghetti_word_flag_inline.s" -o "$TMPDIR/spaghetti_word_flag_inline.os.s"
grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_word_flag_inline.os.s"
grep -Eq '^__xopt_spaghetti_0:' "$TMPDIR/spaghetti_word_flag_inline.os.s"

cat >"$TMPDIR/spaghetti_word_flag_inline_live.s" <<'ASM'
_demo:
	ld	hl,#_word
	call	__xopt_spaghetti_0
	jr	z,_zero
	ld	a,b
	ld	bc,#0
	ld	hl,#0
	ret
_zero:
	ld	bc,#0
	ld	hl,#0
	ret
__xopt_spaghetti_0:
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	ld	a, h
	or	a, l
	ret
_word:
	.dw	0
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_word_flag_inline_live.s" -o "$TMPDIR/spaghetti_word_flag_inline_live.out.s"
grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_word_flag_inline_live.out.s"
grep -Eq '^__xopt_spaghetti_0:' "$TMPDIR/spaghetti_word_flag_inline_live.out.s"

cat >"$TMPDIR/spaghetti_de_flag_inline.s" <<'ASM'
_demo:
	call	_get_status
	call	__xopt_spaghetti_0
	jr	z,_zero
	ld	bc,#0
	ld	hl,#0
	ret
_zero:
	ld	bc,#0
	ld	hl,#0
	ret
__xopt_spaghetti_0:
	ld	h, d
	ld	l, e
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_de_flag_inline.s" -o "$TMPDIR/spaghetti_de_flag_inline.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+a, d' "$TMPDIR/spaghetti_de_flag_inline.out.s"
grep -Eq '^[[:space:]]+or[[:space:]]+a, e' "$TMPDIR/spaghetti_de_flag_inline.out.s"
if grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_spaghetti_0|^__xopt_spaghetti_0:' "$TMPDIR/spaghetti_de_flag_inline.out.s"; then
    echo "xopt smoke: Spaghetti DE flag helper was not fully inlined" >&2
    exit 1
fi

cat >"$TMPDIR/spaghetti_de_flag_inline_live.s" <<'ASM'
_demo:
	call	_get_status
	call	__xopt_spaghetti_0
	jr	z,_zero
	ld	a,h
	ld	bc,#0
	ld	hl,#0
	ret
_zero:
	ld	bc,#0
	ld	hl,#0
	ret
__xopt_spaghetti_0:
	ld	h, d
	ld	l, e
	ld	b, h
	ld	c, l
	ld	a, h
	or	a, l
	ret
ASM

"$XOPT" -O3 "$TMPDIR/spaghetti_de_flag_inline_live.s" -o "$TMPDIR/spaghetti_de_flag_inline_live.out.s"
grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_de_flag_inline_live.out.s"
grep -Eq '^__xopt_spaghetti_0:' "$TMPDIR/spaghetti_de_flag_inline_live.out.s"

"$XOPT" -O3 --cross-file "$TMPDIR/in.s" "$TMPDIR/in.s" --stdout >/dev/null

mkdir "$TMPDIR/optimized"
cp "$TMPDIR/in.s" "$TMPDIR/second.s"
"$XOPT" -O3 --out-dir "$TMPDIR/optimized" "$TMPDIR"/*.s
test -s "$TMPDIR/optimized/in.s"
test -s "$TMPDIR/optimized/second.s"

"$XOPT" --stats -O3 "$TMPDIR"/*.s >"$TMPDIR/stats.txt"
grep -q 'saved' "$TMPDIR/stats.txt"
grep -q 'total' "$TMPDIR/stats.txt"

"$XOPT" --reg-coverage "$TMPDIR/in.s" >"$TMPDIR/regcov.txt"
grep -q 'press' "$TMPDIR/regcov.txt"
grep -q '_demo' "$TMPDIR/regcov.txt"

cat >"$TMPDIR/alt_regs.s" <<'ASM'
_alt_demo:
	ex	af,af'
	exx
	exx
	ex	af,af'
	ret
ASM

"$XOPT" --reg-coverage "$TMPDIR/alt_regs.s" >"$TMPDIR/alt_regs.txt"
grep -q 'swap' "$TMPDIR/alt_regs.txt"
grep -q "A'" "$TMPDIR/alt_regs.txt"

if "$XOPT" -O1 "$TMPDIR/in.s" >/dev/null 2>&1; then
    echo "xopt smoke: -O1 should be rejected" >&2
    exit 1
fi

echo "xopt smoke: ok"
