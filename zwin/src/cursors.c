/*
 * cursors.c
 *
 * Outline animmation.
 *
 * MIT License (see: LICENSE)
 * Copyright (c) 2021 Tomaz Stih
 *
 * 12.04.2021   tstih
 *
 */
#include "yos.h"
#include "gpx.h"

byte_t cur_arrow[] = {
    
    /* gpy_envelope_t */
      GCLS_MOUSE_CURSOR|GDWM_TINY                       /* Tiny mouse cursor format. */
    , 0x00                                              /* Reserved byte. Only used for raster graphics. */
    , 0x07, 0x00                                        /* Glyph width=7. */
    , 0x0a, 0x00                                        /* Glyph height=10. */
    , 0x01                                              /* only 1 glyph following */
    
    /* gpy_tiny_glyph_s */
    , 0x01                                              /* x origin = 1 */
    , 0x01                                              /* y origin = 1 */
    , 0x2a                                              /* 42 moves */

    /* glyph data */
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_LEFT
    , TPV_SET|TDR_LEFT_UP
    , TPV_SET|TDR_LEFT_UP
    , TPV_SET|TDR_LEFT_UP
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_RIGHT_DOWN
    , TPV_SET|TDR_LEFT
    , TPV_SET|TDR_LEFT_UP
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_RIGHT_UP
    , TPV_SET|TDR_RIGHT
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_RIGHT
    , TPV_SET|TDR_DOWN
    , TPV_SET|TDR_DOWN
    , TPV_RESET|TDR_RIGHT_UP
    , TPV_RESET|TDR_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_RIGHT
    , TPV_RESET|TDR_RIGHT_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_LEFT_UP
    , TPV_RESET|TDR_LEFT_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_DOWN
    , TPV_RESET|TDR_RIGHT_DOWN
    , TPV_RESET|TDR_RIGHT_UP
    , TPV_RESET|TDR_RIGHT_DOWN
};