        ; btowc.s — single byte to wide char.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module btowc
        .optsdcc -mz80 sdcccall(1)
        .globl  _btowc
        .area   _CODE
        ; HL = c (int) -> DE = wint_t, or WEOF (0xFFFF) if out of byte range
_btowc::
        bit     7,h
        jr      nz,bt_weof              ; c < 0
        ld      a,h
        or      a
        jr      nz,bt_weof              ; c > 255
        ld      e,l
        ld      d,#0
        ret
bt_weof:
        ld      de,#0xffff
        ret
