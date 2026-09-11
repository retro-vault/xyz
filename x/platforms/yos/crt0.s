        ;; crt0.s -- relocatable YOS XPRG process startup.
        ;;
        ;; The kernel supplies the process stack.  This startup initializes C
        ;; storage, resolves the ABI-8 "yos" table through RST 18, calls main,
        ;; and terminates the current process through that table.

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _entry
        .globl  __exit
        .globl  _query_service
        .globl  _yos_api_table
        .globl  s__BSS
        .globl  s__HEAP
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER

        .area   _CODE
_entry::
        call    .gsinit
        ld      hl,#.yos_name
        call    _query_service
        ld      (_yos_api_table),de
        call    _main
        ex      de,hl
        jp      __exit

.yos_name:
        .asciz  "yos"

        .area   _GSINIT
.gsinit:
        ld      hl,#s__BSS
        ld      de,#s__HEAP
.zero_bss:
        ld      a,h
        cp      d
        jr      nz,.clear_byte
        ld      a,l
        cp      e
        jr      z,.no_bss
.clear_byte:
        ld      (hl),#0
        inc     hl
        jr      .zero_bss
.no_bss:
        ld      de,#s__INITIALIZED
        ld      bc,#s__BSS
        ld      hl,#s__INITIALIZER
.copy_init:
        ld      a,d
        cp      b
        jr      nz,.copy_byte
        ld      a,e
        cp      c
        jr      z,.no_init
.copy_byte:
        ldi
        jr      .copy_init
.no_init:
        .area   _GSFINAL
        ret

        .area   _DATA
        .area   _INITIALIZED
        .area   _BSS
        .area   _HEAP
        .area   _INITIALIZER
