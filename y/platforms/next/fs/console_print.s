        .module next_print
        .globl __console_print, __console_print_hook
        .area _CODE
__console_print::
        push af
        ld hl,(__console_print_hook)
        ld a,h
        or l
        jr z,.done
        pop af
        jp (hl)
.done:
        pop af
        ret
        .area _BSS
__console_print_hook:: .ds 2
