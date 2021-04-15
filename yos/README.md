# the yos operating system for zx spectrum 48k

welcome to yos, the graphical, multitasking operating system for
zx specturm 48k.

# building yos

## prerequisites

to compile this code you will need the following:
 * gnu make build system,
 * sdcc compiler, and
 * fuse zx specturm emulator.

all tools are open source and freely available.

## the build process

run

`make`

to build yos.

## deliverables

### makezxbin
makezxbin is a pc utility that fixes sdcc bug
and modifies generated binary. it is a precondition
for further compilation so it is build first.

### zxsvr
this is a pc server. it listens on serial port and
offers services that zx spectrum does not have, such
as hard drive and real time clock. yos makes
rpc calls to zxsvr.

### 48.rom
this is a rom version of yos.

### yos.bin
this is a ram version of yos. you can run it
as standard zx spectum application.

# yos internals

## memory layout

yos occupies lower 16kb of zx spectrum memory i.e. the ROM. it has modular
architecture. the lower 1KB of ROM is reserved for the Z80 initialization,
vectors, and the BIOS.

the BIOS can 