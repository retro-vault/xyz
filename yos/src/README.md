# what's this?

the source code of the yos operating system.

## dependencies

this source code depends on the graphics library, located in the /yos/libs/graphics folder.

## files

 * types.h ... definition of standard os types used by all source files: byte, word, bool, string, etc.
 * crt0-rom.s ... rom version of system start up. this is the very first code that is executed at system start up.
 * crt0-ram.s ... ram version of system start up. -"-
 * kernel.c ... operating system startup. you'll find the `main()` function here
 * font6x6.s ... 6x6 kernel font data
 * console.h, console.c ... kernel text mode functions, use 6x6 font