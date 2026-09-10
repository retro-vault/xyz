        ; Close an esxDOS file; console descriptors remain available.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close
        .globl  __zx_esx_fd
        .globl  __zx_esx_errno
        .globl  __zx_esx_error
        .globl  __zx_esx_f_close

        .area   _CODE

        ; inputs: HL = fd; output: DE = 0 or -1.
        ; clobbers: af, bc, de, hl. IX/IY preserved.
_close::
        ld      a,h
        or      a
        jr      nz,.esx_close_file
        ld      a,l
        cp      #3
        jr      c,.esx_close_ok
.esx_close_file:
        call    __zx_esx_fd
        jp      c,__zx_esx_errno
        push    hl
        call    __zx_esx_f_close
        pop     hl
        jp      c,__zx_esx_error
        inc     hl
        ld      (hl),#0
.esx_close_ok:
        ld      de,#0
        ret
