#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
XOPT="${1:-$SCRIPT_DIR/../../../../bin/x/bin/xopt}"
if [[ ! -x "$XOPT" ]]; then
    echo "xopt smoke: executable not found: $XOPT" >&2
    exit 1
fi
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

cat >"$TMPDIR/exx_island.s" <<'ASM'
	.area	_CODE
_exx_island:
	exx
	ld	a, 3(ix)
	xor	#0x5a
	ld	4(iy), a
	exx
	ret
_exx_live_pair:
	exx
	ld	a, c
	exx
	ret
ASM

"$XOPT" -O3 "$TMPDIR/exx_island.s" -o "$TMPDIR/exx_island.out.s"
if awk '
    /^_exx_island:/ { in_fn=1; next }
    /^_exx_live_pair:/ { in_fn=0 }
    in_fn && /^[[:space:]]*exx/ { found=1 }
    END { exit found ? 0 : 1 }
' "$TMPDIR/exx_island.out.s"; then
    echo "xopt smoke: register-independent EXX island was not collapsed" >&2
    exit 1
fi
if ! awk '
    /^_exx_live_pair:/ { in_fn=1; next }
    in_fn && /^[[:space:]]*exx/ { count++ }
    END { exit count == 2 ? 0 : 1 }
' "$TMPDIR/exx_island.out.s"; then
    echo "xopt smoke: EXX cleanup crossed a live swapped register" >&2
    exit 1
fi

cat >"$TMPDIR/ix_rmw.s" <<'ASM'
	.area	_CODE
_ix_rmw:
	; sdcccall(1) prologue: ix_rmw (locals=0, temp_frame=1, stack_params=0)
	ld	-1(ix), a
	set	1, -1(ix)
	ld	hl, #_sink
	ld	a, -1(ix)
	ld	(hl), a
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_rmw.s" -o "$TMPDIR/ix_rmw.out.s"
if ! grep -Eq 'ld[[:space:]]+-1\(ix\), ?a' "$TMPDIR/ix_rmw.out.s" ||
   ! grep -Eq 'ld[[:space:]]+a, ?-1\(ix\)' "$TMPDIR/ix_rmw.out.s"; then
    echo "xopt smoke: SET lost the reaching IX store or modified-byte reload" >&2
    exit 1
fi

cat >"$TMPDIR/reused_local_label.s" <<'ASM'
	.area	_CODE
_reused_label_first:
	; sdcccall(1) prologue: reused_label_first (locals=0, temp_frame=2, stack_params=0)
	ld	-2(ix), l
	ld	-1(ix), h
	jp	__reused_inline_exit
__reused_inline_exit:
	ld	e, -2(ix)
	ld	d, -1(ix)
	ret
	.area	_CODE
_reused_label_second:
	; sdcccall(1) prologue: reused_label_second (locals=0, temp_frame=4, stack_params=0)
	ld	-2(ix), l
	ld	-1(ix), h
	ld	l, -2(ix)
	ld	h, -1(ix)
	ld	-4(ix), l
	ld	-3(ix), h
	jp	__reused_inline_exit
	ld	hl, #0
	ld	-4(ix), l
	ld	-3(ix), h
__reused_inline_exit:
	ld	e, -4(ix)
	ld	d, -3(ix)
	ret
ASM

"$XOPT" -O2 "$TMPDIR/reused_local_label.s" \
    -o "$TMPDIR/reused_local_label.out.s"
if ! awk '
    /^_reused_label_second:/ { in_fn=1; next }
    in_fn && /^[[:space:]]*\.area/ { in_fn=0 }
    in_fn && /ld[[:space:]]+-4\(ix\),[[:space:]]*l/ { low=1 }
    in_fn && /ld[[:space:]]+-3\(ix\),[[:space:]]*h/ { high=1 }
    END { exit low && high ? 0 : 1 }
' "$TMPDIR/reused_local_label.out.s"; then
    echo "xopt smoke: duplicate local label hid a live IX result store" >&2
    exit 1
fi

cat >"$TMPDIR/de_reload_loop_join.s" <<'ASM'
	.area	_CODE
_de_reload_loop_join:
	; sdcccall(1) prologue: de_reload_loop_join (locals=0, temp_frame=2, stack_params=0)
	ld	-2(ix), e
	ld	-1(ix), d
	ld	a, #5
_de_reload_loop:
	ld	e, -2(ix)
	ld	d, -1(ix)
	inc	de
	ld	-2(ix), e
	ld	-1(ix), d
	dec	a
	jr	nz, _de_reload_loop
	ret
ASM

"$XOPT" -Of "$TMPDIR/de_reload_loop_join.s" \
    -o "$TMPDIR/de_reload_loop_join.out.s"
if ! awk '
    /^_de_reload_loop:/ { in_loop=1; next }
    in_loop && /ld[[:space:]]+e, ?-2\(ix\)/ { low=1 }
    in_loop && /ld[[:space:]]+d, ?-1\(ix\)/ { high=1 }
    END { exit low && high ? 0 : 1 }
' "$TMPDIR/de_reload_loop_join.out.s"; then
    echo "xopt smoke: DE reload was forwarded across a loop join" >&2
    exit 1
fi

cat >"$TMPDIR/bc_live_loop.s" <<'ASM'
	.area	_CODE
_bc_live_loop:
	ld	hl, #0
	ld	b, h
	ld	c, l
	add	hl, hl
	push	hl
	pop	iy
_bc_live_loop_body:
	ld	a, c
	sub	a, -2(ix)
	ld	a, b
	sbc	a, -1(ix)
	jr	nc, _bc_live_loop_exit
	inc	bc
	inc	iy
	inc	iy
	jr	_bc_live_loop_body
_bc_live_loop_exit:
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_live_loop.s" -o "$TMPDIR/bc_live_loop.out.s"
if ! grep -Eq 'ld[[:space:]]+b, ?h' "$TMPDIR/bc_live_loop.out.s" ||
   ! grep -Eq 'ld[[:space:]]+c, ?l' "$TMPDIR/bc_live_loop.out.s"; then
    echo "xopt smoke: live BC initialization was removed across a loop" >&2
    exit 1
fi

cat >"$TMPDIR/unused_ix_frame.s" <<'ASM'
	.area	_CODE
