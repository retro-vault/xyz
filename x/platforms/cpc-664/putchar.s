        ; CPC firmware Text VDU output for the standard console.

        .module putchar
        .optsdcc -mz80 sdcccall(1)
        .globl  _putchar
        .globl  __cpc_putchar_a

TXT_OUTPUT      .equ    0xbb5a

        .area   _CODE
_putchar::
        push    hl
        ld      a,l
        cp      #0x0a
        jr      nz,.cpc_putchar_plain
        ld      a,#0x0d
        call    TXT_OUTPUT
        ld      a,#0x0a
.cpc_putchar_plain:
        call    TXT_OUTPUT
        pop     hl
        ld      e,l
        ld      d,#0
        ret

__cpc_putchar_a::
        cp      #0x0a
        jr      nz,.cpc_putchar_a_plain
        ld      a,#0x0d
        call    TXT_OUTPUT
        ld      a,#0x0a
.cpc_putchar_a_plain:
        jp      TXT_OUTPUT
