        ; btowc.s — single byte to wide char.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module btowc
        .optsdcc -mz80 sdcccall(1)
        .globl  _btowc
        .area   _CODE
        ;; _btowc
        ;; btowc accepts either EOF (-1) or an unsigned-byte value. Any other
        ;; promoted int is outside the single-byte execution charset and maps to WEOF.
_btowc::
        bit     7,h
        jr      nz,bt_weof              ; Negative values are EOF or invalid.
        ld      a,h
        or      a
        jr      nz,bt_weof              ; Any positive value above 255 is invalid.
        ld      e,l
        ld      d,#0
        ret
bt_weof:
        ld      de,#0xffff
        ret
