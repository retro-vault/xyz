        ;; sys_sbrk.s  (sys backend: none / bare metal)
        ;;
        ;; Bare-metal heap break for the "none" backend. This exposes a fixed
        ;; 8 KB arena through the classic sbrk contract used by libc malloc.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module sys_sbrk
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_sbrk
        .globl  ___sys_sbrk

SYS_HEAP_SIZE    .equ 8192

        .area   _HEAP
__sys_heap:
        .ds     SYS_HEAP_SIZE
__sys_heap_end:
        .db     0

        .area   _DATA
__sys_brk:
        .dw     __sys_heap

        .area   _CODE

__sys_sbrk:
___sys_sbrk::
        ld      de,(__sys_brk)          ; DE = current break, also return value
        push    de
        ex      de,hl                   ; HL = current break, DE = increment
        add     hl,de                   ; HL = requested next break
        ld      de,#__sys_heap
        ld      a,l
        sub     e
        ld      a,h
        sbc     a,d
        ;; Reject requests that would move the break below the arena base.
        jr      c,sys_sbrk_fail
        ld      de,#__sys_heap_end
        ld      a,e
        sub     l
        ld      a,d
        sbc     a,h
        ;; Reject requests that would step beyond the arena end marker.
        jr      c,sys_sbrk_fail
        ld      (__sys_brk),hl
        pop     de
        ret

sys_sbrk_fail:
        pop     hl
        ld      de,#0xffff
        ret
