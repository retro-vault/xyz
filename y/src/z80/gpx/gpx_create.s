        ;; gpx_create.s
        ;;
        ;; ZX Spectrum GPX lifecycle and shared context pointer.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-04-04   TS

        .module gpx_create
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_create
        .globl  _gpx_clrscr
        .globl  __gpx_ctx

        .equ    GPXM_DEFAULT, 0x00
        .equ    GPX_TEXT_BG_OPAQUE, 0x00

        .equ    SCRWIDTH,  0x0100       ; 256
        .equ    SCRHEIGHT, 0x00c0       ; 192
        .equ    SCRPAGES,  0x01         ; one framebuffer page

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; _gpx_create
        ;; Bring up the graphics subsystem and hand back the drawing
        ;; context. The ZX has one screen layout, so the mode argument is
        ;; accepted and ignored; the context is a static descriptor.
        ;;
        ;; Signature:
        ;;   gpx_t *gpx_create(gmode mode)
        ;;
        ;; Arguments:
        ;;   A = mode (ignored, one layout on this machine)
        ;;
        ;; Return:
        ;;   DE = gpx_t*, the static display descriptor
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL
        ;;
        ;; References:
        ;;   _gpx_clrscr
        ;;   __gpx_ctx
        ;;   __gpx_data
_gpx_create::
        call    _gpx_clrscr
        xor     a                       ; GPX_TEXT_BG_OPAQUE
        ld      (__gpx_data+5),a
        ld      de,#__gpx_data
        ld      (__gpx_ctx),de
        ret

        ;; YOS executes from ROM, so writable defaults live in the standard
        ;; copied-data pair rather than in a loadable RAM _DATA section.
        .area   _INITIALIZED

        ;; Internal active context pointer used by gpx_width/gpx_height.
__gpx_ctx::
        .ds     2

        ;; Static ZX Spectrum display descriptor (gpx_t).
__gpx_data::
        .ds     6

        .area   _INITIALIZER
        .dw     __gpx_data
        .dw     SCRWIDTH
        .dw     SCRHEIGHT
        .db     SCRPAGES
        .db     GPX_TEXT_BG_OPAQUE
