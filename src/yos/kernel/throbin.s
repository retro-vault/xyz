        ;; throbin.s
        ;; 
        ;; zx spectrum threar round robin algorithm
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
        ;; 2021-08-07   tstih
        .module throbin

        .globl __thread_robin

        .area   _CODE

__thread_robin::
        ;; no interrupts
         di
        ;; we will need af and hl
        push    af
        push    hl
        ld      hl, (_thread_current)
        ld      a,h
        or      l
        jr      z, trbn_no_current_thread
        ;; if current thread, store its context!
        ;; dont forget that af and hl are already on stack
        push    bc
        push    de
        push    ix
        push    iy
        ex      af,af'
        push    af
        ex      af,af'
        exx
        push    bc
        push    de
        push    hl
        exx 
        ;; hl alredy has current thread, skip over header
        inc     hl
        inc     hl
        inc     hl 
        inc     hl
        ;; hl now points to stack pointer of thread_t
        ;; store register sp into it 
        ld      de,#0                   ; 0 to de
        ex      de,hl                   ; hl=0, de=sp var
        add     hl,sp                   ; hl=sp
        ex      de,hl                   ; change back
        ld      (hl),e
        inc     hl
        ld      (hl),d
        jr      trbn_exec               ; execute commands
trbn_no_current_thread:
        ;; clean stack
        pop     hl
        pop     af
trbn_exec:
        ;; chain pending timers
        call    __tmr_chain
        ;; select next into hl
        call    __thread_select_next
        ld      a,h
        or      l
        jr      z, trb_no_next_thread
        ld      (_thread_current),hl    ; store current thread
        ;; skip over header
        inc     hl  
        inc     hl  
        inc     hl 
        inc     hl
        ;; and restore stack 
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      sp,hl
        ;; restore all registers
        exx
        pop     hl 
        pop     de
        pop     bc
        exx
        ex      af,af'
        pop     af
        ex      af,af'
        pop     iy
        pop     ix
        pop     de
        pop     bc
        pop     hl
        pop     af
trb_no_next_thread:
        ;; and jump to next or idle if no next thread
        ei
        reti