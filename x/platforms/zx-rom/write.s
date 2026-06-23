        ;; write.s  (sys backend: ZX Spectrum ROM image)
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
zxrom_write_loop:
        ld      a,b
        or      c
        jr      z,zxrom_write_done
        ld      a,(hl)
        rst     0x10
        inc     hl
        dec     bc
        jr      zxrom_write_loop
zxrom_write_done:
        ld      e,4(ix)
        ld      d,5(ix)
        pop     ix
        ret
