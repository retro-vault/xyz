        ; crt0.s  (CP/M 3 COM startup)
        ;
        ; Minimal CP/M 3 startup: place the stack near the top of the transient
        ; program area, copy ROM-style initializers when present, then invoke
        ; main() and route its return value through libc exit().
        ;
        ; Important current limitation:
        ; xcc still emits initialized writable globals into `_DATA` rather than
        ; splitting them into `_INITIALIZER` / `_INITIALIZED`, so this startup
        ; can correctly handle `_BSS` and split initializers, but it cannot yet
        ; reconstruct initialized `_DATA` values on its own.

        .module crt0
        .optsdcc -mz80 sdcccall(1)

        .globl  _main
        .globl  _exit
        .globl  _entry
        .globl  __cpm3_entry_sp
        .globl  s__BSS
        .globl  l__BSS
        .globl  s__INITIALIZED
        .globl  s__INITIALIZER
        .globl  l__INITIALIZER

        .equ    BDOS,5
        .equ    S_BDOSVER,12
        .equ    CPM3_BDOSERR,45
        .equ    CPM3_BDOSERR_RETURN,0xff
        .equ    CPM3_VERSION_MIN,0x31

        .area   _CODE
_entry::
        ld      (__cpm3_entry_sp),sp    ; preserve CCP stack for RET-to-CCP exit
        ld      hl,(0x0006)             ; BDOS entry = lowest BDOS byte;
        ld      sp,hl                   ; stack grows down from below BDOS
        call    .gsinit
        push    ix
        push    iy
        push    bc
        push    de
        ld      c,#S_BDOSVER
        call    BDOS
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ld      a,l
        cp      #CPM3_VERSION_MIN
        jr      c,__cpm3_crt0_main
        push    ix
        push    iy
        ld      e,#CPM3_BDOSERR_RETURN
        ld      c,#CPM3_BDOSERR
        call    BDOS
        pop     iy
        pop     ix
__cpm3_crt0_main:
        call    _main
        call    _exit
__cpm3_crt0_halt:
        halt
        jr      __cpm3_crt0_halt

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

        .area   _DATA
__cpm3_entry_sp::
        .dw     0
        .area   _INITIALIZED
        .area   _BSS
        .area   _INITIALIZER
        .area   _HEAP                   ; last: __heap_base marks top of image
