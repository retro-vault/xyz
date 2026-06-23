# The XCC Compiler

This is a beginner-friendly book about the compiler in this repository.
It is written for the "I know what C is, but compiler internals still feel
like black magic" level.

Everything here is based on the current source tree, not just on the older
docs. I also ran the current test suite while studying the project.

## 1. What This Project Is

`xcc` is a C11 compiler that targets the Z80.

That means:

- You write C source code.
- `xcc` turns that C into Z80 assembly.
- Another tool such as `sdasz80` assembles the output.
- A linker such as `sdldz80` links the program with startup code and runtime
  helpers.

So `xcc` is not the entire toolchain. It is the compiler front and middle
of the toolchain, plus the Z80 code generator.

There are really two important outputs in this repo:

- `build/bin/xcc`: the host-side compiler program that runs on Linux
- `lib/xcc/runtime.rel`: the target-side runtime helper object for Z80

## 2. Is This SDCC?

Not exactly.

This repo keeps SDCC reference sources under `../../docs/reference/sdcc/`.
You can see strong family resemblance in names like `icode`, `z80gen`,
`z80peep`, and in the general idea of "C frontend -> IR -> Z80 backend".

But the live compiler in `src/` and `include/` is a modern C++ rewrite,
not a direct compilation of SDCC code.

The best way to think about it is:

- SDCC is the ancestor and reference library on the shelf.
- `xcc` is the new machine built in the workshop.
- Some parts of the new machine still lean on SDCC runtime ideas when the
  new implementation is incomplete.

That last point matters. Some features compile cleanly but still depend on
stubbed runtime helpers, so "parses" and "works end-to-end" are not always
the same thing.

## 3. The Big Picture

Here is the real pipeline:

```text
C source
  ->
preprocessor
  ->
lexer
  ->
parser
  ->
semantic check
  ->
IR generator
  ->
IR optimizer (-O2, -Of / -Os promoted baseline, and experimental -O3)
  ->
Z80 code generator
  ->
peephole optimizer (-O1 and up)
  ->
Z80 assembly
```

The driver that glues this together lives in `src/driver/main.cpp`.

One of the nicest things about this project is that the stages are still
very visible. The compiler is not hiding behind a giant framework.

## 4. Tour of the Repository

These are the directories that matter most:

- `src/frontend/`: preprocessor, lexer, parser, semantic pass
- `src/ir/`: IR definitions and IR lowering
- `src/opt/`: module-level and per-function IR optimizers
- `src/backend/`: assembler emitters and Z80 backend
- `include/`: public headers for the compiler itself
- `lib/runtime/`: Z80 runtime helper implementations and stubs, grouped into
  `int8/`, `int16/`, `int32/`, `int64/`, `float/`, `double/`, `common/`,
  `atomic/`, `jumps/`, and `sys/`
- `../../../libc/include/`: canonical target-side C headers such as `stdatomic.h` and `complex.h`
- `tests/data/core/`: regression inputs and expected assembly outputs
- `docs/`: notes, architecture docs, and now this book
- `../../docs/reference/sdcc/`: SDCC reference sources, not part of the live build

The top-level `Makefile` builds `xcc`, runs tests, and stages installable
artifacts.

## 5. The Mental Model

If compilers feel abstract, use this picture:

- The preprocessor is a text editor.
- The lexer is a word splitter.
- The parser is a grammar checker that builds a tree.
- The semantic layer asks "does this tree make sense as C?"
- The IR generator rewrites the tree into simple tiny instructions.
- The backend turns those instructions into Z80 assembly.

That is the whole game.

The reason compilers look complicated is not because the idea is complicated.
It is because the C language has many corner cases.

## 6. Stage by Stage

### 6.1 Driver

The driver reads command-line options, loads the source file, and runs each
stage in order.

Important options today:

