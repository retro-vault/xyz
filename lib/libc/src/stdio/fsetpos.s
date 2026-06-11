        ;; fsetpos.s
        ;;
        ;; Public fsetpos() entry point. The current FILE model is byte-
        ;; addressed and unbuffered, so an fpos_t is simply the underlying long
        ;; file offset used by fseek().
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fsetpos
        .optsdcc -mz80 sdcccall(1)

        .globl  _fsetpos
        .globl  _fseek

        SEEK_SET_V      .equ 0x0000

        .area   _CODE

        ;; _fsetpos
        ;; sdcccall(1) ABI:
        ;;   HL = FILE *stream
        ;;   DE = const fpos_t *pos
        ;;
        ;; Return 0 on success, -1 on error.
_fsetpos::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      b,h
        ld      c,l
        ld      hl,#-6
        add     hl,sp
        ld      sp,hl
        ld      -6(ix),c
        ld      -5(ix),b
        ld      a,d
        or      e
        jr      z,__stdio_fsetpos_fail ; Null source position is treated as failure.
        ex      de,hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      -4(ix),e
        ld      -3(ix),d
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      -2(ix),e
        ld      -1(ix),d

        ld      bc,#SEEK_SET_V
        push    bc
        ld      l,-2(ix)
        ld      h,-1(ix)
        push    hl
        ld      l,-4(ix)
        ld      h,-3(ix)
        push    hl
        ld      l,-6(ix)
        ld      h,-5(ix)
        call    _fseek
        pop     bc
        pop     bc
        pop     bc
        ld      sp,ix
        pop     ix
        ret

__stdio_fsetpos_fail:
        ld      sp,ix
        pop     ix
        ld      hl,#0xffff
        push    hl
        pop     de
        ret
