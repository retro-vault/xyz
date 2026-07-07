        ;; thread_prepare_startup.s
        ;;
        ;; Compact thread startup stub emitter.
        ;;
        ;; Signature:
        ;;   void thread_prepare_startup(thread_t *t,
        ;;                               void (*entry_point)(void))
        ;;
        ;; Inputs (sdcccall1):
        ;;   HL = t
        ;;   DE = entry_point

        .module thread_prepare_startup

        .globl  _thread_prepare_startup
        .globl  _thread_exit

        .equ    THREAD_SP_OFF,      4
        .equ    THREAD_STARTUP_OFF, 6
        .equ    CONTEXT_RET_OFF,   20

        .area   _CODE

_thread_prepare_startup:
        push    ix

        ld      b,h
        ld      c,l
        push    hl
        pop     ix

        ld      6(ix), #0xCD        ; call entry_point
        ld      7(ix), e
        ld      8(ix), d
        ld      9(ix), #0x21        ; ld hl, t
        ld      10(ix), c
        ld      11(ix), b
        ld      12(ix), #0xC3       ; jp thread_exit

        ld      hl, #_thread_exit
        ld      13(ix), l
        ld      14(ix), h
        ld      15(ix), #0x00       ; padding / guard byte

        ld      l, 4(ix)
        ld      h, 5(ix)
        ld      de, #CONTEXT_RET_OFF
        add     hl, de              ; HL = initial return-address slot
        push    hl

        ld      l, c
        ld      h, b
        ld      de, #THREAD_STARTUP_OFF
        add     hl, de              ; HL = &t->startup[0]
        ex      de, hl              ; DE = startup address
        pop     hl                  ; HL = return-address slot

        ld      (hl), e
        inc     hl
        ld      (hl), d

        pop     ix
        ret