- `-O0`: no optimization
- `-O1`: peephole optimizer after assembly generation; the simplest fixed-window peepholes are now table-driven while the more contextual ones still use custom matchers
- `-O2`: general optimization, including dead static-function elimination, constant actual-argument propagation, translation-unit constant-call evaluation for eligible private integer helpers including nested helper chains, helper calls fed from constant-valued locals or temps, and a small whitelist of pure runtime helpers, whole-function constant evaluation for eligible zero-argument integer functions over that same subset, including straightforward 32-bit integer code, dead-parameter elimination, identical-helper merging for eligible internal callees, conservative size-profitable static helper inlining for the benchmark-proven subset of private helpers, CFG jump threading through label-only and `goto`-only blocks, scalar local promotion for simple helper-free 16-bit locals, conservative `sdcccall(1)` register-parameter promotion for simple helper-free straight-line callees, direct control-condition lowering, counted-byte-loop narrowing, loop pointer-walk canonicalization, and the bounded stable backend temp register allocator for short straight-line 16-bit temp windows; the core local algebraic identities are now shared through one small declarative rule table instead of duplicated `switch` logic
- `-Of`: speed optimization; it shares the current proven aggressive baseline and may spend a little size for fewer cycles, including O3-proven speed-biased peepholes
- `-O3`: experimental optimization; it keeps the proven `-Os` baseline, then adds speed, size, shape-changing, and superoptimizer-inspired peephole experiments. Here be dragons
- `-Os`: size optimization; it is the protected record-setting aggressive size baseline
- `-f<name>` / `-fno-<name>`: per-pass overrides on top of any `-O` preset, including names such as `const-call-eval`, `function-const-eval`, `address-deref-fold`, `scalar-local-promotion`, and `compare-ifx-fusion`
- `-g`: emit DWARF debug info
- `-masm=sdasz80` or `-masm=gnuas`: choose assembly dialect

The driver is refreshingly direct. It does not hide the pipeline.

### 6.2 Preprocessor

The preprocessor lives in `src/frontend/preproc.cpp`.

It is a text-to-text pass. It handles:

- `#define`
- `#undef`
- `#include`
- `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`
- `#error`
- `#pragma` as a no-op
- predefined macros like `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`

It also emits `# line "file"` markers so that later diagnostics still point
back to the original source file and line.

Simple way to understand it:

- It does not understand C syntax deeply.
- It mostly rewrites text before the lexer sees anything.

### 6.3 Lexer

The lexer lives in `src/frontend/lexer.cpp`.

Its job is to turn characters into tokens.

Examples:

- `if` becomes a keyword token
- `123` becomes an integer literal token
- `+` becomes an operator token
- `"abc"` becomes a string literal token

Each token carries source location information, plus decoded literal data.

This is the stage where the compiler stops thinking in raw characters and
starts thinking in meaningful language pieces.

### 6.4 Parser

The parser is the biggest brain in the frontend.

It lives in `src/frontend/parser.cpp`, and it does more than just parsing:

- it builds the AST
- it resolves names using the symbol table
- it recognizes typedef names while parsing
- it lays out function parameters and locals using stack offsets
- it builds struct and union field layouts
- it handles constant-expression evaluation for some constructs

So in this compiler, the parser is not only "grammar code".
It is also doing a lot of semantic setup work.

This is powerful, but it also makes the parser the most overloaded class in
the project.

### 6.5 AST

The AST lives in `include/frontend/ast.h`.

AST means Abstract Syntax Tree.

It is the compiler's "C-shaped" representation of the program.

Examples of AST nodes:

- declarations: functions, variables, typedefs
- statements: `if`, `for`, `return`, block statements
- expressions: literals, binary operations, function calls, array indexing

This is the last stage where the program still looks like C.

### 6.6 Type System

Types live in `include/frontend/types.h` and `src/frontend/types.cpp`.

The compiler stores types as graph-like objects:

- scalar types such as `int`, `long`, `float`
- pointer types
- array types
- function types
- struct, union, and enum types

