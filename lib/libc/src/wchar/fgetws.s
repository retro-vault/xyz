        ;; fgetws.s
        ;;
        ;; The target's execution charset is single-byte, so fgetws can read
        ;; through fgetc and widen each byte into a 16-bit code unit. Newlines
        ;; are preserved exactly like fgets.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module fgetws
        .optsdcc -mz80 sdcccall(1)

        .globl  _fgetws
        .globl  _fgetc

        .area   _DATA
__fgetws_start:
        .dw     0
__fgetws_ptr:
        .dw     0
__fgetws_stream:
        .dw     0
__fgetws_left:
        .dw     0
__fgetws_any:
        .db     0

        .area   _CODE

_fgetws::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      (__fgetws_start),hl
        ld      (__fgetws_ptr),hl
        ld      (__fgetws_left),de
        ld      l,4(ix)
        ld      h,5(ix)
        ld      (__fgetws_stream),hl
        xor     a
        ld      (__fgetws_any),a

        ld      a,h
        or      l
        jr      z,__fgetws_fail
        ld      hl,(__fgetws_left)
        ld      a,h
        or      l
        jr      z,__fgetws_fail
        dec     hl                      ; Reserve one wchar_t slot for NUL.
        ld      (__fgetws_left),hl
        ld      a,h
        or      l
        jr      nz,__fgetws_loop
        jr      __fgetws_terminate

__fgetws_loop:
        ld      hl,(__fgetws_stream)
        call    _fgetc
        ld      a,d
        cp      #0xff
        jr      nz,__fgetws_store
        ld      a,e
        cp      #0xff
        jr      z,__fgetws_eof
__fgetws_store:
        ld      a,#1
        ld      (__fgetws_any),a
        ld      hl,(__fgetws_ptr)
        ld      (hl),e
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      (__fgetws_ptr),hl
        ld      a,e
        cp      #'\n'
        jr      z,__fgetws_terminate
        ld      hl,(__fgetws_left)
        dec     hl
        ld      (__fgetws_left),hl
        ld      a,h
        or      l
        jr      nz,__fgetws_loop
        jr      __fgetws_terminate

__fgetws_eof:
        ld      a,(__fgetws_any)
        or      a
        jr      nz,__fgetws_terminate
__fgetws_fail:
        ld      de,#0x0000
        pop     ix
        ret

__fgetws_terminate:
        ld      hl,(__fgetws_ptr)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      de,(__fgetws_start)
        pop     ix
        ret
