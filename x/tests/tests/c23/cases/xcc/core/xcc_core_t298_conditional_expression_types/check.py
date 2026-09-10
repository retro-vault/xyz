import pathlib
import subprocess
import sys

compiler, directory = sys.argv[1:]
work = pathlib.Path(directory)
work.mkdir(parents=True, exist_ok=True)
valid = work / "conditional_types.c"
valid.write_text("""
long wide(void);
int narrow(void);
long (*indirect)(void);
typedef char * [[xcc::far]] far_pointer;
typedef _BitInt(9) signed9;
typedef _BitInt(17) signed17;
typedef unsigned _BitInt(9) unsigned9;
static_assert(sizeof(wide()) == 4);
static_assert(sizeof(indirect()) == 4);
static_assert(sizeof(1 ? wide() : narrow()) == 4);
static_assert(sizeof(1 ? indirect() : narrow()) == 4);
static_assert(_Generic(1 ? indirect() : narrow(), long: 1, default: 0));
static_assert(_Generic(1 ? (const int *)0 : (volatile int *)0,
    const int *: 0, volatile int *: 0, const volatile int *: 1, default: 0));
static_assert(_Generic(1 ? (volatile void *)0 : (const int *)0,
    volatile void *: 0, const int *: 0, const volatile void *: 1, default: 0));
static_assert(_Generic(1 ? (char *)0 : (far_pointer)0,
    char *: 0, far_pointer: 1, default: 0));
static_assert(sizeof(1 ? (char *)0 : (far_pointer)0) == 3);
static_assert(sizeof(1 ? (far_pointer)0 : (char *)0) == 3);
static_assert(_Generic(1 ? (signed9)0 : (signed17)0,
    signed9: 0, signed17: 1, unsigned9: 0, default: 0));
static_assert(!__builtin_types_compatible_p(signed9, signed17));
static_assert(!__builtin_types_compatible_p(signed9, unsigned9));
static_assert(sizeof((unsigned9)1 << 1) == sizeof(unsigned9));
static_assert(_Generic((unsigned9)1 << 1UL, unsigned9: 1, default: 0));
static_assert(_Generic((signed9)-4 >> 1UL, signed9: 1, default: 0));
static_assert(_Generic((unsigned char)1 << 1UL, int: 1, default: 0));
static_assert(_Generic((unsigned9)1 < (unsigned9)2, int: 1, default: 0));
static_assert(_Generic((signed17)1 >= (signed17)2, int: 1, default: 0));
static_assert(_Generic((unsigned9)1 == (unsigned9)2, int: 1, default: 0));
static_assert(_Generic((unsigned9)1 != (unsigned9)2, int: 1, default: 0));
static_assert(_Generic((unsigned9)1 && (unsigned9)2, int: 1, default: 0));
static_assert(_Generic((signed17)1 || (signed17)2, int: 1, default: 0));
int main(void) { return 0; }
""")
invalid = work / "conditional_const.c"
invalid.write_text("""
void write_selected(int choose, int *first, const int *second) {
    *(choose ? first : second) = 7;
}
""")
for profile in ("O0", "Os", "Of"):
    result = subprocess.run(
        [compiler, "-" + profile, "-S", str(valid), "-o", str(work / (profile + ".s"))],
        capture_output=True, text=True,
    )
    (work / (profile + ".log")).write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
result = subprocess.run(
    [compiler, "-O0", "-S", str(invalid), "-o", str(work / "invalid.s")],
    capture_output=True, text=True,
)
(work / "invalid.log").write_text(result.stdout + result.stderr)
assert result.returncode != 0, "write through combined const pointee was accepted"
assert "const-qualified" in result.stdout + result.stderr
print("conditional call, common-type, qualifier and far-pointer checks passed")
