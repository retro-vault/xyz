        ; CPC 464 seek hook: no seekable files are present.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module lseek
        .optsdcc -mz80 sdcccall(1)

        .globl  _lseek

        .area   _CODE
_lseek::
        ld      de,#0xffff
        ld      hl,#0xffff
        ret
