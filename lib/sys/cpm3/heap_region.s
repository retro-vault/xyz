        ;; heap_region.s  (sys backend: CP/M 3)
        ;;
        ;; Reports the region the default libc heap should manage.  The heap
        ;; grows from the end of the program image (__heap_base, the base of the
        ;; _HEAP area, which crt0 places last) up to the bottom of the BDOS
        ;; (stored by CP/M in the word at 0x0006), less a fixed reserve held
        ;; back for the descending stack (crt0 sets SP to the BDOS base).
        ;;
        ;; This replaces sbrk: nothing is statically reserved, so the heap
        ;; scales to whatever the transient program area provides.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module heap_region
        .optsdcc -mz80 sdcccall(1)

        .globl  _heap_region

        ;; Bytes held below the BDOS for the descending stack.  Tunable.
SYS_STACK_RESERVE   .equ 0x0200

        .area   _HEAP
__heap_base:                             ; heap base = top of program image

        .area   _CODE
        ;; void heap_region(void)  ->  HL = base, DE = limit
_heap_region::
        ld      hl,(0x0006)             ; BDOS base (top of TPA)
        ld      de,#SYS_STACK_RESERVE
        or      a
        sbc     hl,de                   ; HL = limit
        ex      de,hl                   ; DE = limit
        ld      hl,#__heap_base          ; HL = base
        ret
