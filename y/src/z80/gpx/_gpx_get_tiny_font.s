        ;; _gpx_get_tiny_font.s
        ;;
        ;; Return pointer to the tiny font blob.
        ;;
        ;; Size-optimized policy: tiny font currently aliases the
        ;; system font payload to keep total ZX GPX footprint < 8 KiB.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   TS

        .module _gpx_get_tiny_font
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_get_tiny_font
        .globl  _gpx_font_envy

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_get_tiny_font
        ;; The platform small font. The ZX ships one face and returns it for
        ;; both requests, so callers must read glyph_height and advance
        ;; from the header rather than assuming a size.
        ;;
        ;; Signature:
        ;;   const font_t *gpx_get_tiny_font(void)
        ;;
        ;; Return:
        ;;   DE = font_t*, never NULL
        ;;
        ;; Clobbers:
        ;;   DE
        ;;
        ;; References:
        ;;   _gpx_font_envy
_gpx_get_tiny_font::
        ld      de,#_gpx_font_envy
        ret
