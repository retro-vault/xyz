        ;; libc_register_handler.s
        ;; Split from exit_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_register_handler
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_register_handler

ATEXIT_SLOTS     .equ 8

        .area   _CODE
__libc_register_handler::
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

