# YOS XCC backend

`--platform=yos` links a relocatable XL process against the ordinary X libc.
The CRT keeps the stack supplied by YOS, initializes C storage, resolves the
`"yos"` service through `RST 0x18`, and calls `main`.

All hosted facilities cross the `yos_t` table: memory uses
`allocate_memory`/`free_memory`, and POSIX file and directory calls use the
esxDOS-backed entries. Standard console output is intentionally silent until
the application installs a `yos_putchar_hook_t` with
`yos_set_putchar_hook()`; Alto can later provide that hook from its console
window. Raw keyboard transitions remain available as `yos->read_key()`.

Build and package a process with:

```sh
bin/x/bin/xcc -Os --platform=yos app.c -o build/app.xl
bin/x/bin/xprog --process --name app --stack-size 512 --min-os 8 \
  build/app.xl -o bin/y/z80/spectrum/bin/app.sys
```

The linker output must remain XL. Do not select a fixed-address or binary
output format for a YOS process.
