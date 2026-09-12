// Build a pristine 48K/divIDE snapshot, not a saved BASIC session.
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih

#include <libspectrum.h>
#include <stdio.h>
#include <stdlib.h>

static libspectrum_byte *read_image(const char *path, size_t size)
{
    FILE *file = fopen(path, "rb");
    libspectrum_byte *data = libspectrum_new(libspectrum_byte, size);
    if (!file || fread(data, 1, size, file) != size || fgetc(file) != EOF) {
        fprintf(stderr, "expected %zu bytes in %s\n", size, path);
        exit(1);
    }
    fclose(file);
    return data;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s ESXIDE.BIN yos.rom output.szx\n", argv[0]);
        return 2;
    }
    if (libspectrum_init()) return 1;
    libspectrum_snap *snap = libspectrum_snap_alloc();
    if (!snap) return 1;
    libspectrum_snap_set_machine(snap, LIBSPECTRUM_MACHINE_48);
    const int pages[] = {0, 2, 5};
    for (unsigned i = 0; i < sizeof pages / sizeof *pages; ++i)
        libspectrum_snap_set_pages(
            snap, pages[i], libspectrum_new0(libspectrum_byte, 16384));
    libspectrum_snap_set_custom_rom(snap, 1);
    libspectrum_snap_set_custom_rom_pages(snap, 1);
    libspectrum_snap_set_roms(snap, 0, read_image(argv[2], 16384));
    libspectrum_snap_set_rom_length(snap, 0, 16384);
    libspectrum_snap_set_pc(snap, 0);
    libspectrum_snap_set_sp(snap, 0xffff);
    libspectrum_snap_set_iff1(snap, 0);
    libspectrum_snap_set_iff2(snap, 0);
    libspectrum_snap_set_im(snap, 0);
    libspectrum_snap_set_halted(snap, 0);
    libspectrum_snap_set_tstates(snap, 0);
    libspectrum_snap_set_divide_active(snap, 1);
    libspectrum_snap_set_divide_eprom_writeprotect(snap, 1);
    libspectrum_snap_set_divide_paged(snap, 0);
    libspectrum_snap_set_divide_control(snap, 0);
    libspectrum_snap_set_divide_pages(snap, 4);
    libspectrum_snap_set_divide_eprom(snap, 0, read_image(argv[1], 8192));
    for (int page = 0; page < 4; ++page)
        libspectrum_snap_set_divide_ram(
            snap, page, libspectrum_new0(libspectrum_byte, 8192));

    libspectrum_byte *output = NULL;
    size_t output_size = 0;
    int flags;
    if (libspectrum_snap_write(&output, &output_size, &flags, snap,
                              LIBSPECTRUM_ID_SNAPSHOT_SZX, NULL, 0))
        return 1;
    FILE *file = fopen(argv[3], "wb");
    if (!file || fwrite(output, 1, output_size, file) != output_size ||
        fclose(file)) {
        fprintf(stderr, "cannot write %s\n", argv[3]);
        return 1;
    }
    libspectrum_free(output);
    libspectrum_snap_free(snap);
    libspectrum_end();
    return flags ? 1 : 0;
}
