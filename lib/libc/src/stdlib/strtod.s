        ;; strtod.s
        ;;
        ;; Public double parser wrapper around the shared decimal core.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module strtod
        .optsdcc -mz80 sdcccall(1)

        .globl  _strtod
        .globl  __strtod_core

        .area   _CODE

_strtod::
        jp      __strtod_core

        .globl  _strfromd
        .globl  __strfromd_core

; strfromd(s, n, format, fp) -- new C23, implemented by editing this existing file.
; We load the fp double into the 64-bit reg layout expected by the core, then call.
_strfromd::
        push    ix
        ld      ix,#0
        add     ix,sp
        ; The fp is the last arg (8 bytes stacked). Load into DE:HL:DE':HL' convention.
        ld      e,12(ix)
        ld      d,13(ix)
        ld      l,14(ix)
        ld      h,15(ix)
        exx
        ld      e,16(ix)
        ld      d,17(ix)
        ld      l,18(ix)
        ld      h,19(ix)
        exx
        call    __strfromd_core
        pop     ix
        ret
