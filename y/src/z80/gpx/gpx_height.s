        ;; gpx_height.s
        ;;
        ;; ZX Spectrum display height accessor.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-04   TS

        .module gpx_height
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_height
        .globl  __gpx_ctx

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_height
        ;; Height of the active display in pixels, read from the context
        ;; gpx_create built.
        ;;
        ;; Signature:
        ;;   dim gpx_height(void)
        ;;
        ;; Return:
        ;;   DE = height in pixels
        ;;
        ;; Clobbers:
        ;;   DE, HL
        ;;
        ;; References:
        ;;   __gpx_ctx
_gpx_height::
        ld      hl,(__gpx_ctx)          ; HL = active gpx_t*
        inc     hl                      ; +2 => height
        inc     hl
        ld      e,(hl)                  ; height lo
        inc     hl
        ld      d,(hl)                  ; height hi
        ret
