        ;; write.s  (sys backend: generic z80)
        ;;
        ;; No default console is available, so writes report success without
        ;; emitting bytes anywhere.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module write
        .optsdcc -mz80 sdcccall(1)

        .globl  _write

        .area   _CODE

_write::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      e,4(ix)
        ld      d,5(ix)
        pop     ix
        ret
