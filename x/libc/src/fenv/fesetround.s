        ; fesetround.s — set the rounding mode (FE_TONEAREST..FE_TOWARDZERO).
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module fesetround
        .optsdcc -mz80 sdcccall(1)
        .globl  _fesetround
        .globl  __fe_current_env
        .area   _CODE
        ; HL = round -> DE = 0 on success, 1 if invalid
_fesetround::
        ld      a,h
        or      a
        jr      nz,fsr_err
        ld      a,l
        cp      #4
        jr      nc,fsr_err
        ld      (__fe_current_env + 2),hl
        ld      de,#0
        ret
fsr_err:
        ld      de,#1
        ret
