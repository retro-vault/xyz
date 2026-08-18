        ;; close.s  (sys backend: zx-ram)
        ;;
        ;; Console descriptors close successfully; there are no files.

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE

_close::
        ld      a,h
        or      a
        jr      nz,.zx_close_fail
        ld      a,l
        cp      #3
        jr      nc,.zx_close_fail
        ld      de,#0
        ret
.zx_close_fail:
        ld      de,#0xffff
        ret
