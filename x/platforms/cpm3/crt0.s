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
        .equ    CPM_CMDTAIL,0x0080
        .equ    CPM_CMDTAIL_TEXT,0x0081
        .equ    CPM_CMDTAIL_MAX,127

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
        jp      __cpm3_build_argv
__cpm3_crt0_halt:
        halt
        jr      __cpm3_crt0_halt

        ; Build a durable argc/argv from the CP/M command tail. Address 0x0080
        ; is also the default DMA buffer, so parsing it in place would leave
        ; argv strings vulnerable to the first file operation. Copy the text
        ; and exact-sized pointer table above main's stack frame instead; the
        ; stack then grows away from them. CP/M does not supply the transient
        ; program's name, so argv[0] is "".
        ;
        ; Quoted spans are grouped and their double quotes removed. Outside a
        ; quote, every ASCII control/space character is a separator. A damaged
        ; length byte is clamped to CP/M's 127-byte architectural maximum.
        ;
        ; Enters main with HL = argc and DE = argv (sdcccall(1) arguments),
        ; and also pushes argv then argc for the right-to-left sdcccall(0)
        ; stack convention. This is a terminal startup path because it
        ; deliberately moves SP.
__cpm3_build_argv:
        ld      a,(CPM_CMDTAIL)
        cp      #(CPM_CMDTAIL_MAX + 1)
        jr      c,__cpm3_tail_length_ok
        ld      a,#CPM_CMDTAIL_MAX
__cpm3_tail_length_ok:
        ld      e,a
        ld      d,#0
        inc     de                      ; tail terminator
        inc     de                      ; empty argv[0]
        ld      hl,#0
        add     hl,sp
        or      a                       ; clear carry
        sbc     hl,de
        ld      sp,hl
        push    hl
        pop     iy                      ; IY = allocation / empty argv[0]
        xor     a
        ld      0(iy),a

        ; Re-read the bounded length and copy the tail after argv[0].
        ld      a,(CPM_CMDTAIL)
        cp      #(CPM_CMDTAIL_MAX + 1)
        jr      c,__cpm3_tail_copy_length_ok
        ld      a,#CPM_CMDTAIL_MAX
__cpm3_tail_copy_length_ok:
        ld      c,a
        ld      b,#0
        ld      hl,#CPM_CMDTAIL_TEXT
        push    iy
        pop     de
        inc     de
        ld      a,b
        or      a,c
        jr      z,__cpm3_tail_copied
        ldir
__cpm3_tail_copied:
        xor     a
        ld      (de),a                  ; bounded tail is now a C string

        ld      b,#1                    ; argc always includes argv[0]
        push    iy
        pop     hl
        inc     hl                      ; read cursor
        push    iy
        pop     de
        inc     de                      ; quote-stripping write cursor

__cpm3_argv_skip_space:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_argv_done
        cp      #0x21
        jr      nc,__cpm3_argv_start
        inc     hl
        jr      __cpm3_argv_skip_space

__cpm3_argv_start:
        inc     b
        ld      c,#0                    ; bit 0: inside double quotes

__cpm3_argv_copy:
        ld      a,(hl)
        or      a
        jr      z,__cpm3_argv_end_last
        cp      #'"
        jr      z,__cpm3_argv_quote
        bit     0,c
        jr      nz,__cpm3_argv_copy_char
        cp      #0x21
        jr      c,__cpm3_argv_end_word
__cpm3_argv_copy_char:
        ld      (de),a
        inc     de
        inc     hl
        jr      __cpm3_argv_copy

__cpm3_argv_quote:
        inc     hl
        ld      a,c
        xor     #1
        ld      c,a
        jr      __cpm3_argv_copy

__cpm3_argv_end_word:
        xor     a
        ld      (de),a
        inc     de
        inc     hl
        jr      __cpm3_argv_skip_space

__cpm3_argv_end_last:
        ld      (de),a
__cpm3_argv_done:
        ; Allocate exactly argc+1 pointers beneath the copied strings.
        ld      d,b                     ; preserve argc while BC is byte count
        ld      a,b
        inc     a
        add     a,a
        ld      c,a
        ld      b,#0
        ld      hl,#0
        add     hl,sp
        or      a
        sbc     hl,bc
        ld      sp,hl
        push    hl
        pop     ix                      ; IX = argv table base
        ld      b,d
        push    ix
        pop     de                      ; table write cursor

        ; argv[0] is the empty byte at the start of the string allocation.
        push    iy
        pop     hl
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        inc     de

        ; The first pass compacted words into consecutive NUL-terminated
        ; strings, so a count-controlled second pass only has to record them.
        push    iy
        pop     hl
        inc     hl
        ld      c,b
        dec     c
__cpm3_argv_record:
        ld      a,c
        or      a
        jr      z,__cpm3_argv_record_done
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a
        inc     de
__cpm3_argv_find_end:
        ld      a,(hl)
        inc     hl
        or      a
        jr      nz,__cpm3_argv_find_end
        dec     c
        jr      __cpm3_argv_record

__cpm3_argv_record_done:
        xor     a
        ld      (de),a
        inc     de
        ld      (de),a                  ; argv[argc] = NULL
        ld      l,b
        ld      h,#0
        push    ix
        pop     de
        push    de                      ; argv for sdcccall(0)
        push    hl                      ; argc for sdcccall(0)
        call    _main
        pop     bc                      ; discard stack argc without touching DE
        pop     bc                      ; discard stack argv without touching DE
        ex      de,hl                   ; exit(int) takes its argument in HL
        call    _exit
        jp      __cpm3_crt0_halt

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
