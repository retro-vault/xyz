/*
 * Temporary disk-resident YOS shell process.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <gpx.h>
#include <yos.h>
#include "shelllib.h"

static volatile unsigned char initialized_marker = 0x5a;
static volatile unsigned char zero_marker;

void main(void)
{
    static const char message[] = "Alto (c) 2026 Wischner Labs Ltd.";
    gpx_api_t *gpx = (gpx_api_t *)query_service(GPX_SERVICE_NAME);
    yos_t *yos = yos_get_api();
    shelllib_api_t *library = 0;
    const char *library_message = "Library unavailable";

    if (yos && yos->version() >= YOS_VERSION) {
        library = (shelllib_api_t *)yos->load_library(
            "shelllib.svc", YOS_LIBRARY_SHARED);
        if (library && library->probe() == SHELLLIB_RESULT)
            library_message = library->message();
    }

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
            width = gpx->measure_text(library_message, font);
            gpx->draw_text(screen, (coord)((256 - width) / 2),
                           (coord)(y + font->glyph_height + 4),
                           library_message, font, CO_FORE, BM_CPY, 0);
        }
    }

    for (;;) {
        /* Keep the temporary shell process alive for visual validation. */
    }
}
