        ;; getpayloadf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module getpayloadf
        .optsdcc -mz80 sdcccall(1)

        .globl  _getpayloadf

        .area   _CODE
_getpayloadf::
        ; extract mantissa bits as float (simplified: return mant part)
        ; float bits in DE HL (low high? per ABI)
        ; clear sign and exp, return as float
        ld      a,h
        and     #0x7f
        ld      h,a
        ld      a,l
        and     #0x80
        or      #0x3f   ; make normal 1.m
        ld      l,a
        ; low bytes 0 for payload demo
        ld      de,#0
        ret

