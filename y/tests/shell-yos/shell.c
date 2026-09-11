/*
 * Temporary disk-resident YOS shell process.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <gpx.h>
#include <yos.h>

static volatile unsigned char initialized_marker = 0x5a;
static volatile unsigned char zero_marker;

void main(void)
{
    static const char message[] = "Alto (c) 2026 Wischner Labs Ltd.";
    gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);

    if (initialized_marker == 0x5a && zero_marker == 0 && gpx) {
        gpx_t *screen = gpx->create(GPXM_DEFAULT);
        const font_t *font = gpx->get_system_font();

        if (screen && font) {
            coord width = gpx->measure_text(message, font);
            coord x = (coord)((256 - width) / 2);
            coord y = (coord)((192 - font->glyph_height) / 2);

            gpx->clear_screen();
            gpx->draw_text(screen, x, y, message, font,
                           CO_FORE, BM_CPY, 0);
        }
    }

    for (;;) {
        /* Keep the temporary shell process alive for visual validation. */
    }
}
