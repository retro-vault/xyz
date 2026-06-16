        ;; close.s  (sys backend: none — template)
        ;;
        ;; int close(int fd)
        ;;   HL = fd                            (sdcccall(1))
        ;;   returns DE = 0 on success, 0xFFFF (-1) on error.
        ;;
        ;; No filesystem here, so there is nothing to release: report success.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module close
        .optsdcc -mz80 sdcccall(1)

        .globl  _close

        .area   _CODE
_close::
        ld      de,#0                   ; success
        ret
