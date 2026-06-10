        ;; read.s  (sys backend: CP/M)
        ;;
        ;; Read stdin bytes through the BDOS console-input call.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module read
        .optsdcc -mz80 sdcccall(1)

        .globl  _read

        .area   _CODE

_read::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,4(ix)
        ld      b,5(ix)
        ld      h,d
        ld      l,e
        push    bc
cpm_read_loop:
        ld      a,b
        or      c
        jr      z,cpm_read_done
        push    bc
        ld      c,#1
        call    5
        pop     bc
        ld      (hl),a
        inc     hl
        dec     bc
        jr      cpm_read_loop
cpm_read_done:
        pop     hl
        or      a
        sbc     hl,bc
        ex      de,hl
        pop     ix
        ret
