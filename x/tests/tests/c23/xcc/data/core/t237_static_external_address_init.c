/* Regression: keep external symbol addresses in static aggregate data. */
extern unsigned char glyph_l[];
extern unsigned char glyph_u[];

static void *letters[] = {
    &glyph_l,
    &glyph_u
};

void *first_letter(void)
{
    return letters[0];
}
