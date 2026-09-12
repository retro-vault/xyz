# YOS ZX Spectrum libgpx

This directory vendors the ZX Spectrum and portable assembly modules from
the latest upstream `main`, commit
`0ef6f070be0aa35054e6799cae56157b9f7890c5` (`v1.1.0-1-g0ef6f07`):

https://github.com/retro-vault/libgpx

The sources are included in the YOS ROM archive so the ROM is self-contained
and reproducible. `_gpx_name.s` and `_gpx_service.s` are YOS integration
modules. `gpx_create.s` carries the one YOS-specific adaptation: it uses the
ROM-to-RAM initializer areas for its eight bytes of writable context. The
vendored library remains GPL-2.0 licensed; see `LICENSE.libgpx`.
