#!/usr/bin/env python3
"""Execute memset contracts and check conservative builtin eligibility."""
import os
import pathlib
import re
import subprocess
import sys

compiler, source, output = map(pathlib.Path, sys.argv[1:])
output = output.resolve()
output.mkdir(parents=True, exist_ok=True)
repository = next(p for p in pathlib.Path(__file__).resolve().parents
                  if (p / "x/lib/xz80/include/xz80/xz80.h").exists())
prefix = compiler.resolve().parent.parent
if not (prefix / "lib/libxz80.a").exists():
    prefix = repository / "bin/x"
tools = prefix / "bin"
probe = output / "memory-probe"
subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2",
                "-I" + str(repository / "x/lib/xz80/include"),
                str(repository / "x/tests/tests/xopt/observable_memory_probe.cpp"),
                str(prefix / "lib/libxz80.a"), "-o", str(probe)], check=True)
helper = output / "external-memset.c"
helper.write_text("""void *memset(void *destination, int value, unsigned count) {
    unsigned char *p=destination;
    while(count--) *p++=(unsigned char)value;
    return destination;
}
void *__memset(unsigned count, int value, void *destination) {
    return memset(destination, value, count);
}
""")
checks = 0


def compile_source(path, stem, flags):
    assembly = stem.with_suffix(".s")
    result = subprocess.run([str(compiler), "-S", "--dump-ir", *flags,
                             str(path), "-o", str(assembly)],
                            text=True, capture_output=True)
    assert result.returncode == 0, (stem, result.stdout, result.stderr)
    ir = result.stdout + result.stderr
    stem.with_suffix(".ir").write_text(ir)
    return assembly, ir


def assemble(path):
    obj = path.with_suffix(".rel")
    subprocess.run([str(tools / "xas"), str(path), "-o", str(obj)], check=True)
    return obj


def execute(assembly, helper_obj, abi, mode="memory"):
    stem = assembly.with_suffix("")
    obj = assemble(assembly)
    binary, map_file = stem.with_suffix(".bin"), stem.with_suffix(".map")
    subprocess.run([str(tools / "xld"), "-nostdlib", "--no-default-runtime",
                    "--oformat=binary", "--section-start=_CODE=0x100",
                    "--section-start=_BSS=0xc000", "--binary-range=0-0xffff",
                    "-e", "_exercise", "-Map=" + str(map_file), str(obj),
                    str(helper_obj), str(prefix / "z80/lib/libruntime.a"),
                    "-o", str(binary)], check=True)
    symbols = {m[2]: int(m[1], 16) for m in re.finditer(
        r"^([0-9A-Fa-f]{8}) (\S+)", map_file.read_text(), re.MULTILINE)}
    result = subprocess.run([str(probe), str(binary), mode, "0", str(abi),
                             str(symbols["_exercise"]), str(symbols.get("_control", 0)), "0"],
                            text=True, capture_output=True)
    assert result.returncode == 0, (stem, result.stdout, result.stderr)
    return int(result.stdout.split()[0])