_frameless_leaf:
	; sdcccall(1) prologue: frameless_leaf (locals=0, temp_frame=0, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	inc	a
__frameless_leaf_end:
	; epilogue: frameless_leaf
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_frame_still_used:
	; sdcccall(1) prologue: frame_still_used (locals=0, temp_frame=0, stack_params=1)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	a, 4(ix)
__frame_still_used_end:
	; epilogue: frame_still_used
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_dead_allocated_frame:
	; sdcccall(1) prologue: dead_allocated_frame (locals=0, temp_frame=25, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-25
	add	hl, sp
	ld	sp, hl
	add	a, a
__dead_allocated_frame_end:
	; epilogue: dead_allocated_frame
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_mismatched_allocated_frame:
	; sdcccall(1) prologue: mismatched_allocated_frame (locals=0, temp_frame=8, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-7
	add	hl, sp
	ld	sp, hl
	add	a, a
__mismatched_allocated_frame_end:
	; epilogue: mismatched_allocated_frame
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_allocated_frame_hl_live:
	; sdcccall(1) prologue: allocated_frame_hl_live (locals=0, temp_frame=8, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	ld	a, l
__allocated_frame_hl_live_end:
	; epilogue: allocated_frame_hl_live
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_allocated_frame_carry_live:
	; sdcccall(1) prologue: allocated_frame_carry_live (locals=0, temp_frame=8, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	ld	hl, #-8
	add	hl, sp
	ld	sp, hl
	jr	c, __allocated_frame_carry_seen
	xor	a
__allocated_frame_carry_seen:
__allocated_frame_carry_live_end:
	; epilogue: allocated_frame_carry_live
	ld	sp, ix
	pop	ix
	ret
ASM

"$XOPT" -O3 "$TMPDIR/unused_ix_frame.s" \
    -o "$TMPDIR/unused_ix_frame.out.s"
if awk '
    /^_frameless_leaf:/ { in_fn=1; next }
    /^_frame_still_used:/ { in_fn=0 }
    in_fn && /(push[[:space:]]+ix|pop[[:space:]]+ix|ld[[:space:]]+ix|ld[[:space:]]+sp,[[:space:]]*ix|add[[:space:]]+ix,[[:space:]]*sp)/ { bad=1 }
    END { exit bad ? 0 : 1 }
' "$TMPDIR/unused_ix_frame.out.s"; then
    echo "xopt smoke: unused canonical IX frame was not removed" >&2
    exit 1
fi
if ! awk '
    /^_frame_still_used:/ { in_fn=1; next }
    in_fn && /push[[:space:]]+ix/ { push_ix=1 }
    in_fn && /ld[[:space:]]+a, ?4\(ix\)/ { load_ix=1 }
    END { exit push_ix && load_ix ? 0 : 1 }
' "$TMPDIR/unused_ix_frame.out.s"; then
    echo "xopt smoke: live IX parameter frame was removed" >&2
    exit 1
fi
if awk '
    /^_dead_allocated_frame:/ { in_fn=1; next }
    /^_mismatched_allocated_frame:/ { in_fn=0 }
    in_fn && /(push[[:space:]]+ix|pop[[:space:]]+ix|ld[[:space:]]+(ix|sp)|add[[:space:]]+(ix|hl),[[:space:]]*sp)/ { bad=1 }
    END { exit bad ? 0 : 1 }
' "$TMPDIR/unused_ix_frame.out.s"; then
    echo "xopt smoke: exact dead temporary allocation was not removed" >&2
    exit 1
fi
for fn in mismatched_allocated_frame allocated_frame_hl_live allocated_frame_carry_live; do
    if ! awk -v fn="_$fn:" '
        $0 == fn { in_fn=1; next }
        in_fn && /^_[A-Za-z0-9_]+:/ { in_fn=0 }
        in_fn && /push[[:space:]]+ix/ { kept=1 }
        END { exit kept ? 0 : 1 }
    ' "$TMPDIR/unused_ix_frame.out.s"; then
        echo "xopt smoke: unsafe allocated frame $fn was removed" >&2
        exit 1
    fi
done

cat >"$TMPDIR/add_one_return_flags.s" <<'ASM'
	.area	_CODE
_add_one_flags_dead:
	; sdcccall(1) prologue: add_one_flags_dead (locals=0, temp_frame=0, stack_params=0)
	add	a, #1
	ld	(_sink), a
	; epilogue: add_one_flags_dead
	ret
	.area	_CODE
_add_one_carry_live:
	; sdcccall(1) prologue: add_one_carry_live (locals=0, temp_frame=0, stack_params=0)
	add	a, #1
	jr	c, _carry_seen
	ret
_carry_seen:
	ret
	.area	_CODE
_add_one_asm_carry_result:
	add	a, #1
	ret
ASM

"$XOPT" -O3 "$TMPDIR/add_one_return_flags.s" \
    -o "$TMPDIR/add_one_return_flags.out.s"
if ! awk '
    /^_add_one_flags_dead:/ { in_fn=1; next }
    /^_add_one_carry_live:/ { in_fn=0 }
    in_fn && /inc[[:space:]]+a/ { found=1 }
    END { exit found ? 0 : 1 }
' "$TMPDIR/add_one_return_flags.out.s"; then
    echo "xopt smoke: add-one with return-dead flags did not use INC A" >&2
    exit 1
fi
if ! awk '
    /^_add_one_carry_live:/ { in_fn=1; next }
    /^_add_one_asm_carry_result:/ { in_fn=0 }
    in_fn && /add[[:space:]]+a, ?#1/ { found=1 }
    END { exit found ? 0 : 1 }
' "$TMPDIR/add_one_return_flags.out.s"; then
    echo "xopt smoke: add-one lost carry consumed by a branch" >&2
    exit 1
fi
if ! awk '
    /^_add_one_asm_carry_result:/ { in_fn=1; next }
    in_fn && /add[[:space:]]+a, ?#1/ { found=1 }
    END { exit found ? 0 : 1 }
' "$TMPDIR/add_one_return_flags.out.s"; then
    echo "xopt smoke: add-one changed an assembly carry result" >&2
    exit 1
fi

cat >"$TMPDIR/equ_case.s" <<'ASM'
XLO     .equ    -6
THI     .equ    -1
_demo:
        ld      XLO(ix),a
        ld      h,THI(ix)
        ret
ASM

"$XOPT" -O3 "$TMPDIR/equ_case.s" -o "$TMPDIR/equ_case.out.s"
grep -q '^XLO[[:space:]]\.equ' "$TMPDIR/equ_case.out.s"
grep -q '^THI[[:space:]]\.equ' "$TMPDIR/equ_case.out.s"
grep -q 'XLO(ix)' "$TMPDIR/equ_case.out.s"
grep -q 'THI(ix)' "$TMPDIR/equ_case.out.s"

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

"$XOPT" -Os "$TMPDIR/spaghetti.s" -o "$TMPDIR/spaghetti.os.out.s"
if grep -q '__xopt_spaghetti_' "$TMPDIR/spaghetti.os.out.s"; then
    echo "xopt smoke: stable -Os unexpectedly fired Spaghetti outlining" >&2
    exit 1
fi

"$XOPT" -O3 "$TMPDIR/spaghetti.s" -o "$TMPDIR/spaghetti.out.s"
if grep -q '__xopt_spaghetti_' "$TMPDIR/spaghetti.out.s"; then
    echo "xopt smoke: O3 unexpectedly fired disabled Spaghetti outlining" >&2
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

cat >"$TMPDIR/size_outline.s" <<'ASM'
	.area	_CODE
_outline_a:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	ld	e,#5
	inc	a
	ret
_outline_b:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	ld	e,#5
	dec	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/size_outline.s" -o "$TMPDIR/size_outline.os.s"
if [[ "$(grep -Ec '^[[:space:]]+call[[:space:]]+__xopt_outline_' "$TMPDIR/size_outline.os.s")" != "2" ]]; then
    echo "xopt smoke: -Os did not share a profitable repeated sequence" >&2
    exit 1
fi
grep -Eq '^__xopt_outline_[0-9]+:' "$TMPDIR/size_outline.os.s"

"$XOPT" -Of "$TMPDIR/size_outline.s" -o "$TMPDIR/size_outline.of.s"
if grep -q '__xopt_outline_' "$TMPDIR/size_outline.of.s"; then
    echo "xopt smoke: speed mode unexpectedly enabled size outlining" >&2
    exit 1
fi

{
    printf '\t.area\t_CODE\n'
    for ((i = 0; i < 4010; ++i)); do
        printf '_large_barrier_%d:\n\tnop\n' "$i"
    done
    cat <<'ASM'
_large_outline_a:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	ld	e,#5
	inc	a
	ret
_large_outline_b:
	ld	a,#1
	ld	b,#2
	ld	c,#3
	ld	d,#4
	ld	e,#5
	dec	a
	ret
ASM
} >"$TMPDIR/large_size_outline.s"

"$XOPT" -Os "$TMPDIR/large_size_outline.s" \
    -o "$TMPDIR/large_size_outline.out.s"
if [[ "$(grep -Ec '^[[:space:]]+call[[:space:]]+__xopt_outline_' \
        "$TMPDIR/large_size_outline.out.s")" != "2" ]]; then
    echo "xopt smoke: scalable large -Os path did not outline" >&2
    exit 1
fi

cat >"$TMPDIR/outline_register_result.s" <<'ASM'
	.area	_CODE
_result_0:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#0
	ld	(bc),a
	ret
_result_1:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#1
	ld	(bc),a
	ret
_result_2:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#2
	ld	(bc),a
	ret
_result_3:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#3
	ld	(bc),a
	ret
_result_4:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#4
	ld	(bc),a
	ret
_result_5:
	inc	hl
	inc	hl
	ld	b,h
	ld	c,l
	ld	a,#5
	ld	(bc),a
	ret
ASM

"$XOPT" -Os "$TMPDIR/outline_register_result.s" -o "$TMPDIR/outline_register_result.out.s"
grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_outline_' "$TMPDIR/outline_register_result.out.s"
if ! grep -Eq '^[[:space:]]+ld[[:space:]]+b, ?h' "$TMPDIR/outline_register_result.out.s" ||
   ! grep -Eq '^[[:space:]]+ld[[:space:]]+c, ?l' "$TMPDIR/outline_register_result.out.s"; then
    echo "xopt smoke: outlined helper lost a live register result" >&2
    exit 1
fi

cat >"$TMPDIR/outline_final_layout.s" <<'ASM'
	.area	_CODE
_outline_tail_caller:
	call	__xopt_outline_900	; xopt-ix:none
	ret
__xopt_outline_900:
	ld	a,#1
	inc	a
	ret
_outline_jump_next:
	jr	_outline_next
_outline_next:
	ret
ASM

"$XOPT" -Os "$TMPDIR/outline_final_layout.s" -o "$TMPDIR/outline_final_layout.out.s"
if grep -Eq '^[[:space:]]+call[[:space:]]+__xopt_outline_900|^[[:space:]]+jr[[:space:]]+_outline_next' "$TMPDIR/outline_final_layout.out.s"; then
    echo "xopt smoke: outlined final layout was not compacted" >&2
    exit 1
fi

cat >"$TMPDIR/size_tail_merge.s" <<'ASM'
	.area	_CODE
_tail_a:
	ld	a,#1
	ld	b,#2
	ret
_tail_b:
	ld	a,#1
	ld	b,#2
	ret
ASM

"$XOPT" -Os "$TMPDIR/size_tail_merge.s" -o "$TMPDIR/size_tail_merge.os.s"
grep -Eq '^__xopt_tail_[0-9]+:' "$TMPDIR/size_tail_merge.os.s"
grep -Eq '^[[:space:]]+j[pr][[:space:]]+__xopt_tail_' "$TMPDIR/size_tail_merge.os.s"

cat >"$TMPDIR/size_stack_tail_merge.s" <<'ASM'
	.area	_CODE
_stack_tail_a:
	ld	sp,ix
	pop	ix
	ret
_stack_tail_b:
	ld	sp,ix
	pop	ix
	ret
ASM

"$XOPT" -Os "$TMPDIR/size_stack_tail_merge.s" -o "$TMPDIR/size_stack_tail_merge.os.s"
grep -Eq '^__xopt_tail_[0-9]+:' "$TMPDIR/size_stack_tail_merge.os.s"
grep -Eq '^[[:space:]]+j[pr][[:space:]]+__xopt_tail_' "$TMPDIR/size_stack_tail_merge.os.s"

cat >"$TMPDIR/size_outline_control.s" <<'ASM'
	.area	_CODE
_control_a:
	ld	a,#1
	ld	b,#2
	jr	z,_control_a_done
	ld	c,#3
	ld	d,#4
_control_a_done:
	ret
_control_b:
	ld	a,#1
	ld	b,#2
	jr	z,_control_b_done
	ld	c,#3
	ld	d,#4
_control_b_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/size_outline_control.s" -o "$TMPDIR/size_outline_control.out.s"
if grep -q '__xopt_outline_' "$TMPDIR/size_outline_control.out.s"; then
    echo "xopt smoke: -Os outlined through control flow" >&2
    exit 1
fi

cat >"$TMPDIR/outline_ix_barrier.s" <<'ASM'
	.area	_CODE
_outline_ix_caller:
	; sdcccall(1) prologue: outline_ix_caller (locals=0, temp_frame=2, stack_params=0)
	ld	-2(ix),#17
	call	__xopt_outline_0
	ret
	.area	_CODE
__xopt_outline_0:
	ld	a,-2(ix)
	inc	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/outline_ix_barrier.s" -o "$TMPDIR/outline_ix_barrier.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+-2\(ix\), ?#17' "$TMPDIR/outline_ix_barrier.out.s"

cat >"$TMPDIR/register_shift_xor_diamond.s" <<'ASM'
	.area	_CODE
_register_shift_xor:
	bit	7,e
	jr	z,_register_shift_else
_register_shift_true:
	ld	a,e
	add	a,a
	xor	#7
	ld	e,a
	jr	_register_shift_join
_register_shift_else:
	ld	a,e
	add	a,a
	ld	e,a
_register_shift_join:
	ret
ASM

"$XOPT" -Os "$TMPDIR/register_shift_xor_diamond.s" -o "$TMPDIR/register_shift_xor_diamond.out.s"
if grep -Eq '^[[:space:]]+bit[[:space:]]+7, ?e' "$TMPDIR/register_shift_xor_diamond.out.s"; then
    echo "xopt smoke: register shift/XOR diamond was not folded" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+jr[[:space:]]+nc, ?_register_shift_join' "$TMPDIR/register_shift_xor_diamond.out.s"

cat >"$TMPDIR/register_shift_xor_tail_diamond.s" <<'ASM'
	.area	_CODE
_register_shift_xor_tail:
	bit	7,e
	jr	z,_register_tail_else
_register_tail_true:
	ld	a,e
	add	a,a
	xor	#7
	ld	e,a
	jr	_register_tail_continue
_register_tail_else:
	ld	a,e
	add	a,a
	ld	e,a
_register_tail_fallthrough:
	jr	_register_tail_continue
_register_tail_continue:
	ret
ASM

"$XOPT" -Os "$TMPDIR/register_shift_xor_tail_diamond.s" -o "$TMPDIR/register_shift_xor_tail_diamond.out.s"
if grep -Eq '^[[:space:]]+bit[[:space:]]+7, ?e' "$TMPDIR/register_shift_xor_tail_diamond.out.s"; then
    echo "xopt smoke: register tail shift/XOR diamond was not folded" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+jr[[:space:]]+nc, ?_register_tail_else' "$TMPDIR/register_shift_xor_tail_diamond.out.s"

cat >"$TMPDIR/slot_shift_xor_chain.s" <<'ASM'
	.area	_CODE
_slot_shift_xor_chain:
	; sdcccall(0) prologue: slot_shift_xor_chain (locals=0, temp_frame=1, stack_params=0)
	ld	-1(ix),a
	bit	7,-1(ix)
	jr	z,_slot_shift_else
_slot_shift_true:
	add	a,a
	xor	#7
	ld	-1(ix),a
	jr	_slot_shift_join
_slot_shift_else:
	add	a,a
	ld	-1(ix),a
_slot_shift_join:
	bit	7,-1(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/slot_shift_xor_chain.s" -o "$TMPDIR/slot_shift_xor_chain.out.s"
if [[ "$(grep -Ec '^[[:space:]]+bit[[:space:]]+7, ?-1\(ix\)' "$TMPDIR/slot_shift_xor_chain.out.s")" != "1" ]]; then
    echo "xopt smoke: slot-backed shift/XOR recurrence was not folded" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+jr[[:space:]]+nc, ?_slot_shift_join' "$TMPDIR/slot_shift_xor_chain.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+-1\(ix\), ?a' "$TMPDIR/slot_shift_xor_chain.out.s"

cat >"$TMPDIR/legacy_return_after_xopt_label.s" <<'ASM'
_legacy_return_after_xopt_label:
	; sdcccall(0) prologue: legacy_return_after_xopt_label (locals=0, temp_frame=0, stack_params=0)
__xopt_inc16_0:
	ld	h,b
	ld	l,c
	jr	_legacy_return_after_xopt_label_end
_legacy_return_after_xopt_label_end:
	jp	__sdcc_leave_ix
ASM

"$XOPT" -Os "$TMPDIR/legacy_return_after_xopt_label.s" -o "$TMPDIR/legacy_return_after_xopt_label.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+h, ?b' "$TMPDIR/legacy_return_after_xopt_label.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+l, ?c' "$TMPDIR/legacy_return_after_xopt_label.out.s"

cat >"$TMPDIR/prewritten_stack_alloc.s" <<'ASM'
	.area	_CODE
_prewritten_stack_alloc:
	; sdcccall(1) prologue: prewritten_stack_alloc (locals=0, temp_frame=8, stack_params=0)
	dec	sp
	dec	sp
	ld	-10(ix),l
	ld	-9(ix),h
	ld	hl,#-8
	add	hl,sp
	ld	sp,hl
	ret
ASM

"$XOPT" -Os "$TMPDIR/prewritten_stack_alloc.s" -o "$TMPDIR/prewritten_stack_alloc.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+hl, ?#-8' "$TMPDIR/prewritten_stack_alloc.out.s"
if grep -Eq '^[[:space:]]+push[[:space:]]+af' "$TMPDIR/prewritten_stack_alloc.out.s"; then
    echo "xopt smoke: PUSH allocation overwrote a prewritten IX slot" >&2
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
grep -q 'call	__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail.out.s"
if grep -Eq '^[[:space:]]+j[pr][[:space:]]+__xopt_spaghetti_0' "$TMPDIR/spaghetti_tail.out.s"; then
    echo "xopt smoke: O3 unexpectedly fired disabled Spaghetti tail threading" >&2
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

cat >"$TMPDIR/scaled_imm_base.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=8, stack_params=0)
	ld	l, -2(ix)
	ld	h, -1(ix)
	dec	hl
	add	hl, hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #_stk
	ld	e, -4(ix)
	ld	d, -3(ix)
	add	hl, de
	ld	a, (hl)
	ret
ASM

"$XOPT" -Os "$TMPDIR/scaled_imm_base.s" -o "$TMPDIR/scaled_imm_base.out.s"
grep -q 'ld	de, #_stk' "$TMPDIR/scaled_imm_base.out.s"
if grep -Eq 'ld[[:space:]]+-[34]\(ix\)' "$TMPDIR/scaled_imm_base.out.s"; then
    echo "xopt smoke: scaled immediate-base temp elide did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/de_scaled_imm_base.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=8, stack_params=0)
	ld	b, d
	ld	c, e
	ld	h, b
	ld	l, c
	add	hl, hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #_mem
	ld	e, -4(ix)
	ld	d, -3(ix)
	add	hl, de
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	bc, #0
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_scaled_imm_base.s" -o "$TMPDIR/de_scaled_imm_base.out.s"
grep -q 'ld	de, #_mem' "$TMPDIR/de_scaled_imm_base.out.s"
grep -Eq 'ld[[:space:]]+h,[[:space:]]*d|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/de_scaled_imm_base.out.s"
if grep -Eq 'ld[[:space:]]+-[34]\(ix\)|ld[[:space:]]+b,[[:space:]]*d|ld[[:space:]]+c,[[:space:]]*e' "$TMPDIR/de_scaled_imm_base.out.s"; then
    echo "xopt smoke: DE scaled immediate-base temp elide did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/truncated_byte_xor.s" <<'ASM'
_demo:
	ld	a, -2(ix)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	ld	a, -13(ix)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	ld	b, h
	ld	c, l
	ld	a, -2(ix)
	ld	l, a
	rlca
	sbc	a, a
	ld	h, a
	ld	a, l
	xor	a, c
	ld	l, a
	ld	a, h
	xor	a, b
	ld	h, a
	ld	a, l
	ld	-2(ix), a
	inc	-9(ix)
	ld	l, -9(ix)
	ld	h, -8(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/truncated_byte_xor.s" -o "$TMPDIR/truncated_byte_xor.out.s"
grep -q 'xor	a, -13(ix)' "$TMPDIR/truncated_byte_xor.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|xor[[:space:]]+a,[[:space:]]*b|xor[[:space:]]+a,[[:space:]]*c' "$TMPDIR/truncated_byte_xor.out.s"; then
    echo "xopt smoke: truncated promoted byte xor was not collapsed" >&2
    exit 1
fi

cat >"$TMPDIR/srl_a_const_shift_flags.s" <<'ASM'
	.area	_CODE
_srl_flags_dead_after_spill:
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-1(ix), a
	xor	a
	ret
	.area	_CODE
_srl_carry_live_after_spill:
	srl	a
	srl	a
	srl	a
	ld	(_sink), a
	jr	c, _srl_carry_seen
	ret
_srl_carry_seen:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/srl_a_const_shift_flags.s" \
    -o "$TMPDIR/srl_a_const_shift_flags.out.s"
if ! awk '
    /^_srl_flags_dead_after_spill:/ { in_fn=1; next }
    /^_srl_carry_live_after_spill:/ { in_fn=0 }
    in_fn && /rlca/ { rotate=1 }
    in_fn && /and[[:space:]]+#1/ { mask=1 }
    in_fn && /srl[[:space:]]+a/ { stale=1 }
    END { exit rotate && mask && !stale ? 0 : 1 }
' "$TMPDIR/srl_a_const_shift_flags.out.s"; then
    echo "xopt smoke: constant SRL chain did not cross a flag-transparent spill" >&2
    exit 1
fi
if [[ "$(awk '
    /^_srl_carry_live_after_spill:/ { in_fn=1; next }
    in_fn && /^_srl_carry_seen:/ { in_fn=0 }
    in_fn && /srl[[:space:]]+a/ { ++count }
    END { print count + 0 }
' "$TMPDIR/srl_a_const_shift_flags.out.s")" != "3" ]]; then
    echo "xopt smoke: carry-live constant SRL chain was rewritten" >&2
    exit 1
fi

cat >"$TMPDIR/switch_key_direct.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=12, stack_params=0)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-10(ix), e
	ld	-9(ix), d
	ld	l, -10(ix)
	ld	h, -9(ix)
	ld	a, h
	or	a, a
	jp	nz, _default
	ld	a, l
	cp	#12
	ret
_default:
	ret
ASM

"$XOPT" -Os "$TMPDIR/switch_key_direct.s" -o "$TMPDIR/switch_key_direct.out.s"
grep -q 'ld	a, d' "$TMPDIR/switch_key_direct.out.s"
grep -q 'ld	a, e' "$TMPDIR/switch_key_direct.out.s"
if grep -Eq -- '-1[09]\(ix\)' "$TMPDIR/switch_key_direct.out.s"; then
    echo "xopt smoke: switch key temp spill was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/de_temp_branch_reuse.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=12, stack_params=0)
	ld	-8(ix), e
	ld	-7(ix), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	jp	m, _left
	jp	_right
_left:
	ret
_right:
	ld	l, -4(ix)
	ld	h, -3(ix)
	push	hl
	ld	l, -8(ix)
	ld	h, -7(ix)
	pop	de
	ex	de, hl
	or	a, a
	sbc	hl, de
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_temp_branch_reuse.s" -o "$TMPDIR/de_temp_branch_reuse.out.s"
grep -Eq 'ld[[:space:]]+-8\(ix\),[[:space:]]*e' "$TMPDIR/de_temp_branch_reuse.out.s"
grep -Eq 'ld[[:space:]]+-7\(ix\),[[:space:]]*d' "$TMPDIR/de_temp_branch_reuse.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-8[[:space:]]*\(ix\)' "$TMPDIR/de_temp_branch_reuse.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-7[[:space:]]*\(ix\)' "$TMPDIR/de_temp_branch_reuse.out.s"

cat >"$TMPDIR/de_temp_reload_addr.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=16, stack_params=0)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-12(ix), e
	ld	-11(ix), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	inc	hl
	ld	-4(ix), l
	ld	-3(ix), h
	add	hl, hl
	ld	bc, #_stk
	add	hl, bc
	ld	e, -12(ix)
	ld	d, -11(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_temp_reload_addr.s" -o "$TMPDIR/de_temp_reload_addr.out.s"
if grep -Eq -- '-1[12]\(ix\)' "$TMPDIR/de_temp_reload_addr.out.s"; then
    echo "xopt smoke: DE temp reload across address calc was not elided" >&2
    exit 1
fi
grep -q 'ld	(hl), e' "$TMPDIR/de_temp_reload_addr.out.s"

cat >"$TMPDIR/hl_temp_reload_addr.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=16, stack_params=0)
	add	hl, bc
	ld	-8(ix), l
	ld	-7(ix), h
	ld	l, -4(ix)
	ld	h, -3(ix)
	dec	hl
	add	hl, hl
	ld	de, #_stk
	add	hl, de
	ld	e, -8(ix)
	ld	d, -7(ix)
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	bc, #0
	ret
ASM

"$XOPT" -Os "$TMPDIR/hl_temp_reload_addr.s" -o "$TMPDIR/hl_temp_reload_addr.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/hl_temp_reload_addr.out.s"
grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_stk' "$TMPDIR/hl_temp_reload_addr.out.s"
if grep -Eq -- '-[78]\(ix\)' "$TMPDIR/hl_temp_reload_addr.out.s"; then
    echo "xopt smoke: HL temp reload across address calc was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/de_temp_to_hl_stack.s" <<'ASM'
_demo:
	; sdcccall(1) prologue: demo (locals=0, temp_frame=16, stack_params=0)
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	-8(ix), e
	ld	-7(ix), d
	ld	l, -4(ix)
	ld	h, -3(ix)
	add	hl, hl
	ld	bc, #_stk
	add	hl, bc
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	b, d
	ld	c, e
	ld	l, -8(ix)
	ld	h, -7(ix)
	add	hl, bc
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_temp_to_hl_stack.s" -o "$TMPDIR/de_temp_to_hl_stack.out.s"
grep -Eq 'push[[:space:]]+de' "$TMPDIR/de_temp_to_hl_stack.out.s"
grep -Eq 'pop[[:space:]]+hl' "$TMPDIR/de_temp_to_hl_stack.out.s"
if grep -Eq -- '-[78]\(ix\)' "$TMPDIR/de_temp_to_hl_stack.out.s"; then
    echo "xopt smoke: DE temp reload to HL was not stack-preserved" >&2
    exit 1
fi

cat >"$TMPDIR/signed_zero_branch_m.s" <<'ASM'
_demo:
	ld	de,#0
	or	a,a
	sbc	hl,de
	jp	m,_neg
	ld	de,#1
	xor	a
	ret
_neg:
	ld	de,#2
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/signed_zero_branch_m.s" -o "$TMPDIR/signed_zero_branch_m.out.s"
grep -Eq 'bit[[:space:]]+7,[[:space:]]*h' "$TMPDIR/signed_zero_branch_m.out.s"
grep -Eq 'j[pr][[:space:]]+nz,[[:space:]]*_neg' "$TMPDIR/signed_zero_branch_m.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#0|sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/signed_zero_branch_m.out.s"; then
    echo "xopt smoke: signed zero negative branch did not use high-bit test" >&2
    exit 1
fi

cat >"$TMPDIR/signed_zero_branch_p.s" <<'ASM'
_demo:
	ld	de,#0
	or	a,a
	sbc	hl,de
	jp	p,_nonneg
	ld	de,#1
	xor	a
	ret
_nonneg:
	ld	de,#2
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/signed_zero_branch_p.s" -o "$TMPDIR/signed_zero_branch_p.out.s"
grep -Eq 'bit[[:space:]]+7,[[:space:]]*h' "$TMPDIR/signed_zero_branch_p.out.s"
grep -Eq 'j[pr][[:space:]]+z,[[:space:]]*_nonneg' "$TMPDIR/signed_zero_branch_p.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#0|sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/signed_zero_branch_p.out.s"; then
    echo "xopt smoke: signed zero nonnegative branch did not use high-bit test" >&2
    exit 1
fi

cat >"$TMPDIR/page_bound_branch.s" <<'ASM'
_demo:
	ld	l,-3(ix)
	ld	h,-2(ix)
	ld	de,#1024
	or	a,a
	sbc	hl,de
	jp	nc,_done
	ld	de,#1
	xor	a
	ret
_done:
	ld	de,#2
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/page_bound_branch.s" -o "$TMPDIR/page_bound_branch.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-2\(ix\)' "$TMPDIR/page_bound_branch.out.s"
grep -Eq 'cp[[:space:]]+#4' "$TMPDIR/page_bound_branch.out.s"
grep -Eq 'j[pr][[:space:]]+nc,[[:space:]]*_done' "$TMPDIR/page_bound_branch.out.s"
if grep -Eq 'ld[[:space:]]+l,[[:space:]]*-3\(ix\)|ld[[:space:]]+de,[[:space:]]*#1024|sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/page_bound_branch.out.s"; then
    echo "xopt smoke: page-aligned unsigned bound branch did not use high-byte compare" >&2
    exit 1
fi

cat >"$TMPDIR/page_bound_branch_signed.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	de,#256
	or	a,a
	sbc	hl,de
	jp	p,_done
	ld	de,#1
	xor	a
	ret
_done:
	ld	de,#2
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/page_bound_branch_signed.s" -o "$TMPDIR/page_bound_branch_signed.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-3\(ix\)' "$TMPDIR/page_bound_branch_signed.out.s"
grep -Eq 'cp[[:space:]]+#1' "$TMPDIR/page_bound_branch_signed.out.s"
grep -Eq 'j[pr][[:space:]]+p,[[:space:]]*_done' "$TMPDIR/page_bound_branch_signed.out.s"
if grep -Eq 'ld[[:space:]]+l,[[:space:]]*-4\(ix\)|ld[[:space:]]+de,[[:space:]]*#256|sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/page_bound_branch_signed.out.s"; then
    echo "xopt smoke: signed page-aligned bound branch did not use high-byte compare" >&2
    exit 1
fi

cat >"$TMPDIR/page_bound_branch_de_return.s" <<'ASM'
_demo:
	ld	l,-3(ix)
	ld	h,-2(ix)
	ld	de,#1024
	or	a,a
	sbc	hl,de
	jp	nc,_ret
	ld	de,#0
	xor	a
	ret
_ret:
	ld	e,-5(ix)
	ld	d,-4(ix)
	ld	sp,ix
	pop	ix
	ret
ASM

"$XOPT" -Os "$TMPDIR/page_bound_branch_de_return.s" -o "$TMPDIR/page_bound_branch_de_return.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-2\(ix\)' "$TMPDIR/page_bound_branch_de_return.out.s"
grep -Eq 'cp[[:space:]]+#4' "$TMPDIR/page_bound_branch_de_return.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1024|sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/page_bound_branch_de_return.out.s"; then
    echo "xopt smoke: page-aligned bound did not fold before DE return tail" >&2
    exit 1
fi

cat >"$TMPDIR/page_bound_branch_a_live.s" <<'ASM'
_demo:
	ld	l,-3(ix)
	ld	h,-2(ix)
	ld	de,#1024
	or	a,a
	sbc	hl,de
	jp	nc,_done
	ld	e,a
	ret
_done:
	ld	e,a
	ret
ASM

"$XOPT" -Os "$TMPDIR/page_bound_branch_a_live.s" -o "$TMPDIR/page_bound_branch_a_live.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1024' "$TMPDIR/page_bound_branch_a_live.out.s"
grep -Eq 'sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/page_bound_branch_a_live.out.s"

cat >"$TMPDIR/redundant_u8_self_mask.s" <<'ASM'
_demo:
	ld	a,l
	and	#255
	ld	l,a
	ld	a,h
	and	#7
	ld	h,a
	ret
ASM

"$XOPT" -Os "$TMPDIR/redundant_u8_self_mask.s" -o "$TMPDIR/redundant_u8_self_mask.out.s"
grep -Eq 'and[[:space:]]+#7' "$TMPDIR/redundant_u8_self_mask.out.s"
if grep -Eq 'and[[:space:]]+#255|ld[[:space:]]+a,[[:space:]]*l|ld[[:space:]]+l,[[:space:]]*a' "$TMPDIR/redundant_u8_self_mask.out.s"; then
    echo "xopt smoke: redundant u8 self-mask was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/redundant_u8_self_mask_flags_live.s" <<'ASM'
_demo:
	ld	a,l
	and	#255
	ld	l,a
	jr	z,_done
	ld	a,h
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/redundant_u8_self_mask_flags_live.s" -o "$TMPDIR/redundant_u8_self_mask_flags_live.out.s"
grep -Eq 'and[[:space:]]+#255' "$TMPDIR/redundant_u8_self_mask_flags_live.out.s"
grep -Eq 'jr[[:space:]]+z,' "$TMPDIR/redundant_u8_self_mask_flags_live.out.s"

cat >"$TMPDIR/hl_bc_roundtrip.s" <<'ASM'
_demo:
	ld	hl,#1234
	ld	b,h
	ld	c,l
	ld	h,b
	ld	l,c
	ex	de,hl
	ld	b,#0
	ret
ASM

"$XOPT" -Os "$TMPDIR/hl_bc_roundtrip.s" -o "$TMPDIR/hl_bc_roundtrip.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/hl_bc_roundtrip.out.s" ||
   grep -Eq 'ld[[:space:]]+h,[[:space:]]*b' "$TMPDIR/hl_bc_roundtrip.out.s"; then
    echo "xopt smoke: dead HL->BC->HL roundtrip was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/hl_bc_roundtrip_live.s" <<'ASM'
_demo:
	ld	hl,#1234
	ld	b,h
	ld	c,l
	ld	h,b
	ld	l,c
	ld	a,c
	ret
ASM

"$XOPT" -Os "$TMPDIR/hl_bc_roundtrip_live.s" -o "$TMPDIR/hl_bc_roundtrip_live.out.s"
grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/hl_bc_roundtrip_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*c' "$TMPDIR/hl_bc_roundtrip_live.out.s"

cat >"$TMPDIR/bc_base_add.s" <<'ASM'
_demo:
	ld	l,-1(ix)
	ld	h,-2(ix)
	add	hl,hl
	ld	b,h
	ld	c,l
	ld	hl,#_arr
	add	hl,bc
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_base_add.s" -o "$TMPDIR/bc_base_add.out.s"
grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_arr' "$TMPDIR/bc_base_add.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/bc_base_add.out.s"; then
    echo "xopt smoke: BC base-add direct fold did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/bc_base_add_live.s" <<'ASM'
_demo:
	ld	l,-1(ix)
	ld	h,-2(ix)
	add	hl,hl
	ld	b,h
	ld	c,l
	ld	hl,#_arr
	add	hl,bc
	ld	a,c
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_base_add_live.s" -o "$TMPDIR/bc_base_add_live.out.s"
grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/bc_base_add_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*c' "$TMPDIR/bc_base_add_live.out.s"

cat >"$TMPDIR/bc_index_add_reload.s" <<'ASM'
_demo:
	ld	hl,#3
	ld	b,h
	ld	c,l
	ld	l,-1(ix)
	ld	h,-2(ix)
	add	hl,bc
	ld	de,#0
	ld	bc,#0
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_index_add_reload.s" -o "$TMPDIR/bc_index_add_reload.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#3|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/bc_index_add_reload.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/bc_index_add_reload.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/bc_index_add_reload.out.s"; then
    echo "xopt smoke: BC index add reload did not switch to DE" >&2
    exit 1
fi

cat >"$TMPDIR/bc_index_add_reload_de_live.s" <<'ASM'
_demo:
	ld	hl,#3
	ld	b,h
	ld	c,l
	ld	l,-1(ix)
	ld	h,-2(ix)
	add	hl,bc
	ld	a,e
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_index_add_reload_de_live.s" -o "$TMPDIR/bc_index_add_reload_de_live.out.s"
grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/bc_index_add_reload_de_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*e' "$TMPDIR/bc_index_add_reload_de_live.out.s"

cat >"$TMPDIR/bc_saved_hl_push_word_to_de_direct.s" <<'ASM'
_demo:
	ld	l,-1(ix)
	ld	h,-2(ix)
	or	a,a
	sbc	hl,de
	ld	b,h
	ld	c,l
	ld	l,-17(ix)
	ld	h,-16(ix)
	push	hl
	ld	h,b
	ld	l,c
	pop	de
	or	a,a
	sbc	hl,de
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_saved_hl_push_word_to_de_direct.s" -o "$TMPDIR/bc_saved_hl_push_word_to_de_direct.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-17\(ix\)' "$TMPDIR/bc_saved_hl_push_word_to_de_direct.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-16\(ix\)' "$TMPDIR/bc_saved_hl_push_word_to_de_direct.out.s"
if grep -Eq 'push[[:space:]]+hl|pop[[:space:]]+de' "$TMPDIR/bc_saved_hl_push_word_to_de_direct.out.s"; then
    echo "xopt smoke: BC-saved HL stack word to DE direct rewrite did not fire" >&2
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

cat >"$TMPDIR/de_result_hl_forward_label_overwrite.s" <<'ASM'
_demo:
	call	_r
	push	de
	pop	hl
	ld	b,h
	ld	c,l
	ld	-2(ix),l
	ld	-1(ix),h
_fallthrough:
	ld	hl,#0
	inc	-3(ix)
	call	_next
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_result_hl_forward_label_overwrite.s" -o "$TMPDIR/de_result_hl_forward_label_overwrite.out.s"
grep -Eq 'ld[[:space:]]+-2\(ix\),[[:space:]]*e' "$TMPDIR/de_result_hl_forward_label_overwrite.out.s"
grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*d' "$TMPDIR/de_result_hl_forward_label_overwrite.out.s"
if grep -Eq 'push[[:space:]]+de|pop[[:space:]]+hl' "$TMPDIR/de_result_hl_forward_label_overwrite.out.s"; then
    echo "xopt smoke: DE result through fallthrough label was not forwarded" >&2
    exit 1
fi

cat >"$TMPDIR/legacy_de_result_return.s" <<'ASM'
_legacy_de_result_return:
	; sdcccall(0) prologue: legacy_de_result_return (locals=0, temp_frame=0, stack_params=0)
	call	_modern_result
	push	de
	pop	hl
	ld	b,h
	ld	c,l
	; epilogue: legacy_de_result_return
	jp	__sdcc_leave_ix
ASM

"$XOPT" -Os "$TMPDIR/legacy_de_result_return.s" -o "$TMPDIR/legacy_de_result_return.out.s"
if ! grep -Eq 'pop[[:space:]]+hl|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/legacy_de_result_return.out.s"; then
    echo "xopt smoke: legacy HL return conversion was discarded" >&2
    exit 1
fi

cat >"$TMPDIR/de_result_hl_forward_call_arg_guard.s" <<'ASM'
_demo:
	call	_r
	push	de
	pop	hl
	ld	b,h
	ld	c,l
	ld	-2(ix),l
	ld	-1(ix),h
_fallthrough:
	inc	-3(ix)
	call	_next
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_result_hl_forward_call_arg_guard.s" -o "$TMPDIR/de_result_hl_forward_call_arg_guard.out.s"
grep -Eq 'push[[:space:]]+de' "$TMPDIR/de_result_hl_forward_call_arg_guard.out.s"
grep -Eq 'pop[[:space:]]+hl' "$TMPDIR/de_result_hl_forward_call_arg_guard.out.s"

cat >"$TMPDIR/adjacent_indexed_byte_stores_postinc.s" <<'ASM'
_demo:
	ld	l,-5(ix)
	ld	h,-4(ix)
	inc	hl
	ld	-11(ix),l
	ld	-10(ix),h
	ld	hl,#_outb
	ld	e,-5(ix)
	ld	d,-4(ix)
	add	hl,de
	ld	(hl),a
	ld	l,-11(ix)
	ld	h,-10(ix)
	inc	hl
	ld	-5(ix),l
	ld	-4(ix),h
	ld	hl,#_outb
	ld	e,-11(ix)
	ld	d,-10(ix)
	add	hl,de
	ld	a,-1(ix)
	ld	(hl),a
	ld	hl,#0
	ld	de,#0
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/adjacent_indexed_byte_stores_postinc.s" -o "$TMPDIR/adjacent_indexed_byte_stores_postinc.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#_outb' "$TMPDIR/adjacent_indexed_byte_stores_postinc.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-1\(ix\)' "$TMPDIR/adjacent_indexed_byte_stores_postinc.out.s"
if grep -Eq -- '-11\(ix\)|-10\(ix\)' "$TMPDIR/adjacent_indexed_byte_stores_postinc.out.s"; then
    echo "xopt smoke: adjacent indexed byte stores still use the temporary index" >&2
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

cat >"$TMPDIR/push_de_pop_hl_to_ex.s" <<'ASM'
_push_de_pop_hl_to_ex:
	push	de
	pop	hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	de, #12
	or	a, a
	sbc	hl, de
	ret
ASM

"$XOPT" -Os "$TMPDIR/push_de_pop_hl_to_ex.s" -o "$TMPDIR/push_de_pop_hl_to_ex.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/push_de_pop_hl_to_ex.out.s"
if grep -Eq 'push[[:space:]]+de|pop[[:space:]]+hl' "$TMPDIR/push_de_pop_hl_to_ex.out.s"; then
    echo "xopt smoke: push DE/pop HL was not exchanged when DE died" >&2
    exit 1
fi

cat >"$TMPDIR/push_de_pop_hl_de_live.s" <<'ASM'
_push_de_pop_hl_de_live:
	push	de
	pop	hl
	ld	a, e
	ret
ASM

"$XOPT" -Os "$TMPDIR/push_de_pop_hl_de_live.s" -o "$TMPDIR/push_de_pop_hl_de_live.out.s"
grep -Eq 'push[[:space:]]+de' "$TMPDIR/push_de_pop_hl_de_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*e' "$TMPDIR/push_de_pop_hl_de_live.out.s"

cat >"$TMPDIR/pop_bc_run_sp_adjust.s" <<'ASM'
_pop_bc_run_sp_adjust:
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	ld	l, -4(ix)
	ld	h, -3(ix)
	or	a, a
	ret
ASM

"$XOPT" -Os "$TMPDIR/pop_bc_run_sp_adjust.s" -o "$TMPDIR/pop_bc_run_sp_adjust.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#12' "$TMPDIR/pop_bc_run_sp_adjust.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*sp' "$TMPDIR/pop_bc_run_sp_adjust.out.s"
grep -Eq 'ld[[:space:]]+sp,[[:space:]]*hl' "$TMPDIR/pop_bc_run_sp_adjust.out.s"
if grep -Eq 'pop[[:space:]]+bc' "$TMPDIR/pop_bc_run_sp_adjust.out.s"; then
    echo "xopt smoke: long pop BC run was not converted to SP adjust" >&2
    exit 1
fi

cat >"$TMPDIR/pop_bc_run_sp_adjust_flags_live.s" <<'ASM'
_pop_bc_run_sp_adjust_flags_live:
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	pop	bc
	jr	c, _done
	ld	hl, #0
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/pop_bc_run_sp_adjust_flags_live.s" -o "$TMPDIR/pop_bc_run_sp_adjust_flags_live.out.s"
grep -Eq 'pop[[:space:]]+bc' "$TMPDIR/pop_bc_run_sp_adjust_flags_live.out.s"
grep -Eq 'jr[[:space:]]+c,' "$TMPDIR/pop_bc_run_sp_adjust_flags_live.out.s"

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
grep -q 'inc	-3(ix)' "$TMPDIR/ix_word_inc_direct.out.s"
if grep -q 'inc	hl' "$TMPDIR/ix_word_inc_direct.out.s"; then
    echo "xopt smoke: IX word increment rewrite did not fire" >&2
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

cat >"$TMPDIR/ix_word_inc_hl_live.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	inc	hl
	ld	-4(ix),l
	ld	-3(ix),h
	ld	de,#3
	or	a
	sbc	hl,de
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_word_inc_hl_live.s" -o "$TMPDIR/ix_word_inc_hl_live.out.s"
grep -q 'inc	hl' "$TMPDIR/ix_word_inc_hl_live.out.s"
grep -Eq 'sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/ix_word_inc_hl_live.out.s"
if grep -q 'inc	-4(ix)' "$TMPDIR/ix_word_inc_hl_live.out.s"; then
    echo "xopt smoke: direct IX word increment clobbered live HL" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_add1_direct.s" <<'ASM'
_demo:
	ld	l,-6(ix)
	ld	h,-5(ix)
	ld	de,#1
	add	hl,de
	ld	-6(ix),l
	ld	-5(ix),h
	ld	de,#7
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_word_add1_direct.s" -o "$TMPDIR/ix_word_add1_direct.out.s"
grep -q 'inc	-6(ix)' "$TMPDIR/ix_word_add1_direct.out.s"
grep -q 'inc	-5(ix)' "$TMPDIR/ix_word_add1_direct.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1|add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/ix_word_add1_direct.out.s"; then
    echo "xopt smoke: IX word add-one rewrite did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_add1_de_live.s" <<'ASM'
_demo:
	ld	l,-6(ix)
	ld	h,-5(ix)
	ld	de,#1
	add	hl,de
	ld	-6(ix),l
	ld	-5(ix),h
	ld	a,e
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_word_add1_de_live.s" -o "$TMPDIR/ix_word_add1_de_live.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1' "$TMPDIR/ix_word_add1_de_live.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/ix_word_add1_de_live.out.s"
if grep -q 'inc	-6(ix)' "$TMPDIR/ix_word_add1_de_live.out.s"; then
    echo "xopt smoke: IX word add-one rewrite clobbered live DE" >&2
    exit 1
fi

cat >"$TMPDIR/add_hl_de_one_to_inc.s" <<'ASM'
_demo:
	ld	de,#1
	add	hl,de
	ld	de,#2
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/add_hl_de_one_to_inc.s" -o "$TMPDIR/add_hl_de_one_to_inc.out.s"
grep -q 'inc	hl' "$TMPDIR/add_hl_de_one_to_inc.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1|add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/add_hl_de_one_to_inc.out.s"; then
    echo "xopt smoke: add-hl-de-one fold did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/add_hl_de_one_de_live.s" <<'ASM'
_demo:
	ld	de,#1
	add	hl,de
	ld	a,e
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/add_hl_de_one_de_live.s" -o "$TMPDIR/add_hl_de_one_de_live.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1' "$TMPDIR/add_hl_de_one_de_live.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/add_hl_de_one_de_live.out.s"
if grep -q 'inc	hl' "$TMPDIR/add_hl_de_one_de_live.out.s"; then
    echo "xopt smoke: add-hl-de-one fold clobbered live DE" >&2
    exit 1
fi

cat >"$TMPDIR/add_hl_de_one_flags_live.s" <<'ASM'
_demo:
	ld	de,#1
	add	hl,de
	jr	c,_done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/add_hl_de_one_flags_live.s" -o "$TMPDIR/add_hl_de_one_flags_live.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#1' "$TMPDIR/add_hl_de_one_flags_live.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/add_hl_de_one_flags_live.out.s"
if grep -q 'inc	hl' "$TMPDIR/add_hl_de_one_flags_live.out.s"; then
    echo "xopt smoke: add-hl-de-one fold clobbered live flags" >&2
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
grep -Eq '^[[:space:]]+inc[[:space:]]+a' "$TMPDIR/ix_byte_inc_a_live.out.s"
if grep -q 'add	a,#1' "$TMPDIR/ix_byte_inc_a_live.out.s"; then
    echo "xopt smoke: A-live byte increment kept larger ADD encoding" >&2
    exit 1
fi
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

"$XOPT" -Os "$TMPDIR/ix_byte_load_forward.s" -o "$TMPDIR/ix_byte_load_forward.os.s"
grep -q 'ld	c, -4(ix)' "$TMPDIR/ix_byte_load_forward.os.s"
if grep -q 'ld	a,-4(ix)' "$TMPDIR/ix_byte_load_forward.os.s"; then
    echo "xopt smoke: -Os IX byte load forwarding did not fire" >&2
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

cat >"$TMPDIR/ix_imm_store_direct.s" <<'ASM'
_demo:
	ld	a,#42
	ld	-4(ix),a
	ld	a,#7
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_imm_store_direct.s" -o "$TMPDIR/ix_imm_store_direct.os.s"
grep -q 'ld	-4(ix), #42' "$TMPDIR/ix_imm_store_direct.os.s"
if grep -q 'ld	a,#42' "$TMPDIR/ix_imm_store_direct.os.s"; then
    echo "xopt smoke: -Os IX immediate store forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_imm_store_direct_a_live.s" <<'ASM'
_demo:
	ld	a,#42
	ld	-4(ix),a
	ld	b,a
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_imm_store_direct_a_live.s" -o "$TMPDIR/ix_imm_store_direct_a_live.os.s"
grep -q 'ld	a,#42' "$TMPDIR/ix_imm_store_direct_a_live.os.s"
if grep -q 'ld	-4(ix), #42' "$TMPDIR/ix_imm_store_direct_a_live.os.s"; then
    echo "xopt smoke: -Os IX immediate store forwarding clobbered live A" >&2
    exit 1
fi

cat >"$TMPDIR/de_spill_reload_exchange_dead_hl.s" <<'ASM'
_demo:
	ld	-5(ix),e
	ld	-4(ix),d
	ld	l,-5(ix)
	ld	h,-4(ix)
	ex	de,hl
	ld	l,-9(ix)
	ld	h,-8(ix)
	or	a,a
	sbc	hl,de
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_spill_reload_exchange_dead_hl.s" -o "$TMPDIR/de_spill_reload_exchange_dead_hl.os.s"
grep -q 'ld	-5(ix),e' "$TMPDIR/de_spill_reload_exchange_dead_hl.os.s"
grep -Eq 'sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/de_spill_reload_exchange_dead_hl.os.s"
if grep -Eq 'ld[[:space:]]+l,[[:space:]]*-5\(ix\)|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/de_spill_reload_exchange_dead_hl.os.s"; then
    echo "xopt smoke: DE spill/reload exchange cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/de_spill_reload_exchange_hl_live.s" <<'ASM'
_demo:
	ld	-5(ix),e
	ld	-4(ix),d
	ld	l,-5(ix)
	ld	h,-4(ix)
	ex	de,hl
	ld	a,l
	ret
ASM

"$XOPT" -Os "$TMPDIR/de_spill_reload_exchange_hl_live.s" -o "$TMPDIR/de_spill_reload_exchange_hl_live.os.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-5\(ix\)' "$TMPDIR/de_spill_reload_exchange_hl_live.os.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/de_spill_reload_exchange_hl_live.os.s"; then
    echo "xopt smoke: equal DE/HL spill reload kept a no-op exchange" >&2
    exit 1
fi

cat >"$TMPDIR/hl_to_de_before_hl_reload.s" <<'ASM'
_hl_to_de_before_hl_reload:
	ld	d, h
	ld	e, l
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	_use
	ret
ASM

"$XOPT" -Os "$TMPDIR/hl_to_de_before_hl_reload.s" -o "$TMPDIR/hl_to_de_before_hl_reload.os.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/hl_to_de_before_hl_reload.os.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-4\(ix\)' "$TMPDIR/hl_to_de_before_hl_reload.os.s"
if grep -Eq 'ld[[:space:]]+d,[[:space:]]*h|ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/hl_to_de_before_hl_reload.os.s"; then
    echo "xopt smoke: HL->DE copy before HL reload was not exchanged" >&2
    exit 1
fi

cat >"$TMPDIR/hl_to_de_before_hl_reload_live.s" <<'ASM'
_hl_to_de_before_hl_reload_live:
	ld	d, h
	ld	e, l
	ld	a, l
	call	_use
	ret
ASM

"$XOPT" -Os "$TMPDIR/hl_to_de_before_hl_reload_live.s" -o "$TMPDIR/hl_to_de_before_hl_reload_live.os.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*h' "$TMPDIR/hl_to_de_before_hl_reload_live.os.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*l' "$TMPDIR/hl_to_de_before_hl_reload_live.os.s"

cat >"$TMPDIR/pair_copy_offset_materialize.s" <<'ASM'
_bc_offset:
	ld	h,b
	ld	l,c
	inc	hl
	inc	hl
	inc	hl
	inc	hl
	ld	a,(hl)
	or	a,a
	ret
_de_offset:
	ld	l,e
	ld	h,d
	dec	hl
	dec	hl
	dec	hl
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_copy_offset_materialize.s" -o "$TMPDIR/pair_copy_offset_materialize.os.s"
grep -q 'ld	hl, #4' "$TMPDIR/pair_copy_offset_materialize.os.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*bc' "$TMPDIR/pair_copy_offset_materialize.os.s"
grep -q 'ld	hl, #65533' "$TMPDIR/pair_copy_offset_materialize.os.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/pair_copy_offset_materialize.os.s"
if grep -Eq 'inc[[:space:]]+hl|dec[[:space:]]+hl' "$TMPDIR/pair_copy_offset_materialize.os.s"; then
    echo "xopt smoke: pair-copy offset materialization did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/pair_copy_offset_flags_live.s" <<'ASM'
_demo:
	ld	h,b
	ld	l,c
	inc	hl
	inc	hl
	inc	hl
	jr	c,_done
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_copy_offset_flags_live.s" -o "$TMPDIR/pair_copy_offset_flags_live.os.s"
grep -Eq 'inc[[:space:]]+hl' "$TMPDIR/pair_copy_offset_flags_live.os.s"
if grep -Eq 'add[[:space:]]+hl,[[:space:]]*bc' "$TMPDIR/pair_copy_offset_flags_live.os.s"; then
    echo "xopt smoke: pair-copy offset materialization clobbered live flags" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_alu_forward.s" <<'ASM'
_ix_byte_alu_forward:
	ld	e, -4(ix)
	ld	d, -5(ix)
	ld	a, e
	add	a, d
	ld	d, #0
	ld	e, #0
	ret
_ix_byte_xor_forward:
	ld	e, -6(ix)
	ld	d, -7(ix)
	ld	a, e
	xor	a, d
	ld	e, #1
	ld	d, #2
	ret
_ix_byte_a_xor_forward:
	ld	e, a
	ld	d, -8(ix)
	ld	a, e
	xor	a, d
	ld	e, #3
	ld	d, #4
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_alu_forward.s" -o "$TMPDIR/ix_byte_alu_forward.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-4\(ix\)' "$TMPDIR/ix_byte_alu_forward.out.s"
grep -Eq 'add[[:space:]]+a,[[:space:]]*-5\(ix\)' "$TMPDIR/ix_byte_alu_forward.out.s"
grep -Eq 'xor[[:space:]]+(a,[[:space:]]*)?-7\(ix\)' "$TMPDIR/ix_byte_alu_forward.out.s"
grep -Eq 'xor[[:space:]]+(a,[[:space:]]*)?-8\(ix\)' "$TMPDIR/ix_byte_alu_forward.out.s"
if grep -Eq 'ld[[:space:]]+e,[[:space:]]*(-[46]\(ix\)|a)|ld[[:space:]]+d,[[:space:]]*-[578]\(ix\)' "$TMPDIR/ix_byte_alu_forward.out.s"; then
    echo "xopt smoke: IX byte ALU forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_byte_alu_forward_de_live.s" <<'ASM'
_ix_byte_alu_forward_de_live:
	ld	e, -4(ix)
	ld	d, -5(ix)
	ld	a, e
	add	a, d
	ld	h, d
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_byte_alu_forward_de_live.s" -o "$TMPDIR/ix_byte_alu_forward_de_live.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-4\(ix\)' "$TMPDIR/ix_byte_alu_forward_de_live.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-5\(ix\)' "$TMPDIR/ix_byte_alu_forward_de_live.out.s"

cat >"$TMPDIR/ix_byte_store_forward.s" <<'ASM'
_demo:
	ld	-5(ix),a
	ld	e,-5(ix)
	ret
ASM

"$XOPT" -O2 "$TMPDIR/ix_byte_store_forward.s" -o "$TMPDIR/ix_byte_store_forward.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*a' "$TMPDIR/ix_byte_store_forward.out.s"
if grep -Eq 'ld[[:space:]]+e,[[:space:]]*-5\\(ix\\)' "$TMPDIR/ix_byte_store_forward.out.s"; then
    echo "xopt smoke: IX byte store forwarding did not fire" >&2
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

"$XOPT" -Os "$TMPDIR/compare_fallthrough_reload.s" -o "$TMPDIR/compare_fallthrough_reload.out.s"
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

"$XOPT" -Os "$TMPDIR/compare_fallthrough_reload_referenced.s" -o "$TMPDIR/compare_fallthrough_reload_referenced.out.s"
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

"$XOPT" -Os "$TMPDIR/xor_compare_fallthrough_reload.s" -o "$TMPDIR/xor_compare_fallthrough_reload.out.s"
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

"$XOPT" -Os "$TMPDIR/xor_compare_fallthrough_reload_referenced.s" -o "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s"
if [[ "$(grep -c 'ld	a,-4(ix)' "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s")" != "2" ]]; then
    echo "xopt smoke: transformed compare reload cleanup crossed a referenced label" >&2
    exit 1
fi
if [[ "$(grep -c 'xor	#128' "$TMPDIR/xor_compare_fallthrough_reload_referenced.out.s")" != "2" ]]; then
    echo "xopt smoke: transformed compare reload cleanup crossed a referenced label transform" >&2
    exit 1
fi

cat >"$TMPDIR/a_temp_branch_reload.s" <<'ASM'
_a_temp_branch_reload:
	; sdcccall(1) prologue: a_temp_branch_reload (locals=1, temp_frame=2, stack_params=0)
	ld	a,-1(ix)
	ld	-2(ix),a
	or	a,a
	jr	z,_done
_fall:
	ld	a,-2(ix)
	cp	#37
	jr	z,_done
_next:
	ld	a,-2(ix)
	cp	#45
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/a_temp_branch_reload.s" -o "$TMPDIR/a_temp_branch_reload.out.s"
if grep -Eq -- '-2\(ix\)' "$TMPDIR/a_temp_branch_reload.out.s"; then
    echo "xopt smoke: A temp branch reload/store was not elided" >&2
    exit 1
fi
grep -Eq 'cp[[:space:]]+#37' "$TMPDIR/a_temp_branch_reload.out.s"
grep -Eq 'cp[[:space:]]+#45' "$TMPDIR/a_temp_branch_reload.out.s"

cat >"$TMPDIR/a_temp_branch_target_reload.s" <<'ASM'
_a_temp_branch_target_reload:
	; sdcccall(1) prologue: a_temp_branch_target_reload (locals=1, temp_frame=2, stack_params=0)
	ld	a,-1(ix)
	ld	-2(ix),a
	cp	#48
	jr	nz,_target
	inc	-1(ix)
	jr	_done
_target:
	ld	a,-2(ix)
	cp	#42
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/a_temp_branch_target_reload.s" -o "$TMPDIR/a_temp_branch_target_reload.out.s"
if grep -Eq -- '-2\(ix\)' "$TMPDIR/a_temp_branch_target_reload.out.s"; then
    echo "xopt smoke: A temp branch-target reload/store was not elided" >&2
    exit 1
fi
grep -Eq 'cp[[:space:]]+#42' "$TMPDIR/a_temp_branch_target_reload.out.s"

cat >"$TMPDIR/a_temp_branch_target_referenced.s" <<'ASM'
_a_temp_branch_target_referenced:
	; sdcccall(1) prologue: a_temp_branch_target_referenced (locals=1, temp_frame=2, stack_params=0)
	ld	a,-1(ix)
	ld	-2(ix),a
	cp	#48
	jr	nz,_target
	inc	-1(ix)
	jr	_done
_target:
	ld	a,-2(ix)
	cp	#42
_done:
	xor	a
	jp	_target
ASM

"$XOPT" -Os "$TMPDIR/a_temp_branch_target_referenced.s" -o "$TMPDIR/a_temp_branch_target_referenced.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-2\(ix\)' "$TMPDIR/a_temp_branch_target_referenced.out.s"

cat >"$TMPDIR/a_local_branch_reload_live.s" <<'ASM'
_a_local_branch_reload_live:
	; sdcccall(1) prologue: a_local_branch_reload_live (locals=2, temp_frame=0, stack_params=0)
	ld	a,-1(ix)
	ld	-2(ix),a
	or	a,a
	jr	z,_done
_fall:
	ld	a,-2(ix)
	cp	#37
_done:
	ld	l,-2(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/a_local_branch_reload_live.s" -o "$TMPDIR/a_local_branch_reload_live.out.s"
grep -Eq 'ld[[:space:]]+-2\(ix\),[[:space:]]*a' "$TMPDIR/a_local_branch_reload_live.out.s"
if grep -Eq 'ld[[:space:]]+a,[[:space:]]*-2\(ix\)' "$TMPDIR/a_local_branch_reload_live.out.s"; then
    echo "xopt smoke: A local branch reload was not forwarded through A" >&2
    exit 1
fi
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-2\(ix\)' "$TMPDIR/a_local_branch_reload_live.out.s"

cat >"$TMPDIR/a_local_branch_reload_dead.s" <<'ASM'
_a_local_branch_reload_dead:
	; sdcccall(1) prologue: a_local_branch_reload_dead (locals=2, temp_frame=0, stack_params=0)
	ld	a,-1(ix)
	ld	-2(ix),a
	or	a,a
	jr	z,_done
_fall:
	ld	a,-2(ix)
	cp	#37
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/a_local_branch_reload_dead.s" -o "$TMPDIR/a_local_branch_reload_dead.out.s"
grep -Eq 'ld[[:space:]]+-2\(ix\),[[:space:]]*a' "$TMPDIR/a_local_branch_reload_dead.out.s"
if grep -Eq 'ld[[:space:]]+a,[[:space:]]*-2\(ix\)' "$TMPDIR/a_local_branch_reload_dead.out.s"; then
    echo "xopt smoke: A local branch reload was not forwarded" >&2
    exit 1
fi
grep -Eq 'cp[[:space:]]+#37' "$TMPDIR/a_local_branch_reload_dead.out.s"

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
grep -q 'ld	c, -4(ix)' "$TMPDIR/zero_extend_src_to_bc.os.s"
grep -q 'ld	b, #0' "$TMPDIR/zero_extend_src_to_bc.os.s"
if grep -q 'ld	l,-4(ix)' "$TMPDIR/zero_extend_src_to_bc.os.s"; then
    echo "xopt smoke: -Os zero-extend to BC did not remove L load" >&2
    exit 1
fi
if grep -q 'ld	h,#0' "$TMPDIR/zero_extend_src_to_bc.os.s"; then
    echo "xopt smoke: -Os zero-extend to BC did not remove H zero" >&2
    exit 1
fi

cat >"$TMPDIR/dead_bc_zero_extend_from_a.s" <<'ASM'
_demo:
	ld	c,a
	ld	b,#0
	ld	hl,#19
	add	hl,sp
	ld	e,a
	ld	d,#0
	add	hl,de
	ld	(hl),#46
	ld	c,#0
	ld	b,#0
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_bc_zero_extend_from_a.s" -o "$TMPDIR/dead_bc_zero_extend_from_a.out.s"
if grep -Eq 'ld[[:space:]]+c,[[:space:]]*a' "$TMPDIR/dead_bc_zero_extend_from_a.out.s"; then
    echo "xopt smoke: dead BC zero-extend from A was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/dead_bc_zero_extend_from_a_live.s" <<'ASM'
_demo:
	ld	c,a
	ld	b,#0
	add	hl,bc
	ld	e,a
	ld	d,#0
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_bc_zero_extend_from_a_live.s" -o "$TMPDIR/dead_bc_zero_extend_from_a_live.out.s"
grep -Eq 'ld[[:space:]]+c,[[:space:]]*a' "$TMPDIR/dead_bc_zero_extend_from_a_live.out.s"

cat >"$TMPDIR/dead_bc_hl_to_de_call.s" <<'ASM'
_demo:
	ld	b,h
	ld	c,l
	ld	d,b
	ld	e,c
	ld	l,-2(ix)
	ld	h,-1(ix)
	call	_use_de_hl
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_bc_hl_to_de_call.s" -o "$TMPDIR/dead_bc_hl_to_de_call.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl|ld[[:space:]]+d,[[:space:]]*h' "$TMPDIR/dead_bc_hl_to_de_call.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/dead_bc_hl_to_de_call.out.s"; then
    echo "xopt smoke: dead BC middle copy before call was not folded" >&2
    exit 1
fi

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
grep -q 'ld	a, l' "$TMPDIR/zero_extend_pair_test.os.s"
grep -q 'or	a, a' "$TMPDIR/zero_extend_pair_test.os.s"
if grep -q 'ld	h,b' "$TMPDIR/zero_extend_pair_test.os.s"; then
    echo "xopt smoke: -Os zero-extend pair test kept copy-back high byte" >&2
    exit 1
fi
if grep -q 'or	a,l' "$TMPDIR/zero_extend_pair_test.os.s"; then
    echo "xopt smoke: -Os zero-extend pair test kept wide OR test" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_zero_fallthrough_reload.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,h
	or	a,l
	jr	z,_done
_fall:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,(hl)
_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_word_zero_fallthrough_reload.s" -o "$TMPDIR/ix_word_zero_fallthrough_reload.os.s"
grep -q 'ld	a, -3(ix)' "$TMPDIR/ix_word_zero_fallthrough_reload.os.s"
grep -q 'or	a, -4(ix)' "$TMPDIR/ix_word_zero_fallthrough_reload.os.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-4\(ix\)' "$TMPDIR/ix_word_zero_fallthrough_reload.os.s"
if grep -q 'ld	a, h' "$TMPDIR/ix_word_zero_fallthrough_reload.os.s"; then
    echo "xopt smoke: IX zero-test fallthrough reload cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_zero_target_reload.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,h
	or	a,l
	jr	nz,_use
	ld	de,#0
	ret
_use:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,(hl)
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_word_zero_target_reload.s" -o "$TMPDIR/ix_word_zero_target_reload.os.s"
grep -q 'ld	a, -3(ix)' "$TMPDIR/ix_word_zero_target_reload.os.s"
grep -q 'or	a, -4(ix)' "$TMPDIR/ix_word_zero_target_reload.os.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-4\(ix\)' "$TMPDIR/ix_word_zero_target_reload.os.s"
if grep -q 'ld	a, h' "$TMPDIR/ix_word_zero_target_reload.os.s"; then
    echo "xopt smoke: IX zero-test target reload cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/ix_word_zero_leave_ix_tail.s" <<'ASM'
_demo:
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,h
	or	a,l
	jr	z,_done
	ld	l,-4(ix)
	ld	h,-3(ix)
	ld	a,(hl)
_done:
	.globl	__sdcc_leave_ix
	jp	__sdcc_leave_ix
ASM

"$XOPT" -Os "$TMPDIR/ix_word_zero_leave_ix_tail.s" -o "$TMPDIR/ix_word_zero_leave_ix_tail.os.s"
grep -q 'ld	a, -3(ix)' "$TMPDIR/ix_word_zero_leave_ix_tail.os.s"
grep -q 'or	a, -4(ix)' "$TMPDIR/ix_word_zero_leave_ix_tail.os.s"
if grep -q 'ld	a, h' "$TMPDIR/ix_word_zero_leave_ix_tail.os.s"; then
    echo "xopt smoke: IX zero-test leave-IX tail cleanup did not fire" >&2
    exit 1
fi

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
if grep -Eq '^[[:space:]]+exx([[:space:]]|$)' "$TMPDIR/exx_cancel.os.s"; then
    echo "xopt smoke: adjacent exx pair was not cancelled in -Os" >&2
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
grep -Eq 'push[[:space:]]+de' "$TMPDIR/call_arg_de_same.os.s"
grep -q 'ld	de, #0' "$TMPDIR/call_arg_de_same.os.s"
if grep -Eq 'push[[:space:]]+hl|ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/call_arg_de_same.os.s"; then
    echo "xopt smoke: -Os same-immediate call argument kept old HL path" >&2
    exit 1
fi

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
grep -q 'ld	de, #9' "$TMPDIR/dead_hl_exchange.os.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/dead_hl_exchange.os.s"; then
    echo "xopt smoke: -Os dead-HL exchange did not become direct DE load" >&2
    exit 1
fi

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
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/equal_de_hl_exchange.os.s"; then
    echo "xopt smoke: -Os equal DE/HL exchange cleanup kept no-op exchange" >&2
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
grep -Eq '^[[:space:]]+ld[[:space:]]+de, #1234' "$TMPDIR/exchange_sandwich_de_load.os.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/exchange_sandwich_de_load.os.s"; then
    echo "xopt smoke: -Os exchange sandwich did not become direct DE load" >&2
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
grep -Eq '^[[:space:]]+pop[[:space:]]+bc' "$TMPDIR/dead_bc_stack_discard.os.s"
if grep -Eq '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/dead_bc_stack_discard.os.s"; then
    echo "xopt smoke: -Os dead-BC stack discard did not become pop bc" >&2
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
if grep -Eq '^[[:space:]]+(pop|push)[[:space:]]+hl' "$TMPDIR/dead_pair_pop_push.os.s"; then
    echo "xopt smoke: -Os dead pair pop/push cleanup did not fire" >&2
    exit 1
fi

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
if [[ "$(grep -Ec '^[[:space:]]+pop[[:space:]]+bc' "$TMPDIR/long_inc_sp.os.s")" != "4" ]]; then
    echo "xopt smoke: -Os long stack discard did not use compact dead pops" >&2
    exit 1
fi
if grep -Eq '^[[:space:]]+inc[[:space:]]+sp' "$TMPDIR/long_inc_sp.os.s"; then
    echo "xopt smoke: -Os long stack discard kept inc-sp instructions" >&2
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
if [[ "$(grep -Ec '^[[:space:]]+pop[[:space:]]+hl' "$TMPDIR/long_inc_sp_live_flags.out.s")" != "3" ]]; then
    echo "xopt smoke: live-flag stack discard did not use flag-preserving pops" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+jr[[:space:]]+c,' "$TMPDIR/long_inc_sp_live_flags.out.s"
if grep -Eq '^[[:space:]]+(inc[[:space:]]+sp|add[[:space:]]+hl,sp)' "$TMPDIR/long_inc_sp_live_flags.out.s"; then
    echo "xopt smoke: live-flag stack discard kept a larger or flag-clobbering form" >&2
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

cat >"$TMPDIR/return_copy_direct.s" <<'ASM'
_imm_return:
	ld	hl, #4660
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
_imm_return_end:
	ret
_mem_return:
	ld	hl, (#65296)
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
_mem_return_end:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/return_copy_direct.s" -o "$TMPDIR/return_copy_direct.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#4660' "$TMPDIR/return_copy_direct.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*\(#65296\)' "$TMPDIR/return_copy_direct.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/return_copy_direct.out.s"; then
    echo "xopt smoke: HL via BC return copy did not collapse" >&2
    exit 1
fi

cat >"$TMPDIR/ix_return_direct.s" <<'ASM'
_slot_return:
	ld	l, -8(ix)
	ld	h, -7(ix)
	ld	d, h
	ld	e, l
_slot_return_end:
	ld	sp, ix
	pop	ix
	ret
ASM

"$XOPT" -O3 "$TMPDIR/ix_return_direct.s" -o "$TMPDIR/ix_return_direct.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-8\(ix\)' "$TMPDIR/ix_return_direct.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-7\(ix\)' "$TMPDIR/ix_return_direct.out.s"
if grep -Eq 'ld[[:space:]]+l,[[:space:]]*-8\(ix\)|ld[[:space:]]+h,[[:space:]]*-7\(ix\)|ld[[:space:]]+d,[[:space:]]*h|ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/ix_return_direct.out.s"; then
    echo "xopt smoke: IX word return load did not collapse to DE" >&2
    exit 1
fi

cat >"$TMPDIR/lowbyte_sum_return.s" <<'ASM'
_byte_tail:
	ld	b, #0
	ld	hl, #305
	add	hl, bc
	add	hl, de
	ld	a, l
	ret
ASM

"$XOPT" -O3 "$TMPDIR/lowbyte_sum_return.s" -o "$TMPDIR/lowbyte_sum_return.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*e' "$TMPDIR/lowbyte_sum_return.out.s"
grep -Eq 'add[[:space:]]+a,[[:space:]]*c' "$TMPDIR/lowbyte_sum_return.out.s"
grep -Eq 'add[[:space:]]+a,[[:space:]]*#49' "$TMPDIR/lowbyte_sum_return.out.s"
if grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#305|add[[:space:]]+hl,[[:space:]]*bc|add[[:space:]]+hl,[[:space:]]*de|ld[[:space:]]+a,[[:space:]]*l' "$TMPDIR/lowbyte_sum_return.out.s"; then
    echo "xopt smoke: low-byte sum return did not collapse to A arithmetic" >&2
    exit 1
fi

cat >"$TMPDIR/dead_hl_before_pair_load.s" <<'ASM'
_dead_hl_pair:
	ld	hl, #0
	ld	l, -16(ix)
	ld	h, -15(ix)
	ex	de, hl
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_hl_before_pair_load.s" -o "$TMPDIR/dead_hl_before_pair_load.out.s"
if grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#0' "$TMPDIR/dead_hl_before_pair_load.out.s"; then
    echo "xopt smoke: dead HL immediate before pair load was not removed" >&2
    exit 1
fi
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-16\(ix\)' "$TMPDIR/dead_hl_before_pair_load.out.s"
grep -Eq 'ld[[:space:]]+h,[[:space:]]*-15\(ix\)' "$TMPDIR/dead_hl_before_pair_load.out.s"

cat >"$TMPDIR/dead_hl_before_pair_load_live.s" <<'ASM'
_dead_hl_pair_live:
	ld	hl, #4660
	ld	a, l
	ld	h, #0
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_hl_before_pair_load_live.s" -o "$TMPDIR/dead_hl_before_pair_load_live.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#4660' "$TMPDIR/dead_hl_before_pair_load_live.out.s"

cat >"$TMPDIR/scaled_offset_temp.s" <<'ASM'
_scaled:
	ld	l, -2(ix)
	ld	h, #0
	add	hl,hl
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	e, -21(ix)
	ld	d, -20(ix)
	add	hl,de
	ret
ASM

"$XOPT" -O3 "$TMPDIR/scaled_offset_temp.s" -o "$TMPDIR/scaled_offset_temp.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/scaled_offset_temp.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/scaled_offset_temp.out.s"
if grep -Eq 'ld[[:space:]]+-2[01]\(ix\)|ld[[:space:]]+[ed],[[:space:]]*-2[01]\(ix\)' "$TMPDIR/scaled_offset_temp.out.s"; then
    echo "xopt smoke: scaled offset temp spill was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/scaled_offset_temp_live.s" <<'ASM'
_scaled_live:
	ld	l, -2(ix)
	ld	h, #0
	add	hl,hl
	ld	-21(ix), l
	ld	-20(ix), h
	ld	l, -16(ix)
	ld	h, -15(ix)
	ld	e, -21(ix)
	ld	d, -20(ix)
	add	hl,de
	ld	a, -21(ix)
	ret
ASM

"$XOPT" -O3 "$TMPDIR/scaled_offset_temp_live.s" -o "$TMPDIR/scaled_offset_temp_live.out.s"
grep -Eq 'ld[[:space:]]+-21\(ix\),[[:space:]]*l' "$TMPDIR/scaled_offset_temp_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-21\(ix\)' "$TMPDIR/scaled_offset_temp_live.out.s"

cat >"$TMPDIR/index14_scaled_base_temp.s" <<'ASM'
_index14_scaled_base_temp:
	ld	l,-1(ix)
	ld	h,#0
	ld	-3(ix),l
	ld	-2(ix),h
	add	hl,hl
	ld	-5(ix),l
	ld	-4(ix),h
	ld	l,-3(ix)
	ld	h,-2(ix)
	add	hl,hl
	add	hl,hl
	ld	-7(ix),l
	ld	-6(ix),h
	ld	l,-3(ix)
	ld	h,-2(ix)
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	-9(ix),l
	ld	-8(ix),h
	ld	l,-5(ix)
	ld	h,-4(ix)
	ld	e,-7(ix)
	ld	d,-6(ix)
	add	hl,de
	ld	e,-9(ix)
	ld	d,-8(ix)
	add	hl,de
	ld	-11(ix),l
	ld	-10(ix),h
	ld	hl,#_base
	ld	e,-11(ix)
	ld	d,-10(ix)
	add	hl,de
	ld	-13(ix),l
	ld	-12(ix),h
	ld	de,#11
	add	hl,de
	call	_use
	ret
ASM

"$XOPT" -Os "$TMPDIR/index14_scaled_base_temp.s" -o "$TMPDIR/index14_scaled_base_temp.out.s"
grep -Eq 'add[[:space:]]+hl,[[:space:]]*bc' "$TMPDIR/index14_scaled_base_temp.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_base' "$TMPDIR/index14_scaled_base_temp.out.s"
if grep -Eq -- '-([2-9]|1[01])\(ix\)' "$TMPDIR/index14_scaled_base_temp.out.s"; then
    echo "xopt smoke: index*14 scaled-base temps were not elided" >&2
    exit 1
fi

cat >"$TMPDIR/index14_scaled_base_temp_bc_live.s" <<'ASM'
_index14_scaled_base_temp_bc_live:
	ld	l,-1(ix)
	ld	h,#0
	ld	-3(ix),l
	ld	-2(ix),h
	add	hl,hl
	ld	-5(ix),l
	ld	-4(ix),h
	ld	l,-3(ix)
	ld	h,-2(ix)
	add	hl,hl
	add	hl,hl
	ld	-7(ix),l
	ld	-6(ix),h
	ld	l,-3(ix)
	ld	h,-2(ix)
	add	hl,hl
	add	hl,hl
	add	hl,hl
	ld	-9(ix),l
	ld	-8(ix),h
	ld	l,-5(ix)
	ld	h,-4(ix)
	ld	e,-7(ix)
	ld	d,-6(ix)
	add	hl,de
	ld	e,-9(ix)
	ld	d,-8(ix)
	add	hl,de
	ld	-11(ix),l
	ld	-10(ix),h
	ld	hl,#_base
	ld	e,-11(ix)
	ld	d,-10(ix)
	add	hl,de
	ld	-13(ix),l
	ld	-12(ix),h
	add	hl,bc
	call	_use
	ret
ASM

"$XOPT" -Os "$TMPDIR/index14_scaled_base_temp_bc_live.s" -o "$TMPDIR/index14_scaled_base_temp_bc_live.out.s"
grep -Eq 'ld[[:space:]]+-3\(ix\),[[:space:]]*l' "$TMPDIR/index14_scaled_base_temp_bc_live.out.s"

cat >"$TMPDIR/byte_shift_xor_temp.s" <<'ASM'
_left_shift_xor:
	ld	a, -7(ix)
	add	a, a
	add	a, a
	add	a, a
	ld	-11(ix), a
	ld	e, -7(ix)
	ld	d, -11(ix)
	ld	a, e
	xor	a, d
	ld	-7(ix), a
	ret
_right_shift_xor:
	ld	-7(ix), a
	srl	a
	srl	a
	srl	a
	srl	a
	srl	a
	ld	-12(ix), a
	ld	e, -7(ix)
	ld	d, -12(ix)
	ld	a, e
	xor	a, d
	ld	-7(ix), a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/byte_shift_xor_temp.s" -o "$TMPDIR/byte_shift_xor_temp.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*a' "$TMPDIR/byte_shift_xor_temp.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*a' "$TMPDIR/byte_shift_xor_temp.out.s"
grep -Eq 'xor[[:space:]]+e' "$TMPDIR/byte_shift_xor_temp.out.s"
if grep -Eq -- '-1[12]\(ix\)' "$TMPDIR/byte_shift_xor_temp.out.s"; then
    echo "xopt smoke: byte shift-xor temp spill was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/byte_shift_xor_temp_live.s" <<'ASM'
_left_shift_xor_live:
	ld	a, -7(ix)
	add	a, a
	add	a, a
	add	a, a
	ld	-11(ix), a
	ld	e, -7(ix)
	ld	d, -11(ix)
	ld	a, e
	xor	a, d
	ld	-7(ix), a
	ld	a, -11(ix)
	ret
ASM

"$XOPT" -O3 "$TMPDIR/byte_shift_xor_temp_live.s" -o "$TMPDIR/byte_shift_xor_temp_live.out.s"
grep -Eq 'ld[[:space:]]+-11\(ix\),[[:space:]]*a' "$TMPDIR/byte_shift_xor_temp_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-11\(ix\)' "$TMPDIR/byte_shift_xor_temp_live.out.s"

cat >"$TMPDIR/temp_const_word.s" <<'ASM'
_temp_const_word:
	; sdcccall(1) prologue: temp_const_word (locals=2, temp_frame=4, stack_params=0)
	ld	hl, #_table
	ld	-4(ix), l
	ld	-3(ix), h
__loop:
	ld	l, -4(ix)
	ld	h, -3(ix)
	add	hl, de
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_const_word.s" -o "$TMPDIR/temp_const_word.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#_table' "$TMPDIR/temp_const_word.out.s"
if grep -Eq 'ld[[:space:]]+[lh],[[:space:]]*-[34]\(ix\)' "$TMPDIR/temp_const_word.out.s"; then
    echo "xopt smoke: temp-frame constant word reload was not forwarded" >&2
    exit 1
fi

cat >"$TMPDIR/temp_const_word_local.s" <<'ASM'
_temp_const_word_local:
	; sdcccall(1) prologue: temp_const_word_local (locals=4, temp_frame=2, stack_params=0)
	ld	hl, #_table
	ld	-4(ix), l
	ld	-3(ix), h
	ld	a, #1
	ld	l, -4(ix)
	ld	h, -3(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_const_word_local.s" -o "$TMPDIR/temp_const_word_local.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-4\(ix\)' "$TMPDIR/temp_const_word_local.out.s"
grep -Eq 'ld[[:space:]]+h,[[:space:]]*-3\(ix\)' "$TMPDIR/temp_const_word_local.out.s"

cat >"$TMPDIR/zero_reg_dead_a.s" <<'ASM'
_zero_reg_dead_a:
	ld	a, h
	and	#0
	ld	h, a
	xor	a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/zero_reg_dead_a.s" -o "$TMPDIR/zero_reg_dead_a.out.s"
grep -Eq 'ld[[:space:]]+h,[[:space:]]*#0' "$TMPDIR/zero_reg_dead_a.out.s"
if grep -Eq 'and[[:space:]]+#0|ld[[:space:]]+a,[[:space:]]*h' "$TMPDIR/zero_reg_dead_a.out.s"; then
    echo "xopt smoke: dead-A zero-register cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/zero_reg_live_flags.s" <<'ASM'
_zero_reg_live_flags:
	ld	a, h
	and	#0
	ld	h, a
	jr	z, _done
_done:
	ret
ASM

"$XOPT" -O3 "$TMPDIR/zero_reg_live_flags.s" -o "$TMPDIR/zero_reg_live_flags.out.s"
grep -Eq 'and[[:space:]]+#0' "$TMPDIR/zero_reg_live_flags.out.s"
grep -Eq 'jr[[:space:]]+z,' "$TMPDIR/zero_reg_live_flags.out.s"

cat >"$TMPDIR/call_result_store_de_direct.s" <<'ASM'
_call_result_store_de_direct:
	call	_foo
	ex	de, hl
	ld	b, h
	ld	c, l
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #0
	ld	de, #0
	ld	bc, #0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/call_result_store_de_direct.s" -o "$TMPDIR/call_result_store_de_direct.out.s"
grep -Eq 'ld[[:space:]]+-4\(ix\),[[:space:]]*e' "$TMPDIR/call_result_store_de_direct.out.s"
grep -Eq 'ld[[:space:]]+-3\(ix\),[[:space:]]*d' "$TMPDIR/call_result_store_de_direct.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl|ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l' "$TMPDIR/call_result_store_de_direct.out.s"; then
    echo "xopt smoke: call-result direct store did not remove the exchange/copy" >&2
    exit 1
fi

cat >"$TMPDIR/call_result_store_hl_live.s" <<'ASM'
_call_result_store_hl_live:
	call	_foo
	ex	de, hl
	ld	-4(ix), l
	ld	-3(ix), h
	ld	a, l
	ld	de, #0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/call_result_store_hl_live.s" -o "$TMPDIR/call_result_store_hl_live.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/call_result_store_hl_live.out.s"

cat >"$TMPDIR/dead_bc_store_copy.s" <<'ASM'
_dead_bc_store_copy:
	ld	b, h
	ld	c, l
	ld	-4(ix), l
	ld	-3(ix), h
	ld	bc, #0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_bc_store_copy.s" -o "$TMPDIR/dead_bc_store_copy.out.s"
grep -Eq 'ld[[:space:]]+-4\(ix\),[[:space:]]*l' "$TMPDIR/dead_bc_store_copy.out.s"
grep -Eq 'ld[[:space:]]+-3\(ix\),[[:space:]]*h' "$TMPDIR/dead_bc_store_copy.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l' "$TMPDIR/dead_bc_store_copy.out.s"; then
    echo "xopt smoke: dead-BC store copy cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/dead_bc_copy_before_c_call.s" <<'ASM'
_dead_bc_copy_before_c_call:
	ld	b, h
	ld	c, l
	push	hl
	.globl	_consume_hl
	call	_consume_hl
	inc	sp
	inc	sp
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_bc_copy_before_c_call.s" -o "$TMPDIR/dead_bc_copy_before_c_call.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l' "$TMPDIR/dead_bc_copy_before_c_call.out.s"; then
    echo "xopt smoke: dead BC copy before a direct C call survived" >&2
    exit 1
fi

cat >"$TMPDIR/bc_copy_before_internal_call.s" <<'ASM'
_bc_copy_before_internal_call:
	ld	b, h
	ld	c, l
	call	__sdcc_call_bc
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_copy_before_internal_call.s" -o "$TMPDIR/bc_copy_before_internal_call.out.s"
grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/bc_copy_before_internal_call.out.s"
grep -Eq 'ld[[:space:]]+c,[[:space:]]*l' "$TMPDIR/bc_copy_before_internal_call.out.s"

cat >"$TMPDIR/dead_bc_store_copy_live.s" <<'ASM'
_dead_bc_store_copy_live:
	ld	b, h
	ld	c, l
	ld	-4(ix), l
	ld	-3(ix), h
	ld	a, c
	ld	bc, #0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_bc_store_copy_live.s" -o "$TMPDIR/dead_bc_store_copy_live.out.s"
grep -Eq 'ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/dead_bc_store_copy_live.out.s"
grep -Eq 'ld[[:space:]]+c,[[:space:]]*l' "$TMPDIR/dead_bc_store_copy_live.out.s"

cat >"$TMPDIR/lowbyte_zero_extend_to_de.s" <<'ASM'
_lowbyte_zero_extend_to_de:
	ld	a, h
	and	#0
	ld	h, a
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	hl, #0
	ld	bc, #0
	ret
ASM

"$XOPT" -O3 "$TMPDIR/lowbyte_zero_extend_to_de.s" -o "$TMPDIR/lowbyte_zero_extend_to_de.out.s"
grep -Eq 'and[[:space:]]+#0' "$TMPDIR/lowbyte_zero_extend_to_de.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*a' "$TMPDIR/lowbyte_zero_extend_to_de.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/lowbyte_zero_extend_to_de.out.s"
if grep -Eq 'ld[[:space:]]+a,[[:space:]]*h|ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/lowbyte_zero_extend_to_de.out.s"; then
    echo "xopt smoke: low-byte zero-extension to DE cleanup did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.s" <<'ASM'
_lowbyte_zero_extend_to_de_call_clobber:
	ld	a, h
	and	#0
	ld	h, a
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	l, -2(ix)
	ld	h, -1(ix)
	call	_use_de
	ret
ASM

"$XOPT" -O3 "$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.s" -o "$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*a' "$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/lowbyte_zero_extend_to_de_call_clobber.out.s"; then
    echo "xopt smoke: low-byte zero-extension cleanup did not treat call as BC clobber" >&2
    exit 1
fi

cat >"$TMPDIR/lowbyte_zero_extend_to_de_hl_live.s" <<'ASM'
_lowbyte_zero_extend_to_de_hl_live:
	ld	a, h
	and	#0
	ld	h, a
	ld	b, h
	ld	c, l
	ld	d, b
	ld	e, c
	ld	a, h
	ret
ASM

"$XOPT" -O3 "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.s" -o "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"
grep -Eq 'ld[[:space:]]+h,[[:space:]]*a' "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*h' "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*h' "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/lowbyte_zero_extend_to_de_hl_live.out.s"; then
    echo "xopt smoke: low-byte zero-extension HL-live case kept a dead BC shuttle" >&2
    exit 1
fi

cat >"$TMPDIR/dead_temp_ix_store.s" <<'ASM'
_dead_temp_ix_store:
	; sdcccall(1) prologue: dead_temp_ix_store (locals=1, temp_frame=2, stack_params=0)
	ld	-1(ix), a
	ld	-2(ix), a
	ld	b, a
	ret
ASM

"$XOPT" -O3 "$TMPDIR/dead_temp_ix_store.s" -o "$TMPDIR/dead_temp_ix_store.out.s"
grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*a' "$TMPDIR/dead_temp_ix_store.out.s"
if grep -Eq 'ld[[:space:]]+-2\(ix\),[[:space:]]*a' "$TMPDIR/dead_temp_ix_store.out.s"; then
    echo "xopt smoke: dead compiler temp IX store was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/temp_store_read_before_preserved_reload.s" <<'ASM'
_temp_store_read_before_preserved_reload:
	; sdcccall(0) prologue: temp_store_read_before_preserved_reload (locals=0, temp_frame=6, stack_params=0)
	ld	-6(ix), a
	ld	a, -3(ix)
	xor	a, -6(ix)
	ld	-3(ix), a
	bit	7, a
	jp	z, L_first_else
L_first_true:
	ld	a, -3(ix)
	add	a, a
	xor	#7
	ld	-3(ix), a
	jr	L_first_join
L_first_else:
	ld	a, -3(ix)
	add	a, a
	ld	-3(ix), a
L_first_join:
	bit	7, -3(ix)
	jp	z, L_second_else
L_second_true:
	ld	a, -3(ix)
	add	a, a
	xor	#7
	ld	-3(ix), a
	jr	L_second_join
L_second_else:
	ld	a, -3(ix)
	add	a, a
	ld	-3(ix), a
L_second_join:
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_store_read_before_preserved_reload.s" -o "$TMPDIR/temp_store_read_before_preserved_reload.out.s"
if grep -Eq 'bit[[:space:]]+7,[[:space:]]*-3\(ix\)' "$TMPDIR/temp_store_read_before_preserved_reload.out.s" &&
   ! sed -n '/^L_first_join:/,+2p' "$TMPDIR/temp_store_read_before_preserved_reload.out.s" |
       grep -Eq 'ld[[:space:]]+-3\(ix\),[[:space:]]*a'; then
    echo "xopt smoke: IX store read before an A-preserved reload was removed" >&2
    exit 1
fi

cat >"$TMPDIR/temp_store_branch_read_before_rewrite.s" <<'ASM'
_temp_store_branch_read_before_rewrite:
	; sdcccall(0) prologue: temp_store_branch_read_before_rewrite (locals=0, temp_frame=2, stack_params=0)
	ld	-1(ix), a
	cp	#32
	jr	z, L_return_zero
	xor	a, a
	jr	L_compare_more
L_return_zero:
	ld	-2(ix), #0
	ld	-1(ix), #0
	jr	L_done
L_compare_more:
	ld	a, -1(ix)
	cp	#9
	jr	z, L_done
	ld	a, -1(ix)
	cp	#10
L_done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_store_branch_read_before_rewrite.s" -o "$TMPDIR/temp_store_branch_read_before_rewrite.out.s"
grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*a' "$TMPDIR/temp_store_branch_read_before_rewrite.out.s"

cat >"$TMPDIR/pair_imm_copy_reload.s" <<'ASM'
_pair_imm_copy_reload:
	ld	hl, #_service
	ld	d, h
	ld	e, l
	ld	hl, #_name
	call	_register
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_imm_copy_reload.s" -o "$TMPDIR/pair_imm_copy_reload.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_service' "$TMPDIR/pair_imm_copy_reload.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#_name' "$TMPDIR/pair_imm_copy_reload.out.s"
if grep -Eq 'ld[[:space:]]+d,[[:space:]]*h|ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/pair_imm_copy_reload.out.s"; then
    echo "xopt smoke: pair immediate direct copy/reload was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/pair_imm_copy_reload_bc_temp.s" <<'ASM'
_pair_imm_copy_reload_bc_temp:
	ld	hl, #_heap
	ld	b, h
	ld	c, l
	ld	hl, #65535
	ld	d, b
	ld	e, c
	or	a, a
	sbc	hl, de
	push	hl
	call	_print
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_imm_copy_reload_bc_temp.s" -o "$TMPDIR/pair_imm_copy_reload_bc_temp.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_heap' "$TMPDIR/pair_imm_copy_reload_bc_temp.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#65535' "$TMPDIR/pair_imm_copy_reload_bc_temp.out.s"
if grep -Eq 'ld[[:space:]]+b,[[:space:]]*h|ld[[:space:]]+c,[[:space:]]*l|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/pair_imm_copy_reload_bc_temp.out.s"; then
    echo "xopt smoke: pair immediate BC-temp copy/reload was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/pair_imm_copy_reload_bc_live.s" <<'ASM'
_pair_imm_copy_reload_bc_live:
	ld	hl, #_heap
	ld	b, h
	ld	c, l
	ld	hl, #65535
	ld	d, b
	ld	e, c
	ld	a, b
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_imm_copy_reload_bc_live.s" -o "$TMPDIR/pair_imm_copy_reload_bc_live.out.s"
grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_heap|ld[[:space:]]+b,[[:space:]]*h' "$TMPDIR/pair_imm_copy_reload_bc_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*b' "$TMPDIR/pair_imm_copy_reload_bc_live.out.s"
if grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_heap' "$TMPDIR/pair_imm_copy_reload_bc_live.out.s"; then
    echo "xopt smoke: pair immediate BC-temp live case clobbered BC" >&2
    exit 1
fi

cat >"$TMPDIR/temp_base_subtract_exchange.s" <<'ASM'
_temp_base_subtract_exchange:
	; sdcccall(1) prologue: temp_base_subtract_exchange (locals=0, temp_frame=4, stack_params=0)
	ld	hl, #_heap
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #65535
	ld	de, #_heap
	or	a, a
	sbc	hl, de
	ld	d, h
	ld	e, l
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	_mem_init
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_base_subtract_exchange.s" -o "$TMPDIR/temp_base_subtract_exchange.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#65535' "$TMPDIR/temp_base_subtract_exchange.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_heap' "$TMPDIR/temp_base_subtract_exchange.out.s"
grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/temp_base_subtract_exchange.out.s"
if grep -Eq -- '-[34]\(ix\)|ld[[:space:]]+d,[[:space:]]*h|ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/temp_base_subtract_exchange.out.s"; then
    echo "xopt smoke: temp base subtract/exchange spill was not elided" >&2
    exit 1
fi

cat >"$TMPDIR/temp_base_subtract_exchange_live.s" <<'ASM'
_temp_base_subtract_exchange_live:
	; sdcccall(1) prologue: temp_base_subtract_exchange_live (locals=0, temp_frame=4, stack_params=0)
	ld	hl, #_heap
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #65535
	ld	de, #_heap
	or	a, a
	sbc	hl, de
	ld	d, h
	ld	e, l
	ld	l, -4(ix)
	ld	h, -3(ix)
	call	_mem_init
	ld	a, -4(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_base_subtract_exchange_live.s" -o "$TMPDIR/temp_base_subtract_exchange_live.out.s"
grep -Eq 'ld[[:space:]]+-4\(ix\),[[:space:]]*l' "$TMPDIR/temp_base_subtract_exchange_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-4\(ix\)' "$TMPDIR/temp_base_subtract_exchange_live.out.s"

cat >"$TMPDIR/bc_imm_copy_to_pair_direct.s" <<'ASM'
_bc_imm_copy_to_pair_direct:
	ld	bc, #_heap
	ld	hl, #65535
	ld	d, b
	ld	e, c
	or	a, a
	sbc	hl, de
	push	hl
	call	_print
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_imm_copy_to_pair_direct.s" -o "$TMPDIR/bc_imm_copy_to_pair_direct.out.s"
grep -Eq 'ld[[:space:]]+de,[[:space:]]*#_heap' "$TMPDIR/bc_imm_copy_to_pair_direct.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#65535' "$TMPDIR/bc_imm_copy_to_pair_direct.out.s"
if grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_heap|ld[[:space:]]+d,[[:space:]]*b|ld[[:space:]]+e,[[:space:]]*c' "$TMPDIR/bc_imm_copy_to_pair_direct.out.s"; then
    echo "xopt smoke: BC immediate copy to pair was not folded" >&2
    exit 1
fi

cat >"$TMPDIR/bc_imm_copy_to_pair_bc_live.s" <<'ASM'
_bc_imm_copy_to_pair_bc_live:
	ld	bc, #_heap
	ld	hl, #65535
	ld	d, b
	ld	e, c
	ld	a, b
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_imm_copy_to_pair_bc_live.s" -o "$TMPDIR/bc_imm_copy_to_pair_bc_live.out.s"
grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_heap' "$TMPDIR/bc_imm_copy_to_pair_bc_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*b' "$TMPDIR/bc_imm_copy_to_pair_bc_live.out.s"

cat >"$TMPDIR/bc_imm_copy_to_pair_dest_live.s" <<'ASM'
_bc_imm_copy_to_pair_dest_live:
	ld	bc, #_heap
	ld	a, d
	ld	d, b
	ld	e, c
	call	_print
	ret
ASM

"$XOPT" -Os "$TMPDIR/bc_imm_copy_to_pair_dest_live.s" -o "$TMPDIR/bc_imm_copy_to_pair_dest_live.out.s"
grep -Eq 'ld[[:space:]]+bc,[[:space:]]*#_heap' "$TMPDIR/bc_imm_copy_to_pair_dest_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*d' "$TMPDIR/bc_imm_copy_to_pair_dest_live.out.s"

cat >"$TMPDIR/dead_hl_zero_extend_before_pair_load.s" <<'ASM'
_dead_hl_zero_extend_before_pair_load:
	ld	l, a
	ld	h, #0
	ld	l, -3(ix)
	ld	h, -2(ix)
	push	hl
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_hl_zero_extend_before_pair_load.s" -o "$TMPDIR/dead_hl_zero_extend_before_pair_load.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-3\(ix\)' "$TMPDIR/dead_hl_zero_extend_before_pair_load.out.s"
if grep -Eq 'ld[[:space:]]+l,[[:space:]]*a|ld[[:space:]]+h,[[:space:]]*#0' "$TMPDIR/dead_hl_zero_extend_before_pair_load.out.s"; then
    echo "xopt smoke: dead HL zero-extend before full pair load was not removed" >&2
    exit 1
fi

cat >"$TMPDIR/dead_hl_zero_extend_old_hl_live.s" <<'ASM'
_dead_hl_zero_extend_old_hl_live:
	ld	l, a
	ld	h, #0
	ld	l, (hl)
	ld	h, #0
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_hl_zero_extend_old_hl_live.s" -o "$TMPDIR/dead_hl_zero_extend_old_hl_live.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*a' "$TMPDIR/dead_hl_zero_extend_old_hl_live.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*\(hl\)' "$TMPDIR/dead_hl_zero_extend_old_hl_live.out.s"

cat >"$TMPDIR/byte_temp_zero_extend_after_test.s" <<'ASM'
_byte_temp_zero_extend_after_test:
	; sdcccall(1) prologue: f (locals=0, temp_frame=4, stack_params=0)
	ld	a, (hl)
	ld	-1(ix), a
	or	a, a
	jr	z, done
fallthrough:
	ld	l, -1(ix)
	ld	h, #0
	call	_tty_putc
done:
	ret
ASM

"$XOPT" -Os "$TMPDIR/byte_temp_zero_extend_after_test.s" -o "$TMPDIR/byte_temp_zero_extend_after_test.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*a' "$TMPDIR/byte_temp_zero_extend_after_test.out.s"
if grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*a|ld[[:space:]]+l,[[:space:]]*-1\(ix\)' "$TMPDIR/byte_temp_zero_extend_after_test.out.s"; then
    echo "xopt smoke: byte temp zero-extend forwarding did not fire" >&2
    exit 1
fi

cat >"$TMPDIR/byte_temp_zero_extend_live.s" <<'ASM'
_byte_temp_zero_extend_live:
	; sdcccall(1) prologue: f (locals=0, temp_frame=4, stack_params=0)
	ld	a, (hl)
	ld	-1(ix), a
	or	a, a
	jr	z, done
fallthrough:
	ld	l, -1(ix)
	ld	h, #0
	call	_tty_putc
done:
	ld	a, -1(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/byte_temp_zero_extend_live.s" -o "$TMPDIR/byte_temp_zero_extend_live.out.s"
grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*a' "$TMPDIR/byte_temp_zero_extend_live.out.s"
grep -Eq 'ld[[:space:]]+l,[[:space:]]*-1\(ix\)' "$TMPDIR/byte_temp_zero_extend_live.out.s"

cat >"$TMPDIR/indexed_stack_immediate_store_run.s" <<'ASM'
_indexed_stack_immediate_store_run:
	; sdcccall(1) prologue: f (locals=1, temp_frame=4, stack_params=0)
	ld	a, -1(ix)
	inc	-1(ix)
	ld	hl, #12
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #46
	ld	a, -1(ix)
	inc	-1(ix)
	ld	e, a
	ld	d, #0
	ld	hl, #12
	add	hl, sp
	add	hl, de
	ld	(hl), #97
	ld	a, -1(ix)
	ld	hl, #12
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #0
	ld	a, #5
	ld	de, #0
	or	a, a
	ret
ASM

"$XOPT" -Os "$TMPDIR/indexed_stack_immediate_store_run.s" -o "$TMPDIR/indexed_stack_immediate_store_run.out.s"
grep -Eq 'ld[[:space:]]+(a|e),[[:space:]]*-1\(ix\)' "$TMPDIR/indexed_stack_immediate_store_run.out.s"
grep -Eq 'ld[[:space:]]+\(hl\),[[:space:]]*#46' "$TMPDIR/indexed_stack_immediate_store_run.out.s"
grep -Eq 'ld[[:space:]]+\(hl\),[[:space:]]*#97' "$TMPDIR/indexed_stack_immediate_store_run.out.s"
grep -Eq 'ld[[:space:]]+\(hl\),[[:space:]]*#0' "$TMPDIR/indexed_stack_immediate_store_run.out.s"
if grep -Eq 'inc[[:space:]]+-1\(ix\)' "$TMPDIR/indexed_stack_immediate_store_run.out.s"; then
    echo "xopt smoke: indexed stack immediate store run kept dead index increments" >&2
    exit 1
fi
if [[ "$(grep -Ec 'inc[[:space:]]+hl' "$TMPDIR/indexed_stack_immediate_store_run.out.s")" != "2" ]]; then
    echo "xopt smoke: indexed stack immediate store run did not become sequential HL stores" >&2
    exit 1
fi

cat >"$TMPDIR/indexed_stack_immediate_store_run_de_killed_late.s" <<'ASM'
_indexed_stack_immediate_store_run_de_killed_late:
	; sdcccall(1) prologue: f (locals=18, temp_frame=13, stack_params=0)
	ld	a, -13(ix)
	inc	-13(ix)
	ld	hl, #19
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #46
	ld	a, -13(ix)
	inc	-13(ix)
	ld	hl, #19
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #97
	ld	a, -13(ix)
	ld	hl, #19
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #0
	ld	hl, #1024
	push	hl
	ld	hl, #21
	add	hl, sp
	ld	d, h
	ld	e, l
	ld	a, (_current_drive)
	.globl	_process_load
	call	_process_load
	ret
ASM

"$XOPT" -Os "$TMPDIR/indexed_stack_immediate_store_run_de_killed_late.s" -o "$TMPDIR/indexed_stack_immediate_store_run_de_killed_late.out.s"
if grep -Eq 'inc[[:space:]]+-13\(ix\)' "$TMPDIR/indexed_stack_immediate_store_run_de_killed_late.out.s"; then
    echo "xopt smoke: indexed stack immediate store run did not accept late DE overwrite proof" >&2
    exit 1
fi
if [[ "$(grep -Ec 'inc[[:space:]]+hl' "$TMPDIR/indexed_stack_immediate_store_run_de_killed_late.out.s")" != "2" ]]; then
    echo "xopt smoke: late-DE indexed stack store run did not become sequential HL stores" >&2
    exit 1
fi

cat >"$TMPDIR/indexed_stack_immediate_store_run_index_live.s" <<'ASM'
_indexed_stack_immediate_store_run_index_live:
	; sdcccall(1) prologue: f (locals=1, temp_frame=4, stack_params=0)
	ld	a, -1(ix)
	inc	-1(ix)
	ld	hl, #12
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #46
	ld	a, -1(ix)
	inc	-1(ix)
	ld	hl, #12
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #97
	ld	a, -1(ix)
	ld	hl, #12
	add	hl, sp
	ld	e, a
	ld	d, #0
	add	hl, de
	ld	(hl), #0
	ld	a, -1(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/indexed_stack_immediate_store_run_index_live.s" -o "$TMPDIR/indexed_stack_immediate_store_run_index_live.out.s"
grep -Eq 'inc[[:space:]]+-1\(ix\)' "$TMPDIR/indexed_stack_immediate_store_run_index_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-1\(ix\)' "$TMPDIR/indexed_stack_immediate_store_run_index_live.out.s"

cat >"$TMPDIR/pair_immediate_store_direct.s" <<'ASM'
_pair_immediate_store_direct:
	ld	hl, #4660
	ld	-4(ix), l
	ld	-3(ix), h
	ld	hl, #_banner
	ld	-6(ix), l
	ld	-5(ix), h
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_immediate_store_direct.s" -o "$TMPDIR/pair_immediate_store_direct.out.s"
grep -Eq 'ld[[:space:]]+-4\(ix\),[[:space:]]*#52' "$TMPDIR/pair_immediate_store_direct.out.s"
grep -Eq 'ld[[:space:]]+-3\(ix\),[[:space:]]*#18' "$TMPDIR/pair_immediate_store_direct.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#_banner' "$TMPDIR/pair_immediate_store_direct.out.s"
if grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#4660' "$TMPDIR/pair_immediate_store_direct.out.s"; then
    echo "xopt smoke: pair immediate store direct kept dead HL load" >&2
    exit 1
fi

cat >"$TMPDIR/pair_immediate_store_direct_live.s" <<'ASM'
_pair_immediate_store_direct_live:
	ld	hl, #4660
	ld	-4(ix), l
	ld	-3(ix), h
	ld	a, h
	ret
ASM

"$XOPT" -Os "$TMPDIR/pair_immediate_store_direct_live.s" -o "$TMPDIR/pair_immediate_store_direct_live.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#4660' "$TMPDIR/pair_immediate_store_direct_live.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*h' "$TMPDIR/pair_immediate_store_direct_live.out.s"

cat >"$TMPDIR/ix_pair_compare_load_de_direct.s" <<'ASM'
_ix_pair_compare_load_de_direct:
	ld	l, -11(ix)
	ld	h, -10(ix)
	ex	de, hl
	ld	l, -7(ix)
	ld	h, -6(ix)
	or	a, a
	sbc	hl, de
	ret
ASM

"$XOPT" -Os "$TMPDIR/ix_pair_compare_load_de_direct.s" -o "$TMPDIR/ix_pair_compare_load_de_direct.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-11\(ix\)' "$TMPDIR/ix_pair_compare_load_de_direct.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-10\(ix\)' "$TMPDIR/ix_pair_compare_load_de_direct.out.s"
grep -Eq 'sbc[[:space:]]+hl,[[:space:]]*de' "$TMPDIR/ix_pair_compare_load_de_direct.out.s"
if grep -Eq 'ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/ix_pair_compare_load_de_direct.out.s"; then
    echo "xopt smoke: IX-pair compare kept avoidable exchange" >&2
    exit 1
fi

cat >"$TMPDIR/small_stack_alloc_push_af.s" <<'ASM'
_small_stack_alloc_push_af:
	ld	hl, #-5
	add	hl, sp
	ld	sp, hl
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/small_stack_alloc_push_af.s" -o "$TMPDIR/small_stack_alloc_push_af.out.s"
if [[ "$(grep -Ec '^[[:space:]]+push[[:space:]]+af' "$TMPDIR/small_stack_alloc_push_af.out.s")" != "2" ]]; then
    echo "xopt smoke: small odd stack allocation did not use compact pushes" >&2
    exit 1
fi
grep -Eq '^[[:space:]]+dec[[:space:]]+sp' "$TMPDIR/small_stack_alloc_push_af.out.s"
if grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#-5|add[[:space:]]+hl,[[:space:]]*sp|ld[[:space:]]+sp,[[:space:]]*hl' "$TMPDIR/small_stack_alloc_push_af.out.s"; then
    echo "xopt smoke: small stack allocation kept arithmetic setup" >&2
    exit 1
fi

cat >"$TMPDIR/speed_stack_alloc_push_af.s" <<'ASM'
_speed_stack_alloc_push_af:
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	xor	a
	ret
_speed_stack_alloc_five_bytes:
	ld	hl, #-5
	add	hl, sp
	ld	sp, hl
	xor	a
	ret
ASM

"$XOPT" -Of "$TMPDIR/speed_stack_alloc_push_af.s" \
    -o "$TMPDIR/speed_stack_alloc_push_af.out.s"
if [[ "$(awk '
    /^_speed_stack_alloc_push_af:/ { in_fn=1; next }
    /^_speed_stack_alloc_five_bytes:/ { in_fn=0 }
    in_fn && /^[[:space:]]+push[[:space:]]+af/ { count++ }
    END { print count+0 }
' "$TMPDIR/speed_stack_alloc_push_af.out.s")" != "2" ]]; then
    echo "xopt smoke: -Of did not promote the faster four-byte stack allocation" >&2
    exit 1
fi
if ! awk '
    /^_speed_stack_alloc_five_bytes:/ { in_fn=1; next }
    in_fn && /ld[[:space:]]+hl,[[:space:]]*#-5/ { load=1 }
    in_fn && /add[[:space:]]+hl,[[:space:]]*sp/ { add=1 }
    in_fn && /ld[[:space:]]+sp,[[:space:]]*hl/ { store=1 }
    END { exit load && add && store ? 0 : 1 }
' "$TMPDIR/speed_stack_alloc_push_af.out.s"; then
    echo "xopt smoke: -Of selected slower PUSH/DEC allocation for five bytes" >&2
    exit 1
fi

cat >"$TMPDIR/redundant_pair_immediate_stores.s" <<'ASM'
_redundant_pair_immediate_stores:
	ld	hl, #4660
	ld	(_first), hl
	ld	(_second), hl
	ld	hl, #4660
	ld	(_third), hl
	ret
ASM

"$XOPT" -Of "$TMPDIR/redundant_pair_immediate_stores.s" \
    -o "$TMPDIR/redundant_pair_immediate_stores.out.s"
if [[ "$(grep -Ec 'ld[[:space:]]+hl,[[:space:]]*#4660' \
        "$TMPDIR/redundant_pair_immediate_stores.out.s")" != "1" ]]; then
    echo "xopt smoke: -Of retained a redundant pair immediate across stores" >&2
    exit 1
fi

cat >"$TMPDIR/large_stack_alloc_stays_compact.s" <<'ASM'
_large_stack_alloc_stays_compact:
	ld	hl, #-11
	add	hl, sp
	ld	sp, hl
	xor	a
	ret
ASM

"$XOPT" -Os "$TMPDIR/large_stack_alloc_stays_compact.s" \
    -o "$TMPDIR/large_stack_alloc_stays_compact.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#-11' \
    "$TMPDIR/large_stack_alloc_stays_compact.out.s"
if grep -Eq '^[[:space:]]+push[[:space:]]+af|^[[:space:]]+dec[[:space:]]+sp' \
    "$TMPDIR/large_stack_alloc_stays_compact.out.s"; then
    echo "xopt smoke: -Os expanded a five-byte stack adjustment" >&2
    exit 1
fi

cat >"$TMPDIR/small_stack_alloc_flags_live.s" <<'ASM'
_small_stack_alloc_flags_live:
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	jr	c, _carry
_carry:
	ret
ASM

"$XOPT" -Os "$TMPDIR/small_stack_alloc_flags_live.s" -o "$TMPDIR/small_stack_alloc_flags_live.out.s"
grep -Eq 'ld[[:space:]]+hl,[[:space:]]*#-4' "$TMPDIR/small_stack_alloc_flags_live.out.s"
grep -Eq 'jr[[:space:]]+c,' "$TMPDIR/small_stack_alloc_flags_live.out.s"

cat >"$TMPDIR/accumulator_one_inc.s" <<'ASM'
_accumulator_one_inc:
	add	a, #1
	ld	-1(ix), a
	and	#15
	ret
_accumulator_one_carry_live:
	add	a, #1
	jr	c, _carry
_carry:
	ret
ASM

"$XOPT" -Os "$TMPDIR/accumulator_one_inc.s" -o "$TMPDIR/accumulator_one_inc.out.s"
grep -Eq '^[[:space:]]+inc[[:space:]]+a' "$TMPDIR/accumulator_one_inc.out.s"
grep -Eq '^[[:space:]]+add[[:space:]]+a,[[:space:]]*#1' "$TMPDIR/accumulator_one_inc.out.s"

cat >"$TMPDIR/redundant_a_zero_extend_reload.s" <<'ASM'
_redundant_a_zero_extend_reload:
	ld	l, a
	ld	h, #0
	ld	a, l
	and	#7
	ret
ASM

"$XOPT" -Os "$TMPDIR/redundant_a_zero_extend_reload.s" -o "$TMPDIR/redundant_a_zero_extend_reload.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+l,[[:space:]]*a' "$TMPDIR/redundant_a_zero_extend_reload.out.s"
if grep -Eq '^[[:space:]]+ld[[:space:]]+a,[[:space:]]*l' "$TMPDIR/redundant_a_zero_extend_reload.out.s"; then
    echo "xopt smoke: zero extension kept redundant A reload" >&2
    exit 1
fi

cat >"$TMPDIR/cp_zero_branch_to_or.s" <<'ASM'
_cp_zero_branch_to_or:
	cp	#0
	ld	hl, #1
	jr	z, _zero
	dec	hl
_zero:
	xor	a
	ret
_cp_zero_parity_live:
	cp	#0
	jr	pe, _parity
	xor	a
_parity:
	ret
ASM

"$XOPT" -Os "$TMPDIR/cp_zero_branch_to_or.s" -o "$TMPDIR/cp_zero_branch_to_or.out.s"
grep -Eq '^[[:space:]]+or[[:space:]]+a,[[:space:]]*a' "$TMPDIR/cp_zero_branch_to_or.out.s"
grep -Eq '^[[:space:]]+cp[[:space:]]+#0' "$TMPDIR/cp_zero_branch_to_or.out.s"

cat >"$TMPDIR/modern_return_pair_exchange.s" <<'ASM'
	.area _CODE
_modern_return_pair_exchange:
	; sdcccall(1) prologue: modern_return_pair_exchange (locals=0, temp_frame=0, stack_params=0)
	ld	d, h
	ld	e, l
	ret
_legacy_return_pair_copy:
	; z88dk fastcall prologue: legacy_return_pair_copy (locals=0, temp_frame=0, stack_params=0)
	ld	d, h
	ld	e, l
	ret
ASM

"$XOPT" -Os "$TMPDIR/modern_return_pair_exchange.s" -o "$TMPDIR/modern_return_pair_exchange.out.s"
grep -Eq '^[[:space:]]+ex[[:space:]]+de,[[:space:]]*hl' "$TMPDIR/modern_return_pair_exchange.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+d,[[:space:]]*h' "$TMPDIR/modern_return_pair_exchange.out.s"
grep -Eq '^[[:space:]]+ld[[:space:]]+e,[[:space:]]*l' "$TMPDIR/modern_return_pair_exchange.out.s"

cat >"$TMPDIR/cp_threshold_branch_fold.s" <<'ASM'
_cp_threshold_branch_fold:
	cp	#32
	jr	z, _small
	jr	c, _small
	ret
_small:
	ret
_cp_threshold_branch_fold_split:
	cp	#6
	jr	z, _ok
	jr	nc, _bad
	jr	_ok
_bad:
	ret
_ok:
	ret
ASM

"$XOPT" -Os "$TMPDIR/cp_threshold_branch_fold.s" -o "$TMPDIR/cp_threshold_branch_fold.out.s"
grep -Eq 'cp[[:space:]]+#33' "$TMPDIR/cp_threshold_branch_fold.out.s"
grep -Eq 'jr[[:space:]]+c,[[:space:]]*_small' "$TMPDIR/cp_threshold_branch_fold.out.s"
grep -Eq 'cp[[:space:]]+#7' "$TMPDIR/cp_threshold_branch_fold.out.s"
grep -Eq 'jr[[:space:]]+(nc,[[:space:]]*_bad|c,[[:space:]]*_ok)' "$TMPDIR/cp_threshold_branch_fold.out.s"
if grep -Eq 'jr[[:space:]]+z,[[:space:]]*_small|jr[[:space:]]+z,[[:space:]]*_ok' "$TMPDIR/cp_threshold_branch_fold.out.s"; then
    echo "xopt smoke: cp threshold branch fold kept redundant z branch" >&2
    exit 1
fi

cat >"$TMPDIR/dead_internal_label_outline.s" <<'ASM'
	.area _CODE
_dead_internal_label_outline_a:
	ld	a, -1(ix)
	add	a, #7
__xcc_dead_join:
	xor	#85
	ld	-1(ix), a
	ret
_dead_internal_label_outline_b:
	ld	a, -1(ix)
	add	a, #7
__worker_end:
	xor	#85
	ld	-1(ix), a
	ret
ASM

"$XOPT" -Os "$TMPDIR/dead_internal_label_outline.s" -o "$TMPDIR/dead_internal_label_outline.out.s"
grep -Eq '(call|jp|jr)[[:space:]]+__xopt_(outline|tail)_' "$TMPDIR/dead_internal_label_outline.out.s"
if grep -Eq '^__xcc_dead_join:' "$TMPDIR/dead_internal_label_outline.out.s"; then
    echo "xopt smoke: unreferenced compiler-internal label survived size cleanup" >&2
    exit 1
fi

cat >"$TMPDIR/referenced_internal_label.s" <<'ASM'
	.area _CODE
	.globl	__exported_end
_referenced_internal_label:
	ld	hl, #__xcc_live_join
	ret
__xcc_live_join:
	.dw	__xcc_live_join
__exported_end:
	ret
ASM

"$XOPT" -Os "$TMPDIR/referenced_internal_label.s" -o "$TMPDIR/referenced_internal_label.out.s"
grep -Eq '^__xcc_live_join:' "$TMPDIR/referenced_internal_label.out.s"
grep -Eq '\.dw[[:space:]]+__xcc_live_join' "$TMPDIR/referenced_internal_label.out.s"
grep -Eq '^__exported_end:' "$TMPDIR/referenced_internal_label.out.s"

cat >"$TMPDIR/temp_frame_compact.s" <<'ASM'
	.area	_CODE
_temp_frame_compact:
	; sdcccall(1) prologue: temp_frame_compact (locals=0, temp_frame=6, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	push	af
	push	af
	push	af
	ld	e, -2(ix)
	ld	d, -1(ix)
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_temp_frame_sp_guard:
	; sdcccall(1) prologue: temp_frame_sp_guard (locals=0, temp_frame=6, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	push	af
	push	af
	push	af
	ld	hl, #-4
	add	hl, sp
	ld	sp, ix
	pop	ix
	ret
	.area	_CODE
_temp_frame_hole:
	; sdcccall(1) prologue: temp_frame_hole (locals=2, temp_frame=8, stack_params=0)
	push	ix
	ld	ix, #0
	add	ix, sp
	push	af
	push	af
	push	af
	push	af
	push	af
	ld	a, -1(ix)
	ld	e, -10(ix)
	ld	d, -9(ix)
	ld	sp, ix
	pop	ix
	ret
ASM

"$XOPT" -Os "$TMPDIR/temp_frame_compact.s" -o "$TMPDIR/temp_frame_compact.out.s"
grep -q 'temp_frame_compact (locals=0, temp_frame=2, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.out.s"
grep -q 'temp_frame_sp_guard (locals=0, temp_frame=6, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.out.s"
grep -q 'temp_frame_hole (locals=2, temp_frame=2, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.out.s"
grep -Eq 'ld[[:space:]]+e,[[:space:]]*-4\(ix\)' \
    "$TMPDIR/temp_frame_compact.out.s"
grep -Eq 'ld[[:space:]]+d,[[:space:]]*-3\(ix\)' \
    "$TMPDIR/temp_frame_compact.out.s"
if grep -Eq -- '-(9|10)\(ix\)' "$TMPDIR/temp_frame_compact.out.s"; then
    echo "xopt smoke: -Os left an interior temporary-frame hole" >&2
    exit 1
fi
if [[ "$(awk '/^_temp_frame_compact:/{in_fn=1;next} /^[^[:space:]].*:/{in_fn=0} in_fn && /push[[:space:]]+af/{n++} END{print n+0}' "$TMPDIR/temp_frame_compact.out.s")" != "1" ]]; then
    echo "xopt smoke: -Os did not compact a dead temporary-frame tail" >&2
    exit 1
fi

"$XOPT" -Of "$TMPDIR/temp_frame_compact.s" \
    -o "$TMPDIR/temp_frame_compact.of.out.s"
grep -q 'temp_frame_compact (locals=0, temp_frame=2, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.of.out.s"
grep -q 'temp_frame_sp_guard (locals=0, temp_frame=6, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.of.out.s"
grep -q 'temp_frame_hole (locals=2, temp_frame=2, stack_params=0)' \
    "$TMPDIR/temp_frame_compact.of.out.s"
if [[ "$(awk '/^_temp_frame_compact:/{in_fn=1;next} /^[^[:space:]].*:/{in_fn=0} in_fn && /push[[:space:]]+af/{n++} END{print n+0}' "$TMPDIR/temp_frame_compact.of.out.s")" != "1" ]]; then
    echo "xopt smoke: -Of did not promote faster dead-frame compaction" >&2
    exit 1
fi

cat >"$TMPDIR/call_result_byte_commute.s" <<'ASM'
	.area	_CODE
_call_result_byte_commute:
	; sdcccall(1) prologue: call_result_byte_commute (locals=0, temp_frame=2, stack_params=0)
	call	_produce_byte
	ld	-2(ix), a
	ld	a, -1(ix)
	xor	a, -2(ix)
	ld	-1(ix), a
	ld	e, -1(ix)
	ret
	.area	_CODE
_call_result_byte_live:
	; sdcccall(1) prologue: call_result_byte_live (locals=0, temp_frame=2, stack_params=0)
	call	_produce_byte
	ld	-2(ix), a
	ld	a, -1(ix)
	xor	a, -2(ix)
	ld	-1(ix), a
	ld	e, -2(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/call_result_byte_commute.s" -o "$TMPDIR/call_result_byte_commute.out.s"
grep -Eq 'xor[[:space:]]+a,[[:space:]]*-1\(ix\)' \
    "$TMPDIR/call_result_byte_commute.out.s"
if awk '/^_call_result_byte_commute:/{in_fn=1;next} /^_call_result_byte_live:/{in_fn=0} in_fn && /-2\(ix\)/{found=1} END{exit found ? 0 : 1}' \
    "$TMPDIR/call_result_byte_commute.out.s"; then
    echo "xopt smoke: dead byte call-result spill survived direct forwarding" >&2
    exit 1
fi
awk '/^_call_result_byte_live:/{in_fn=1;next} in_fn && /ld[[:space:]]+-2\(ix\),[[:space:]]*a/{found=1} END{exit found ? 0 : 1}' \
    "$TMPDIR/call_result_byte_commute.out.s"

cat >"$TMPDIR/loop_carried_a_spill.s" <<'ASM'
	.area	_CODE
_loop_carried_a_spill:
	; sdcccall(1) prologue: loop_carried_a_spill (locals=0, temp_frame=2, stack_params=0)
__loop_carried_body:
	call	_next_byte
	add	a, -1(ix)
	ld	-1(ix), a
	dec	-2(ix)
	jr	nz, __loop_carried_body
	ld	a, -1(ix)
	ret
ASM

"$XOPT" -Os "$TMPDIR/loop_carried_a_spill.s" -o "$TMPDIR/loop_carried_a_spill.out.s"
grep -Eq 'ld[[:space:]]+-1\(ix\),[[:space:]]*a' \
    "$TMPDIR/loop_carried_a_spill.out.s"
grep -Eq 'ld[[:space:]]+a,[[:space:]]*-1\(ix\)' \
    "$TMPDIR/loop_carried_a_spill.out.s"

cat >"$TMPDIR/adjacent_postinc_loop.s" <<'ASM'
	.area	_CODE
_adjacent_postinc_loop:
	ld	hl, #_bytes
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	ld	a, -4(ix)
	ld	(hl), a
	inc	-2(ix)
	jr	nz, __postinc_0
	inc	-1(ix)
__postinc_0:
	ld	hl, #_bytes
	ld	e, -2(ix)
	ld	d, -1(ix)
	add	hl, de
	ld	a, -3(ix)
	ld	(hl), a
	inc	-2(ix)
	jr	nz, __postinc_1
	inc	-1(ix)
__postinc_1:
	jr	__scan_head
__scan_head:
	cp	#4
	jr	nc, __scan_exit
__scan_inner:
	inc	a
	jr	nz, __scan_inner
	jr	_adjacent_postinc_loop
__scan_exit:
	ret
ASM

"$XOPT" -Os "$TMPDIR/adjacent_postinc_loop.s" -o "$TMPDIR/adjacent_postinc_loop.out.s"
if [[ "$(grep -Ec 'ld[[:space:]]+hl,[[:space:]]*#_bytes' "$TMPDIR/adjacent_postinc_loop.out.s")" != "1" ]]; then
    echo "xopt smoke: -Os did not combine adjacent post-increment stores across a loop" >&2
    exit 1
fi
if grep -Eq 'call[[:space:]]+__xopt_outline_' "$TMPDIR/adjacent_postinc_loop.out.s"; then
    echo "xopt smoke: -Os outlined instead of combining adjacent post-increment stores" >&2
    exit 1
fi
grep -Eq 'inc[[:space:]]+hl' "$TMPDIR/adjacent_postinc_loop.out.s"

cat >"$TMPDIR/outline_unallocated_ix.s" <<'ASM'
	.area	_CODE
_outline_unallocated_ix_a:
	call	__sdcc_enter_ix
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	a, #1
	ret
_outline_unallocated_ix_b:
	call	__sdcc_enter_ix
	ld	-2(ix), l
	ld	-1(ix), h
	ld	hl, #-4
	add	hl, sp
	ld	sp, hl
	ld	a, #2
	ret
ASM

"$XOPT" -Os "$TMPDIR/outline_unallocated_ix.s" \
    -o "$TMPDIR/outline_unallocated_ix.out.s"
if awk '
    /^_outline_unallocated_ix_[ab]:/ { in_prologue=1; next }
    in_prologue && /ld[[:space:]]+sp,[[:space:]]*hl/ { in_prologue=0 }
    in_prologue && /call[[:space:]]+__xopt_outline_/ { bad=1 }
    END { exit bad ? 0 : 1 }
' "$TMPDIR/outline_unallocated_ix.out.s"; then
    echo "xopt smoke: -Os outlined an IX spill before allocating its frame" >&2
    exit 1
fi
grep -Eq 'ld[[:space:]]+-2\(ix\),[[:space:]]*l' \
    "$TMPDIR/outline_unallocated_ix.out.s"

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
