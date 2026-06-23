        ; wctob.s — wide char to single byte (or EOF -1).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module wctob
        .optsdcc -mz80 sdcccall(1)
        .globl  _wctob
        .area   _CODE
        ; HL = c (wint_t) -> DE = int byte value, or -1
_wctob::
        ld      a,h
        or      a
        jr      nz,wtb_eof
        ld      e,l
        ld      d,#0
        ret
wtb_eof:
        ld      de,#0xffff
        ret
