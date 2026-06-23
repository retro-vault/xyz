        ; signal.s
        ;
        ; libc signal() for the xcc Z80 libc.  Installs a disposition for a
        ; valid signal and returns the previous one (or SIG_ERR == -1).
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module signal
        .optsdcc -mz80 sdcccall(1)
        .globl  _signal
        .globl  __signal_handlers
        .area   _CODE

        ; _signal
        ; inputs:  HL = sig, DE = func (handler pointer)
        ; outputs: DE = previous handler, or 0xFFFF (SIG_ERR) if sig invalid
_signal::
        ld      a,h
        or      a
        jr      nz,sig_err
        ld      a,l
        or      a
        jr      z,sig_err               ; 0 invalid
        cp      #7
        jr      nc,sig_err              ; >= 7 invalid
        add     a,a                     ; sig * 2 (entry size)
        ld      c,a
        ld      b,#0
        ld      hl,#__signal_handlers
        add     hl,bc                   ; HL = &table[sig]
        ld      c,(hl)
        inc     hl
        ld      b,(hl)                  ; BC = old handler
        ld      (hl),d
        dec     hl
        ld      (hl),e                  ; table[sig] = func
        ld      d,b
        ld      e,c                     ; DE = old handler
        ret
sig_err:
        ld      de,#0xffff              ; SIG_ERR
        ret
