        ; CPC firmware keyboard input for blocking and polling stdio.

        .module keyboard
        .optsdcc -mz80 sdcccall(1)
        .globl  _getchar
        .globl  _trygetchar

KM_WAIT_CHAR    .equ    0xbb06
KM_READ_CHAR    .equ    0xbb09

        .area   _CODE
_getchar::
        call    KM_WAIT_CHAR
        cp      #0x0d
        jr      nz,.cpc_getchar_ready
        ld      a,#0x0a
.cpc_getchar_ready:
        ld      e,a
        ld      d,#0
        ret

_trygetchar::
        call    KM_READ_CHAR
        jr      nc,.cpc_no_char
        cp      #0x0d
        jr      nz,.cpc_try_ready
        ld      a,#0x0a
.cpc_try_ready:
        ld      e,a
        ld      d,#0
        ret
.cpc_no_char:
        ld      de,#0
        ret
