//
// Minimal stdio output support for SDCC Z80 suite cases under uCsim.
// This routes putchar() and puts() through the simulator interface
// attached to Z80 output port 0xff.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#include <stdio.h>

__sfr __at (0xff) c23_simif_port;

static void c23_simif_write_char(char c)
{
    c23_simif_port = 'w';
    c23_simif_port = (unsigned char)c;
}

int putchar(int c)
{
    c23_simif_write_char((char)c);
    return (unsigned char)c;
}

int puts(const char *text)
{
    while (*text)
        c23_simif_write_char(*text++);

    c23_simif_write_char('\n');
    return 0;
}
