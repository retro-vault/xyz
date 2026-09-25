        ;; _gpx_get_system_font.s
        ;;
        ;; Return pointer to the default UI font blob.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-03-29   TS

        .module _gpx_get_system_font
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_get_system_font
        .globl  _gpx_font_envy

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_get_system_font
        ;; The platform default UI font. The ZX ships one face and returns it for
        ;; both requests, so callers must read glyph_height and advance
        ;; from the header rather than assuming a size.
        ;;
        ;; Signature:
        ;;   const font_t *gpx_get_system_font(void)
        ;;
        ;; Return:
        ;;   DE = font_t*, never NULL
        ;;
        ;; Clobbers:
        ;;   DE
        ;;
        ;; References:
        ;;   _gpx_font_envy
_gpx_get_system_font::
        ld      de,#_gpx_font_envy
        ret
