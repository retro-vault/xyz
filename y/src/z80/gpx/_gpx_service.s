        ; Direct-call table for every public ZX Spectrum libgpx function.
        ;
        ; GPL-2.0 License (see: LICENSE.libgpx)
        ; Copyright (C) 2026 tomaz stih

        .module _gpx_service
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_service
        .globl  _gpx_create
        .globl  _gpx_destroy
        .globl  _gpx_set_page
        .globl  _gpx_width
        .globl  _gpx_height
        .globl  _gpx_clrscr
        .globl  _gpx_set_text_background
        .globl  _gpx_draw_pixel
        .globl  _gpx_draw_line
        .globl  _gpx_draw_bmp
        .globl  _gpx_show_sprite
        .globl  _gpx_hide_sprite
        .globl  _gpx_draw_rectangle
        .globl  _gpx_fill_rectangle
        .globl  _gpx_measure_text
        .globl  _gpx_draw_text
        .globl  _gpx_get_system_font
        .globl  _gpx_get_tiny_font
        .globl  _gpx_get_stock_bmp
        .globl  _gpx_draw_circle
        .globl  _gpx_fill_circle
        .globl  _gpx_draw_polygon
        .globl  _gpx_fill_polygon

        .area   _CONST

        ; Keep this order synchronized with gpx_api_t in y/include/gpx.h.
__gpx_service::
        .dw     _gpx_create
        .dw     _gpx_destroy
        .dw     _gpx_set_page
        .dw     _gpx_width
        .dw     _gpx_height
        .dw     _gpx_clrscr
        .dw     _gpx_set_text_background
        .dw     _gpx_draw_pixel
        .dw     _gpx_draw_line
        .dw     _gpx_draw_bmp
        .dw     _gpx_show_sprite
        .dw     _gpx_hide_sprite
        .dw     _gpx_draw_rectangle
        .dw     _gpx_fill_rectangle
        .dw     _gpx_measure_text
        .dw     _gpx_draw_text
        .dw     _gpx_get_system_font
        .dw     _gpx_get_tiny_font
        .dw     _gpx_get_stock_bmp
        .dw     _gpx_draw_circle
        .dw     _gpx_fill_circle
        .dw     _gpx_draw_polygon
        .dw     _gpx_fill_polygon
