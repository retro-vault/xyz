        ; localeconv.s — return the built-in "C" formatting data.
        ; MIT License (see: LICENSE)  Copyright (C) 2026 tomaz stih
        .module localeconv
        .optsdcc -mz80 sdcccall(1)
        .globl  _localeconv
        .globl  __locale_c
        .area   _CODE
_localeconv::
        ld      de,#__locale_c
        ret