The type system is Z80-aware.

Important target sizes in this compiler:

- `char`: 1 byte
- `short` and `int`: 2 bytes
- `long`: 4 bytes
- `long long`: 8 bytes
- pointer: 2 bytes
- `float`: 4 bytes
- `double`: 8 bytes

So the frontend already knows the target machine well before codegen starts.

### 6.7 Symbol Table

The symbol table lives in `include/frontend/symtab.h`.

Think of it as the compiler's notebook for names.

It records things like:

- what a name refers to
- what type it has
- whether it is global, local, or a parameter
- where it lives in the stack frame

C also has separate tag namespaces for `struct`, `union`, and `enum`, and
this compiler models that too.

### 6.8 Semantic Pass

After parsing, `src/frontend/sema.cpp` runs a smaller semantic pass.

Right now this pass is narrow. Its main checked rule is const assignment.

In other words:

- the parser already does a lot of semantic heavy lifting
- the dedicated semantic pass is currently small

This is one reason the frontend feels practical but not yet fully separated
into clean layers.

### 6.9 IR

The IR lives in `include/ir/icode.h`.

IR means Intermediate Representation.

This compiler uses a small three-address style IR.

Instead of storing "big C syntax", it stores small operations like:

- assign
- add
- subtract
- compare
- call
- return
- jump
- label
- load through pointer
- store through pointer

That is important because CPUs do not understand C syntax trees. They
understand tiny operations.

The AST is for understanding the source.
The IR is for preparing machine code.

### 6.10 IR Generator

`src/ir/irgen.cpp` walks the AST and emits IR.

This is the stage where C features get lowered into simpler building blocks.

Examples:

- `a[i]` becomes pointer arithmetic plus load
- `s.x` becomes base address plus field offset
- `f(x, y)` becomes `SEND`, `SEND`, `CALL`
- `if` and loops become explicit labels and jumps

This is one of the best files to study if you want to understand how high-
level C ideas turn into low-level compiler operations.

### 6.11 IR Optimizer

`src/opt/iropt.cpp` contains the IR optimizer pipeline.

It does three main things:

- constant folding
- copy propagation
- dead code elimination for unused temps

This is not a giant SSA optimizer. It is more like a smart cleanup pass.

That is a strength for learning. You can actually read it end to end and
understand what it is doing.

### 6.12 Z80 Code Generator

The main backend lives in `src/backend/z80/z80gen.cpp`.

This is the stage that turns IR into actual Z80 assembly instructions.

The basic strategy is:

- load values into registers such as `HL`, `DE`, or `A`
- perform the operation
- store the result back

The stack frame uses `IX` as the frame pointer.

That gives this shape:

```text
IX+4   first parameter
IX+6   next parameter
IX+...

IX     saved frame pointer

IX-2   first local
IX-4   next local
IX-... temps and extra storage
```

This is a classic, readable backend design.

It is not trying to be a hyper-aggressive optimizing compiler yet.

### 6.13 Small Register Allocation

At `-O2` / `-Os`, or when forced with `-fregalloc`, the backend tries a
limited prepass register allocation.

This is not a full global allocator.
It is more like:

- keep some 16-bit temps in `BC`
- only do it when the live range looks safe

So the project already has a little more than "everything spills to the
stack", even though some comments in the tree still sound older than the
code.

### 6.14 Peephole Optimizer

After assembly is generated, the shared `libxopt` peephole optimizer
(`lib/xopt/src/z80peep.cpp`) runs.

This pass is purely textual and very Z80-specific.

It looks for bad local patterns such as:

- redundant loads
- useless push/pop pairs
- jumps to the next line
- temp store/reload sequences that cancel out

This is the kind of optimization that often gives nice wins on small CPUs.

### 6.15 Runtime Library

`lib/runtime.s` provides helper routines.

These helpers exist because some operations are awkward or expensive to emit
inline every time.

