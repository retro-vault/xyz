// Minimal reproducer for a global-pointer static-initializer miscompile.
//
// Symptom: a global (or file-scope static) POINTER with a static initializer
// to a constant address reads back as 0 at runtime.  xcc emits the pointer
// as an UNINITIALISED reservation (`.ds 2`) instead of initialised data
// (`.dw <addr>`), so the initializer is silently dropped.  A scalar `int`
// initializer (control) and a runtime assignment of the same pointer
// (control) are unaffected.  Fails at -O0 through -O3, -Of and -Os.
//
// Asm evidence (xcc -S <this> at any -O level):
//     _g_iptr:  .ds 2        <-- initializer dropped (BUG; want .dw 0x1234)
//     _g_cptr:  .ds 2        <-- initializer dropped (BUG; want .dw 0x5678)
//     _g_int:   .dw 4660      <-- scalar init emitted correctly (0x1234)
// The uses are genuine memory loads (`ld hl,(_g_iptr)`), not const-folded,
// so the dropped initializer is observable at runtime.
//
// The pointers are never dereferenced -- only their stored values are
// checked -- so the constant addresses are arbitrary and safe.
//
// Self-checking: main() returns 0 on pass, else the id of the first failing
// check.  On the buggy compiler it returns 1 (the static pointer read back 0).
//
// Build:  xcc --platform=emu --oformat=binary this.c -o a.bin
// Run:    xemu --run --load-bin a.bin --max-steps 1000000
// Expected: exit 0
// Actual:   exit 1   (g_iptr read back as 0 instead of 0x1234)
//
// Verify GREEN with a conforming compiler in Docker (this is standard C: any
// conforming compiler emits the initializer and the program returns 0):
//
//     docker run --rm -v "$PWD":/w -w /w gcc:13 \
//       sh -c 'gcc -w -O2 x/tests/repro/global_pointer_init_miscompile.c \
//              -o /tmp/t && /tmp/t; echo exit=$?'
//     # -> exit=0

static int  *g_iptr = (int  *)0x1234;
static char *g_cptr = (char *)0x5678;
static int   g_int  = 0x1234;   /* control: scalar init works */

int main(void)
{
    /* subject: static initializer of a pointer global */
    if ((unsigned int)g_iptr != 0x1234) return 1;
    if ((unsigned int)g_cptr != 0x5678) return 2;

    /* control: scalar global init (must already pass) */
    if (g_int != 0x1234) return 3;

    /* control: runtime assignment of the same pointer (must already pass) */
    g_iptr = (int *)0x9abc;
    if ((unsigned int)g_iptr != 0x9abc) return 4;

    return 0;
}
