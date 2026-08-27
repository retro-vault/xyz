# `cpc-6128` platform

`cpc-6128` links firmware-hosted programs at `0x4000`, uses the firmware Text
VDU, keyboard and 300 Hz ticker, keeps a private stack below the AMSDOS memory
reservation, and returns cleanly to BASIC.

AMSDOS redirects the firmware Cassette Manager jumpblock to disk. The backend
therefore supports one sequential input descriptor (`3`) and one sequential
output descriptor (`4`), backed by the two firmware-required 2 KiB buffers.
`fopen`, `fread`, `fwrite`, `fclose`, input seeking, `remove`, and `rename`
work through those ROM interfaces. Output seeking and update (`O_RDWR`) are
not representable by AMSDOS streams and fail explicitly.

```sh
bin/x/bin/xcc -Os --platform=cpc-6128 --oformat=binary main.c -o app.bin
bin/x/bin/xprog --dsk app.bin --name APP.BIN -o app.dsk
```

Insert `app.dsk` and enter `RUN"APP"` at the BASIC prompt.
