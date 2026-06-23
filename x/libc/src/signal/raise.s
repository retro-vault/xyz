        ; raise.s
        ;
        ; libc raise() for the xcc Z80 libc.  Runs the installed disposition
        ; for sig synchronously: SIG_IGN is a no-op, SIG_DFL aborts, otherwise
        ; the handler is called with sig.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module raise
        .optsdcc -mz80 sdcccall(1)
        .globl  _raise
        .globl  __signal_handlers
        .globl  _abort
        .area   _CODE

        ; _raise
        ; inputs:  HL = sig
        ; outputs: DE = 0 on success, 1 if sig invalid
_raise::
        ld      a,h
        or      a
        jr      nz,raise_bad
        ld      a,l
        or      a
        jr      z,raise_bad
        cp      #7
        jr      nc,raise_bad
        ld      b,h                     ; BC = validated signal value
        ld      c,l
        ld      a,c
        add     a,a
        ld      l,a
        ld      h,#0
        ld      de,#__signal_handlers
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; DE = handler
        ld      a,d
        or      a
        jr      nz,raise_call           ; high byte set -> real handler
        ld      a,e
        cp      #1
        jr      z,raise_ign             ; SIG_IGN
        or      a
        jr      z,raise_dfl             ; SIG_DFL
raise_call:
        ld      hl,#raise_after
        push    hl                      ; return address for the handler
        push    de                      ; handler address
        ld      l,c
        ld      h,b                     ; HL = sig (argument)
        ret                             ; jump into handler; it returns to raise_after
raise_after:
        ld      de,#0
        ret
raise_ign:
        ld      de,#0
        ret
raise_dfl:
        jp      _abort
raise_bad:
        ld      de,#1
        ret
