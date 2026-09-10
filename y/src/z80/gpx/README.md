# YOS ZX Spectrum libgpx

This directory vendors the ZX Spectrum and portable assembly modules from
libgpx v1.1.0, commit `8a9fff7f87ccc9f09bf8caac69e42880b99e4f97`:

https://github.com/retro-vault/libgpx

The sources are included in the YOS ROM archive so the ROM is self-contained
and reproducible. `_gpx_name.s` and `_gpx_service.s` are YOS integration
modules. `gpx_create.s` uses YOS's ROM-to-RAM initializer areas for its eight
bytes of writable context. The vendored library remains GPL-2.0 licensed; see
`LICENSE.libgpx`.
