# `cpc-464` platform

`cpc-464` links firmware-hosted programs at `0x4000`. The C runtime uses the
firmware Text VDU and keyboard jumpblocks, retains interrupts, manages the
linked-image-to-`0x9F00` heap, grows a private stack down from `0xA6FC`, and
returns cleanly to BASIC. The firmware ticker supplies the C clock with a
settable reset-relative epoch.

The CPC 464 has no built-in disk subsystem, so file descriptors above the
three console descriptors fail without pulling disk buffers or AMSDOS command
support into the image. The common target-independent libc remains available.

Build a cassette program with:

```sh
bin/x/bin/xcc -Os --platform=cpc-464 --oformat=binary main.c -o app.bin
bin/x/bin/xprog --cdt app.bin --name APP -o app.cdt
```

Insert `app.cdt` and enter `RUN"!APP"`; the `!` suppresses the firmware's
physical PLAY prompt when an emulator has already started the virtual tape.