for runtime in ("x", "z88dk-classic"):
    for abi in (0, 1):
        common = ["--runtime", runtime, "--sdcccall", str(abi)]
        helper_stem = output / f"helper-{runtime}-abi{abi}"
        helper_assembly, helper_ir = compile_source(helper, helper_stem, ["-O0", *common])
        helper_obj = assemble(helper_assembly)
        for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
            for count in (0, 1, 255, 256, 257):
                stem = output / f"fill{count}-{runtime}-{profile}-abi{abi}"
                assembly, ir = compile_source(source, stem,
                    ["-" + profile, *common, "-DCOMPILE_ONLY", f"-DTEST_COUNT={count}"])
                enabled = profile in ("O3", "Of", "Os")
                # The second sizeof(pointer) fill is always nonempty.
                assert ("block_fill(" in ir) == enabled, (stem, ir)
                checks += execute(assembly, helper_obj, abi) + 1

        bridge = output / "header-bridge.c"
        bridge.write_text("""extern void *__memset(unsigned,int,void*);
__attribute__((always_inline)) static inline void *memset(void*p,int c,unsigned n) {
    return __memset(n,c,p);
}
static unsigned char bytes[258];
#if BRIDGE_CASE == 2
volatile unsigned control;
#endif
unsigned exercise(unsigned seed) {
#if BRIDGE_CASE == 2
    control=seed;
    memset(bytes,control,0);
    return 0;
#else
    unsigned i;
    void *result;
    bytes[0]=0xa5; bytes[257]=0x5a;
#if BRIDGE_CASE == 0
    result=memset(bytes+1,seed,256);
#else
    memset(bytes+1,0,256);
    result=bytes+1;
    seed=0;
#endif
    if(result!=bytes+1 || bytes[0]!=0xa5 || bytes[257]!=0x5a) return 1;
    for(i=1;i<=256;++i) if(bytes[i]!=(unsigned char)seed) return 2;
    return 0;
#endif
}
""")
        for profile in ("Os", "Of"):
            for case in (0, 1, 2):
                stem = output / f"bridge{case}-{runtime}-{profile}-abi{abi}"
                assembly, ir = compile_source(bridge, stem,
                    ["-" + profile, *common, f"-DBRIDGE_CASE={case}"])
                enabled = runtime == "z88dk-classic" and case != 2 and (profile == "Of" or case == 1)
                assert ("block_fill(" in ir) == enabled, (stem, ir)
                checks += execute(assembly, helper_obj, abi) + 1

        # Qualifier erasure must not discard argument reads when the callee
        # or an unused formal parameter disappears through interprocedural
        # optimization. Also exercise indirect calls and default promotions.
        argument_cases = {
            "unused": ("volatile unsigned control;", "static unsigned ignore(unsigned x){return 0;}", "ignore(control)"),
            "narrow": ("volatile unsigned control;", "static unsigned ignore(unsigned char x){return 0;}", "ignore(control)"),
            "indirect": ("volatile unsigned control;", "static unsigned ignore(unsigned x){return 0;} static unsigned (* volatile dispatch)(unsigned)=ignore;", "dispatch(control)"),
            "variadic_argument": ("volatile unsigned control;", "[[sdcc::sdccall(0)]] static unsigned ignore(unsigned x,...){return 0;}", "ignore(7,control)"),
            "port_argument": ("[[sdcc::sfr(0x80)]] unsigned char control_port;", "static unsigned ignore(unsigned char x){return 0;}", "ignore(control_port)"),
        }
        for name, (global_code, callee, call) in argument_cases.items():
            path = output / (name + ".c")
            setup = "" if name == "port_argument" else "control=seed;"
            path.write_text(global_code + callee +
                            "unsigned exercise(unsigned seed){" + setup +
                            "return " + call + ";}")
            for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
                stem = output / f"{name}-{runtime}-{profile}-abi{abi}"
                assembly, ir = compile_source(path, stem, ["-" + profile, *common])
                checks += execute(assembly, helper_obj, abi, "argument")

        for name, expression, expected in (
            ("bool2", "(_Bool)2", 1),
            ("bool256", "(_Bool)256", 1),
            ("bitint8", "(unsigned _BitInt(3))8", 0),
            ("bitint9", "(unsigned _BitInt(3))9", 1),
        ):
            path = output / (name + ".c")
            path.write_text("extern void *memset(void*,int,unsigned);"
                            "static unsigned char bytes[4];"
                            "__attribute__((always_inline)) static inline void fill(void*p,unsigned n){memset(p,0,n);}"
                            "unsigned exercise(unsigned seed){"
                            "bytes[0]=bytes[1]=bytes[2]=bytes[3]=0x55;"
                            "fill(bytes+1," + expression + ");"
                            "return bytes[0]!=0x55 || bytes[1]!=" +
                            ("0" if expected else "0x55") +
                            " || bytes[2]!=0x55 || bytes[3]!=0x55;}")
            for profile in ("Os", "Of"):
                stem = output / f"{name}-{runtime}-{profile}-abi{abi}"
                assembly, ir = compile_source(path, stem, ["-" + profile, *common])
                checks += execute(assembly, helper_obj, abi)

        deep = output / "deep-frame.c"
        deep.write_text("""extern void *memset(void*,int,unsigned);
unsigned exercise(unsigned seed) {
    struct { unsigned char bytes[32]; unsigned char padding[320]; } frame;
    unsigned value=seed*3u+17u, i;
    frame.padding[0]=0xa5; frame.padding[319]=0x5a;
    memset(frame.bytes,value,32);
    for(i=0;i<32;++i) if(frame.bytes[i]!=(unsigned char)(seed*3u+17u)) return 1;
    return frame.padding[0]!=0xa5 || frame.padding[319]!=0x5a;
}
""")
        for profile in ("O0", "O1", "O2", "O3", "Of", "Os"):
            stem = output / f"deep-{runtime}-{profile}-abi{abi}"
            assembly, ir = compile_source(deep, stem, ["-" + profile, *common])
            checks += execute(assembly, helper_obj, abi)

        declarations = "extern void *memset(void *, int, unsigned);\n"
        guards = {
            "disabled": (declarations + "void *f(void *p){return memset(p,3,16);}",
                         ["-fno-memory-builtins"]),
            "dynamic": (declarations + "void *f(void *p,unsigned n){return memset(p,3,n);}", []),
            "volatile": (declarations + "void *f(volatile char *p){return memset((void *)p,3,16);}", []),
            "atomic": (declarations + "void *f(_Atomic char *p){return memset((void *)p,3,16);}", []),
            "far": (declarations + "void *f(char * [[xcc::far]] p){return memset((void *)p,3,16);}", []),
            "signature": ("extern unsigned memset(void *,int,unsigned); unsigned f(void*p){return memset(p,3,16);}", []),
            "variadic": ("[[sdcc::sdccall(0)]] extern void *memset(void *,int,...); void *f(void*p){return memset(p,3,16);}", []),
            "convention": ("[[z88dk::smallc]] extern void *memset(void *,int,unsigned); void *f(void*p){return memset(p,3,16);}", []),
            "definition": ("volatile unsigned calls; void *memset(void*p,int c,unsigned n){++calls;return (char*)p+1;} void *f(void*p){return memset(p,3,16);}", []),
            "indirect": ("void *f(void *(*fn)(void*,int,unsigned),void*p){return fn(p,3,16);}", []),
            "explicit": ("extern void *memset_explicit(void*,int,unsigned); void *f(void*p){return memset_explicit(p,3,16);}", []),
        }
        for name, (code, flags) in guards.items():
            path = output / (name + ".c")
            path.write_text(code)
            for profile in ("Os", "Of"):
                stem = output / f"{name}-{runtime}-{profile}-abi{abi}"
                _, ir = compile_source(path, stem, ["-" + profile, *common, *flags])
                assert "block_fill(" not in ir, (stem, ir)
                checks += 1
        size_policy = {
            "returned_pointer": ("void *f(void*p,int v){return memset(p,v,16);}", False),
            "variable_large": ("void f(void*p,int v){memset(p,v,16);}", False),
            "variable_one": ("void f(void*p,int v){memset(p,v,1);}", True),
            "constant_large": ("void f(void*p){memset(p,0,16);}", True),
        }
        for name, (body, size_enabled) in size_policy.items():
            path = output / (name + ".c")
            path.write_text(declarations + body)
            for profile in ("Os", "Of"):
                stem = output / f"{name}-{runtime}-{profile}-abi{abi}"
                _, ir = compile_source(path, stem, ["-" + profile, *common])
                assert ("block_fill(" in ir) == (profile == "Of" or size_enabled), (stem, ir)
                checks += 1
        # 65535 bytes cannot be executed while retaining code and a stack in
        # the same 64K test image. Check its bounded LDIR lowering instead.
        maximum = output / "maximum.c"
        maximum.write_text(declarations + "void f(void*p){memset(p,255,65535u);}")
        for profile in ("Os", "Of"):
            stem = output / f"maximum-{runtime}-{profile}-abi{abi}"
            assembly, ir = compile_source(maximum, stem, ["-" + profile, *common])
            assert "block_fill(" in ir and "65535" in ir, (stem, ir)
            text = assembly.read_text()
            assert "ldir" in text and len(text.splitlines()) < 100, (stem, text)
            assemble(assembly)
            checks += 1
print(f"{checks} constant-memory value and eligibility checks passed")
