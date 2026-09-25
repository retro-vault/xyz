/*
 * Minimal disk-resident C shell for YOS ABI 6.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */

#include <yos.h>

void main(void)
{
    static const char message[] = "Hello World!";
    yos_t *yos = (yos_t *)query_service("yos");
    gpx_t screen;
    const font_t *font;
    coord text_width;

    if (!yos || yos->version() < YOS_VERSION)
        for (;;) {}

    screen.width = yos->gpx_width();
    screen.height = yos->gpx_height();
    screen.pages = 1;
    screen.text_background = GPX_TEXT_BG_OPAQUE;

    font = yos->gpx_get_system_font();
    if (!font)
        for (;;) {}

    text_width = yos->gpx_measure_text(message, font);
    yos->gpx_clear_screen();
    yos->gpx_draw_text(
        &screen,
        (coord)((screen.width - text_width) / 2),
        (coord)((screen.height - font->glyph_height) / 2),
        message, font, CO_FORE, BM_CPY, 0);

    for (;;) {}
}
