        ;; sf_emit.s
        ;; Split from strftime.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sf_emit
        .optsdcc -mz80 sdcccall(1)

        .globl  __sf_emit

SF_OUT      .equ -10       ; current output pointer
SF_REM      .equ -6        ; bytes remaining (excludes the NUL slot)
SF_TRUNC    .equ -1        ; set when output was truncated

        .area   _CODE
__sf_emit::
        push    hl
        push    de
        ld      l,SF_REM(ix)
        ld      h,SF_REM + 1(ix)
        ld      d,a
        ld      a,h
        or      l
        jp      z,sf_emit_full          ; no room -> mark truncation
        dec     hl
        ld      SF_REM(ix),l
        ld      SF_REM + 1(ix),h
        ld      l,SF_OUT(ix)
        ld      h,SF_OUT + 1(ix)
        ld      (hl),d
        inc     hl
        ld      SF_OUT(ix),l
        ld      SF_OUT + 1(ix),h
        pop     de
        pop     hl
        ret
sf_emit_full:
        ld      a,#1
        ld      SF_TRUNC(ix),a
        pop     de
        pop     hl
        ret

        ; sf_emit2z: emit A (0..99) as two zero-padded digits
