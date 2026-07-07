        ;; syscall_table.s
        ;;
        ;; Populate the YOS service table during GSINIT from a compact ROM
        ;; template, and provide tiny ABI bridge helpers for entries whose
        ;; public signatures differ from the kernel internals.

        .module syscall_table

        .globl  __yos
        .globl  __heap
        .globl  _yos_version
        .globl  _enter_critical_section
        .globl  _leave_critical_section
        .globl  _tty_printf
        .globl  _tty_puts
        .globl  _tty_gets
        .globl  _tty_cls
        .globl  _tty_getc
        .globl  _tty_cur_enable
        .globl  _tty_attr
        .globl  _mem_allocate
        .globl  _mem_free
        .globl  __clock
        .globl  _tmr_install
        .globl  _tmr_uninstall
        .globl  _mdr_detect_drives
        .globl  _mdr_format
        .globl  _mdr_dir
        .globl  _mdr_load
        .globl  _mdr_save
        .globl  _strlen
        .globl  _strcpy
        .globl  _strcmp
        .globl  _tolower

        .equ    YOS_TABLE_SIZE, 52

        .area   _GSINIT
        ld      hl, #__yos_template
        ld      de, #__yos
        ld      bc, #YOS_TABLE_SIZE
        ldir

        .area   _CODE

_yos_malloc:
        ex      de, hl
        ld      bc, #0
        push    bc
        ld      hl, #__heap
        jp      _mem_allocate

_yos_free:
        ex      de, hl
        ld      hl, #__heap
        jp      _mem_free

_yos_clock:
        jp      __clock

_yos_isalpha:
        ld      a, l
        and     #0xDF
        sub     #'A'
        cp      #26
        ld      de, #0
        ret     nc
        inc     e
        ret

_yos_isspace:
        ld      a, l
        cp      #' '
        jr      z, .isspace_true
        sub     #9
        cp      #5
        ld      de, #0
        ret     nc
.isspace_true:
        ld      de, #1
        ret

_yos_install_timer:
        ld      bc, #0
        push    bc
        jp      _tmr_install

_yos_uninstall_timer:
        jp      _tmr_uninstall

        .area   _CONST
__yos_template:
        .dw     _yos_version
        .dw     _enter_critical_section
        .dw     _leave_critical_section
        .dw     _yos_install_timer
        .dw     _yos_uninstall_timer
        .dw     _tty_printf
        .dw     _tty_puts
        .dw     _tty_gets
        .dw     _tty_cls
        .dw     _tty_getc
        .dw     _tty_cur_enable
        .dw     _tty_attr
        .dw     _yos_malloc
        .dw     _yos_free
        .dw     _yos_clock
        .dw     _mdr_detect_drives
        .dw     _mdr_format
        .dw     _mdr_dir
        .dw     _mdr_load
        .dw     _mdr_save
        .dw     _strlen
        .dw     _strcpy
        .dw     _strcmp
        .dw     _yos_isalpha
        .dw     _yos_isspace
        .dw     _tolower