Examples:

- `__mul16`
- `__div16`
- `__mod16`
- `__mul32`
- `__div32`
- `__mod32`
- `__call_hl`

This file also contains support for:

- soft-float entry points
- thread-local storage hooks
- atomics
- some complex-number helpers

But there is an important warning here:

some runtime entry points are real implementations, and some are still stubs.

That means the compiler can accept some language features whose runtime
behavior is not finished yet.

## 7. A Tiny Example Walkthrough

Take this function:

```c
int add(int a, int b) {
    return a + b;
}
```

At a high level:

1. The preprocessor mostly leaves it alone.
2. The lexer creates tokens like `int`, `add`, `(`, `a`, `,`, `b`, `)`, `{`.
3. The parser builds a function declaration node with two parameter nodes and
   a return statement containing a binary `+` expression.
4. The symbol table gives `a` and `b` stack locations.
5. IR generation lowers `a + b` into a temp-producing add instruction.
6. The backend emits Z80 code that loads parameters from `IX+4` and `IX+6`,
   adds them, and returns the result in `HL`.

That is the full compiler story in miniature.

## 8. What This Compiler Already Does Well

From a learning and maintenance point of view, the project already has
several strong qualities:

- The pipeline is explicit and easy to trace.
- The IR is small enough to understand.
- The frontend is hand-written and readable.
- The backend is concrete and not buried in meta-machinery.
- The test suite is fast and easy to run.
- The code is far smaller and easier to reason about than a full production
  compiler like GCC or LLVM.

That makes it a very good project for learning compiler construction on a
real target.

## 9. Important Reality Check

This part matters.

The current source is more honest than some of the older docs.

There are features that are:

- fully implemented and tested
- parsed and lowered but runtime-stubbed
- advertised in docs but still incomplete or incorrect

Examples of current weak spots:

- named labels are not really supported as normal C labels
- `switch`/`case` lowering is incomplete
- some enumerator constant-expression handling is incomplete
- `static` and `extern` variable semantics are not fully correct
- soft-float helpers are stubbed
- `long long` multiply/divide/modulo helpers are stubbed
- some complex-number runtime support is stubbed

So when you read "supports C11", the best interpretation today is:

"supports a useful and growing C11 subset, with several advanced areas still
needing correctness and runtime completion."

## 10. Where To Read First If You Want To Hack On It

Recommended reading order:

1. `src/driver/main.cpp`
2. `include/frontend/ast.h`
3. `src/frontend/parser.cpp`
4. `include/ir/icode.h`
5. `src/ir/irgen.cpp`
6. `src/backend/z80/z80gen.cpp`
7. `lib/xopt/src/z80peep.cpp`
8. `lib/runtime.s`

If you want to add a language feature, the usual path is:

1. teach the lexer about any new token
2. parse it into AST
3. give it type and symbol meaning
4. lower it into IR
5. teach the backend how to emit it
6. add a regression test

## 11. Where The Design Wants To Grow

The project feels ready for a second round of cleanup.

The biggest future wins are likely:

- split the parser into smaller pieces
- split the Z80 backend into smaller subsystems
- centralize diagnostics
- make the C11 status docs honest and evidence-based
- add runtime-backed execution tests, not only assembly snapshot tests

That work would make the compiler both easier to trust and easier to extend.

## 12. Final Summary

`xcc` is a clean, readable, modern C++ compiler for C-to-Z80 work.

Its greatest strength is that you can still see the whole machine:

- source becomes tokens
- tokens become AST
- AST becomes IR
- IR becomes Z80 assembly

That makes it a great learning compiler and a promising foundation.

Its biggest current weakness is not readability. It is truthfulness of
feature status. Some advanced features are present in syntax and structure
before they are fully correct in semantics or runtime behavior.

If you keep that distinction in mind, this compiler becomes much easier to
understand, much easier to improve, and much less mysterious.
