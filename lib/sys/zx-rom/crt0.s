        ; crt0.s  (ZX ROM image startup)
        ;
        ; Minimal ZX Spectrum ROM-style startup. Code is intended to run
        ; from address 0x0000 while writable data lives in RAM. The stack
        ; is placed at the top of RAM. This startup clears BSS and copies
        ; ROM initializers to RAM for targets that emit the classic
        ; _INITIALIZER -> _INITIALIZED split.
        ;
        ; Important current limitation:
        ; xcc still emits ordinary writable globals into _DATA rather than
        ; splitting initialized objects into _INITIALIZER / _INITIALIZED.
        ; So this backend now has the right startup model, but fully correct
        ; ROM + RAM initialized-data support still needs the compiler-side
        ; data split.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _entry
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER

        .area   _CODE
_entry::
        di
        ld      sp,#0xffff
        call    .gsinit
        call    _main
halt_loop:
        halt
        jr      halt_loop

        .area   _GSINIT
.gsinit:
        ld      bc,#l__BSS
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_bss
        ld      hl,#s__BSS
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),#0
        dec     bc
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_bss
        ldir
.gsinit_no_bss:
        ld      de,#s__INITIALIZED
        ld      hl,#s__INITIALIZER
        ld      bc,#l__INITIALIZER
        ld      a,b
        or      a,c
        jr      z,.gsinit_no_init
        ldir
.gsinit_no_init:
        .area   _GSFINAL
        ret

        ; ROM-resident sections first.
        .area   _CONST
        .area   _INITIALIZER

        ; RAM-resident writable sections after the ROM image.
        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
