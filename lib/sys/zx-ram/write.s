        ;; write.s  (sys backend: ZX Spectrum RAM program)
        ;;
        ;; Forward stdout/stderr bytes to the ROM character-output routine.
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
        ld      c,4(ix)
        ld      b,5(ix)
        ld      h,d
        ld      l,e
zxram_write_loop:
        ld      a,b
        or      c
        jr      z,zxram_write_done
        ld      a,(hl)
        rst     0x10
        inc     hl
        dec     bc
        jr      zxram_write_loop
zxram_write_done:
        ld      e,4(ix)
        ld      d,5(ix)
        pop     ix
        ret
