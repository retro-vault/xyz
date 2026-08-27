        ; CPC 464 descriptor close hook.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE
_close::
        ld      a,h
        or      a
        jr      nz,.cpc_close_fail
        ld      a,l
        cp      #3
        jr      nc,.cpc_close_fail
        ld      de,#0
        ret
.cpc_close_fail:
        ld      de,#0xffff
        ret
