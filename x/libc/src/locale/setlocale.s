        ; setlocale.s
        ;
        ; libc setlocale() for the xcc Z80 libc.  Only the built-in "C" (and
        ; equivalently "POSIX" or "") locale is selectable; queries and valid
        ; selections return the locale name, everything else returns NULL.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module setlocale
        .optsdcc -mz80 sdcccall(1)
        .globl  _setlocale
        .globl  _strcmp
        .globl  __locale_name
        .globl  __locale_posix
        .area   _CODE

        ; _setlocale
        ; inputs:  HL = category, DE = locale
        ; outputs: DE = locale name pointer, or 0
_setlocale::
        ld      a,h
        or      a
        jr      nz,sl_null              ; category > 255 -> invalid
        ld      a,l
        cp      #6
        jr      nc,sl_null              ; category >= 6 -> invalid
        ld      a,d
        or      e
        jr      z,sl_name               ; locale == NULL -> query
        ld      a,(de)
        or      a
        jr      z,sl_name               ; locale[0] == '\0' -> "C"
        ; strcmp(locale, "C")
        ld      h,d
        ld      l,e                     ; HL = locale
        push    hl
        ld      de,#__locale_name
        call    _strcmp
        ld      a,d
        or      e
        pop     hl
        jr      z,sl_name
        ; strcmp(locale, "POSIX")
        push    hl
        ld      de,#__locale_posix
        call    _strcmp
        ld      a,d
        or      e
        pop     hl
        jr      z,sl_name
sl_null:
        ld      de,#0
        ret
sl_name:
        ld      de,#__locale_name
        ret
