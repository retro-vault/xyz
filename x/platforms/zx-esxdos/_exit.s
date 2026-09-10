        ; esxDOS program termination and descriptor cleanup.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __exit
        .globl  _close
        .globl  _zx_exit_status

        .area   _CODE
        ; Close every disk descriptor before halting. Failed closes do
        ; not prevent attempting the remaining descriptors.

        ; __exit
        ; inputs: HL = exit status (sdcccall(1)).
        ; outputs: does not return; closes disk descriptors and halts.
        ; clobbers: all registers and flags.
__exit::
        ld      (_zx_exit_status),hl
        di
        ; 16 descriptors, starting at fd 3
        ld      bc,#0x1003
.zx_exit_close:
        ld      h,#0
        ld      l,c
        push    bc
        call    _close
        pop     bc
        inc     c
        djnz    .zx_exit_close
.zx_exit_halt:
        halt
        jr      .zx_exit_halt

        .area   _BSS
_zx_exit_status::
        .ds     2
