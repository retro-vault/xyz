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

        .area   _CODE

FG_START  .equ 0
FG_PTR    .equ 2
FG_STREAM .equ 4
FG_LEFT   .equ 6
FG_ANY    .equ 8

_fgetws::
        ld      b,h
        ld      c,l
        push    ix
        ld      hl,#0
        push    hl
        push    hl
        push    hl
        push    hl
        push    hl
        ld      ix,#0
        add     ix,sp
        ld      FG_START+0(ix),c
        ld      FG_START+1(ix),b
        ld      FG_PTR+0(ix),c
        ld      FG_PTR+1(ix),b
        ld      FG_LEFT+0(ix),e
        ld      FG_LEFT+1(ix),d
        ld      l,14(ix)
        ld      h,15(ix)
        ld      FG_STREAM+0(ix),l
        ld      FG_STREAM+1(ix),h
        xor     a
        ld      FG_ANY(ix),a

        ld      l,FG_START+0(ix)
        ld      h,FG_START+1(ix)
        ld      a,h
        or      l
        jr      z,__fgetws_fail
        ld      l,FG_LEFT+0(ix)
        ld      h,FG_LEFT+1(ix)
        ld      a,h
        or      l
        jr      z,__fgetws_fail
        dec     hl                      ; Reserve one wchar_t slot for NUL.
        ld      FG_LEFT+0(ix),l
        ld      FG_LEFT+1(ix),h
        ld      a,h
        or      l
        jr      nz,__fgetws_loop
        jr      __fgetws_terminate

__fgetws_loop:
        ld      l,FG_STREAM+0(ix)
        ld      h,FG_STREAM+1(ix)
        call    _fgetc
        ld      a,d
        cp      #0xff
        jr      nz,__fgetws_store
        ld      a,e
        cp      #0xff
        jr      z,__fgetws_eof
__fgetws_store:
        ld      a,#1
        ld      FG_ANY(ix),a
        ld      l,FG_PTR+0(ix)
        ld      h,FG_PTR+1(ix)
        ld      (hl),e
        inc     hl
        xor     a
        ld      (hl),a
        inc     hl
        ld      FG_PTR+0(ix),l
        ld      FG_PTR+1(ix),h
        ld      a,e
        cp      #'\n'
        jr      z,__fgetws_terminate
        ld      l,FG_LEFT+0(ix)
        ld      h,FG_LEFT+1(ix)
        dec     hl
        ld      FG_LEFT+0(ix),l
        ld      FG_LEFT+1(ix),h
        ld      a,h
        or      l
        jr      nz,__fgetws_loop
        jr      __fgetws_terminate

__fgetws_eof:
        ld      a,FG_ANY(ix)
        or      a
        jr      nz,__fgetws_terminate
__fgetws_fail:
        ld      de,#0x0000
        ld      sp,ix
        pop     hl
        pop     hl
        pop     hl
        pop     hl
        pop     hl
        pop     ix
        ret

__fgetws_terminate:
        ld      l,FG_PTR+0(ix)
        ld      h,FG_PTR+1(ix)
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      e,FG_START+0(ix)
        ld      d,FG_START+1(ix)
        ld      sp,ix
        pop     hl
        pop     hl
        pop     hl
        pop     hl
        pop     hl
        pop     ix
        ret
