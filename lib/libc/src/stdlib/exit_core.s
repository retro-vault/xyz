        ;; exit_core.s
        ;;
        ;; Hand-written process-termination helpers for the xcc Z80 libc.
        ;; The handler tables remain tiny and deterministic, and the final
        ;; platform hand-off goes through __sys_exit so each backend chooses
        ;; how "process termination" maps to its machine.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module exit_core
        .optsdcc -mz80 sdcccall(1)

        .globl  _abort
        .globl  _atexit
        .globl  _exit
        .globl  __Exit
        .globl  _at_quick_exit
        .globl  _quick_exit
        .globl  __libc_exit_status
        .globl  __libc_exit_kind
        .globl  ___sys_exit
        .globl  __sdcc_call_bc

ATEXIT_SLOTS     .equ 8
QUICKEXIT_SLOTS  .equ 8

        .area   _DATA

__libc_atexit_handlers:
        .ds     (ATEXIT_SLOTS * 2)
__libc_atexit_count:
        .dw     0
__libc_quick_exit_handlers:
        .ds     (QUICKEXIT_SLOTS * 2)
__libc_quick_exit_count:
        .dw     0
__libc_exit_status:
        .dw     0
__libc_exit_kind:
        .dw     0

        .area   _CODE

;; Walk handler tables in reverse registration order, matching the C rules.
;;   HL = handler table base
;;   DE = handler count
__libc_run_handlers:
        ld      b,h
        ld      c,l                     ; BC = handler table base
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-4
        add     hl,sp
        ld      sp,hl
        ld      -4(ix),c
        ld      -3(ix),b
        ld      -2(ix),e
        ld      -1(ix),d
run_handlers_loop:
        ld      a,-1(ix)
        or      -2(ix)
        jr      z,run_handlers_done
        ld      e,-2(ix)
        ld      d,-1(ix)
        dec     de
        ld      -2(ix),e
        ld      -1(ix),d
        ex      de,hl
        add     hl,hl                   ; offset = count * sizeof(void (*)(void))
        ld      e,-4(ix)
        ld      d,-3(ix)
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        call    __sdcc_call_bc
        jr      run_handlers_loop
run_handlers_done:
        ld      sp,ix
        pop     ix
        ret

;; Shared registration path for atexit() and at_quick_exit().
;;   HL = function pointer
;;   DE = table base
;;   BC = address of count cell
;; returns DE = 0 on success, 1 on rejection
__libc_register_handler:
        ld      a,h
        or      l
        jr      z,register_fail
        push    hl                      ; save callback pointer
        push    bc
        ld      a,(bc)
        ld      l,a
        inc     bc
        ld      a,(bc)
        ld      h,a                     ; HL = current count
        pop     bc
        ld      a,h
        or      a
        jr      nz,register_fail_pop
        ld      a,l
        cp      #ATEXIT_SLOTS
        jr      nc,register_fail_pop

        add     hl,hl
        add     hl,de                   ; HL = slot address
        ex      de,hl                   ; DE = slot address
        pop     hl                      ; HL = callback pointer
        ld      a,l
        ld      (de),a
        inc     de
        ld      a,h
        ld      (de),a

        push    bc
        ld      a,(bc)
        inc     a
        ld      (bc),a
        jr      nz,register_count_done
        inc     bc
        ld      a,(bc)
        inc     a
        ld      (bc),a
register_count_done:
        pop     bc

        ld      de,#0
        ret

register_fail_pop:
        pop     hl
register_fail:
        ld      de,#1
        ret

_abort::
        ld      hl,#2
        ld      (__libc_exit_kind),hl
        ld      hl,#1
        ld      (__libc_exit_status),hl
        call    ___sys_exit
abort_halt:
        jr      abort_halt

_atexit::
        ld      de,#__libc_atexit_handlers
        ld      bc,#__libc_atexit_count
        jp      __libc_register_handler

_exit::
        push    hl
        ld      hl,#__libc_atexit_handlers
        ld      de,(__libc_atexit_count)
        call    __libc_run_handlers
        pop     hl
        ld      (__libc_exit_status),hl
        ld      hl,#1
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    ___sys_exit
exit_halt:
        jr      exit_halt

__Exit::
        ld      (__libc_exit_status),hl
        ld      hl,#3
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    ___sys_exit
_Exit_halt:
        jr      _Exit_halt

_at_quick_exit::
        ld      de,#__libc_quick_exit_handlers
        ld      bc,#__libc_quick_exit_count
        jp      __libc_register_handler

_quick_exit::
        push    hl
        ld      hl,#__libc_quick_exit_handlers
        ld      de,(__libc_quick_exit_count)
        call    __libc_run_handlers
        pop     hl
        ld      (__libc_exit_status),hl
        ld      hl,#4
        ld      (__libc_exit_kind),hl
        ld      hl,(__libc_exit_status)
        call    ___sys_exit
quick_exit_halt:
        jr      quick_exit_halt
