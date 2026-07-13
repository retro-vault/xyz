        ;; timegm.s
        ;; This libc has no timezone or DST model: local time is UTC, so
        ;; timegm() is exactly mktime().

        .module timegm
        .optsdcc -mz80 sdcccall(1)

        .globl  _timegm
        .globl  _mktime

        .area   _CODE
_timegm::
        jp      _mktime
