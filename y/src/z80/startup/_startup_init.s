        ; Initialize RAM areas and kernel tables after ROM reset.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _startup_init
        .optsdcc -mz80 sdcccall(1)

        .globl  __startup_init
        .globl  __sys_vec_tbl
        .globl  __sys_vectors_start
        .globl  __sys_vectors_end
        .globl  __esxdos_gates_init
        .globl  __kbd_prev_scan
        .globl  _clock_sec_countdown

        .area   _CODE

        ; __startup_init, internal reset helper
        ; outputs: zeroed BSS and initialized RAM data/tables
        ; clobbers: af, bc, de, hl; preserves ix and iy
__startup_init::
        ld      bc, #l__BSS
        ld      a, b
        or      c
        jr      z, .no_bss
        ld      hl, #s__BSS
        ld      de, #s__BSS+1
        ld      (hl), #0
        dec     bc
        ld      a, b
        or      c
        jr      z, .no_bss
        ldir
.no_bss:
        ld      hl,#__kbd_prev_scan
        ld      b,#8
        ld      a,#0x1f
.keyboard_rows:
        ld      (hl),a
        inc     hl
        djnz    .keyboard_rows
        ld      a,#50
        ld      (_clock_sec_countdown),a
        call    __esxdos_gates_init

        ld      hl, #__sys_vectors_start
        ld      de, #__sys_vec_tbl
        ld      bc, #24                 ; eight three-byte JP vectors
        ldir

        ret
