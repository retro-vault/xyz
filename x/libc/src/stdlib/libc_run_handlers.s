        ;; libc_run_handlers.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_run_handlers
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_run_handlers
        .globl  __sdcc_call_bc

        .area   _CODE
__libc_run_handlers::
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
