        ;; bdos.s
        ;; CP/M 3 BDOS wrappers for target programs.

        .module bdos
        .optsdcc -mz80 sdcccall(1)

        .globl _bdos
        .globl _bdosret

        .equ   BDOS,5

        .area  _CODE

        ;; uint8_t bdos(uint8_t fn, uint16_t param)
_bdos::
        push    ix
        push    iy
        ld      c,a
        call    BDOS
        pop     iy
        pop     ix
        ret

        ;; bdos_ret_t *bdosret(uint8_t fn, uint16_t param, bdos_ret_t *p)
_bdosret::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        push    hl
        push    iy
        ld      c,a
        call    BDOS
        pop     iy
        ex      de,hl
        pop     hl
        ld      (hl),a
        inc     hl
        ld      (hl),b
        inc     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        pop     ix
        ret
