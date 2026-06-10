        ;; write.s  (sys backend: CP/M)
        ;;
        ;; Forward stdout/stderr bytes to the BDOS console-output call.
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
cpm_write_loop:
        ld      a,b
        or      c
        jr      z,cpm_write_done
        ld      e,(hl)
        ld      c,#2
        call    5
        inc     hl
        dec     bc
        jr      cpm_write_loop
cpm_write_done:
        ld      e,4(ix)
        ld      d,5(ix)
        pop     ix
        ret
