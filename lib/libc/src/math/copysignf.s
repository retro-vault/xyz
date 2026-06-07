        ; copysignf.s — magnitude of x with the sign of y.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module copysignf
        .optsdcc -mz80 sdcccall(1)
        .globl  _copysignf
        .globl  _copysign
        .globl  _copysignl
        .area   _CODE
        ; HL:DE = mag, sign at 4(ix)..7(ix) -> HL:DE = result
_copysign::
_copysignl::
_copysignf::
        push    ix
        ld      ix,#0
        add     ix,sp
        res     7,h                     ; clear mag's sign
        bit     7,7(ix)                 ; sign's sign bit (a3)
        jr      z,cps_done
        set     7,h
cps_done:
        pop     ix
        ret
