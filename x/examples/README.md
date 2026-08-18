# X examples

Examples in this tree are small, directly buildable programs for staged X
platforms. Run their commands from the repository root after `make -C x`.

Every immediate directory names exactly one `--platform` target. Examples do
not share source across target directories.

| Directory | Target | Description |
|---|---|---|
| [`cpm3/`](cpm3/) | `cpm3` | CP/M 3 Hello World `.COM` program |
| [`zx-ram/`](zx-ram/) | `zx-ram` | Tamsyn Lorem Ipsum demo as raw binary, TAP, and TZX |
| [`zx-rom/`](zx-rom/) | `zx-rom` | Tamsyn Lorem Ipsum replacement ROM |
