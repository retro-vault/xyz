        ;; gpx_width.s
        ;;
        ;; ZX Spectrum display width accessor.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-04   TS

        .module gpx_width
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_width
        .globl  __gpx_ctx

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_width
        ;; Width of the active display in pixels, read from the context
        ;; gpx_create built.
        ;;
        ;; Signature:
        ;;   dim gpx_width(void)
        ;;
        ;; Return:
        ;;   DE = width in pixels
        ;;
        ;; Clobbers:
        ;;   DE, HL
        ;;
        ;; References:
        ;;   __gpx_ctx
_gpx_width::
        ld      hl,(__gpx_ctx)          ; HL = active gpx_t*
        ld      e,(hl)                  ; width lo
        inc     hl
        ld      d,(hl)                  ; width hi
        ret
