# IR Guide

This page describes the intermediate representation that `xcc` uses
between the C frontend and the Z80 backend.

The real source of truth is:

- [src/xc/xcc/include/ir/icode.h](/home/tstih/data/retro-vault/xyz/src/xc/xcc/include/ir/icode.h)
- [src/xc/xcc/src/ir/icode.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/ir/icode.cpp)

If this guide and the code ever disagree, the code wins.

## What The IR Is For

`xcc` does not generate Z80 assembly directly from the AST.

It first lowers C into a small typed three-address IR:

- the frontend resolves C syntax, types, and lvalues
- IR generation rewrites that into simpler operations
- IR optimization cleans those operations up
- the Z80 backend turns the IR into assembly

That split matters because it lets us reason about:

- control flow
- temporary values
- calls and ABI details
- pointer loads and stores
- machine-independent optimization

without carrying all of C syntax into the backend.

## Core Shape

Most instructions conceptually look like:

```text
result = left OP right
```

But not every opcode uses every field.

For example:

- `ASSIGN` uses `result` and `left`
- `RETURN` uses `left`
- `IFX` uses `left`, `true_lbl`, and `false_lbl`
- `CALL` uses `result`, `func_name`, and `num_params`

The IR is intentionally small and regular rather than SSA-heavy or
machine-specific.

## Main Containers

The IR lives in three layers.

### `operand`

An `operand` is a value reference. It can be:

- no operand
- a temporary
- a named symbol
- an integer constant
- a floating-point constant
- a label reference

Important fields from the real struct:

- `kind`
- `type`
- `temp_id`
- `name`
- `ival`
- `fval`
- `is_global`
- `is_param`
- `is_func`
- `is_tls`
- `is_sfr`
- `stack_offset`
- `byte_offset`

### `icode`

An `icode` is one IR instruction.

Important fields:

- `op`
- `result`
- `left`
- `right`
- `label_name`
- `true_lbl`
- `false_lbl`
- `func_name`
- `asm_text`
- `num_params`
- `local_bytes`
- `argreg`
- `arg_loc`
- `line`
- `arg_bytes`
- `callee_abi`

### `ir_function`

An `ir_function` owns the instruction list for one function plus the
function-level metadata:

- function name
- return type
- local byte count
- parameter count
- calling convention
- stack-passed parameter bytes
- `[[noreturn]]` flag

### `ir_module`

An `ir_module` owns:

- all functions
- all globals
- all string literals

This is what the IR generator produces and what the backend consumes.

## Operand Kinds

The current operand kinds are:

```cpp
NONE
TEMP
SYMBOL
INT_CONST
FLOAT_CONST
LABEL_REF
```

### `TEMP`

Compiler-generated temporaries like `t3`, `t17`, and so on.

These are not source-language variables. They are anonymous IR values
created during lowering and optimization.

### `SYMBOL`

Named storage or function symbols.

In practice this includes:

- locals
- parameters
- globals
- function names
- TLS symbols
- SFR-backed pseudo-objects

The same operand kind is used for both source locals and internal
compiler-created named storage.

### Constants

- `INT_CONST` stores `ival`
- `FLOAT_CONST` stores `fval`

Integer constants are heavily used in normal integer lowering, control
tests, shifts, offsets, and helper calls.

### `LABEL_REF`

Used for control-flow targets.

## How Operands Print

The IR dump uses `operand::to_string()`.

So a dumped operand looks like:

- `_` for no operand
- `t7` for a temp
- `#42` for an integer constant
- `#3.500000` for a float constant
- `label_name` for a label reference

Named symbols print differently depending on storage:

- global: `counter`
- parameter: `x(ix+4)`
- local: `sum(ix-2)`

That printed form is a debugging view, not an ABI promise. It is just
how the dumper helps you see where the value comes from.

## Opcode Families

The opcode set is small and intentionally concrete.

### Control Flow

- `LABEL`
- `GOTO`
- `IFX`

`IFX` is the main conditional branch form:

```text
if left goto true_lbl else false_lbl
```

There is no separate family of many branch opcodes at IR level. Compare
instructions produce a normal integer truth value, and `IFX` consumes a
value.

### Function Boundaries

- `FUNCTION`
- `ENDFUNCTION`
- `RETURN`

These delimit functions and carry frame metadata such as local byte
count.

### Calls And ABI

- `SEND`
- `RECEIVE`
- `CALL`

This is one of the most important things to understand in `xcc` IR.

The IR makes argument movement explicit:

- callers emit one `SEND` per argument
- callees emit `RECEIVE` to capture incoming parameters
- `CALL` names the callee and argument count

`SEND` and `RECEIVE` also carry ABI metadata:

- `argreg`
- `arg_loc`
- `callee_abi`

So the IR is not purely abstract here. It already knows enough about the
selected calling convention to express stack-vs-register argument
placement.

### Data Movement

- `ASSIGN`
- `ADDRESS_OF`
- `GET_VALUE_AT`
- `SET_VALUE_AT`

These are the fundamental storage and pointer operations.

Examples:

- `t1 = &x`
- `t2 = *t1`
- `*t1 = t3`

Important special convention:

For `SET_VALUE_AT`, the fields are intentionally:

- `result` = pointer address
- `left` = value to store

That is the reverse of the usual “result gets the value” reading.
It is called out directly in `icode.h` because getting this wrong causes
silent store bugs.

### Integer Arithmetic

- `ADD`
- `SUB`
- `MUL`
- `DIV`
- `MOD`
- `NEG`

### Bitwise And Shift

- `BAND`
- `BOR`
- `BXOR`
- `BNOT`
- `SHL`
- `SHR`
- `ROL`
- `ROR`
- `PACK_BYTES`

`PACK_BYTES` is a helper-style IR operation that combines two byte
values into one 16-bit value.

### Comparisons

- `EQ`
- `NE`
- `LT`
- `LE`
- `GT`
- `GE`

Comparison results are normal integer values:

- `0` for false
- `1` for true

That is why so many IR optimizations focus on compare-result cleanup and
direct `IFX` fusion.

### Conversion

- `CAST`

This is the explicit typed conversion operation. The destination type is
the `result.type`.

### Floating Point

- `FADD`
- `FSUB`
- `FMUL`
- `FDIV`
- `FITOSF`
- `FSTOI`

These are still IR-level operations even though the backend often lowers
them through soft-float runtime helpers.

### Stack Allocation

- `ALLOCA`

Used for dynamic stack reservation such as VLA-style lowering.

### Inline Assembly

- `INLINE_ASM`

This carries the raw text in `asm_text`.

### Complex Construction

- `MAKE_COMPLEX`

This packs two 4-byte soft-float values into one 8-byte complex object.

## Example IR Shapes

### Arithmetic

Source:

```c
int add(int a, int b) { return a + b; }
```

Typical IR shape:

```text
proc add (params=2 locals=...)
  t1 = recv(0,hl)
  t2 = recv(1,de)
  t3 = t1 ADD t2
  ret t3
endproc add
```

The exact ABI locations depend on the selected calling convention and
optimization level, but this is the general shape.

### Branching

Source:

```c
if (x < 10)
    y = 1;
else
    y = 2;
```

Typical shape:

```text
  t1 = x LT #10
  if t1 goto __xcc_L1 else __xcc_L2
__xcc_L1:
  y = #1
  goto __xcc_L3
__xcc_L2:
  y = #2
__xcc_L3:
```

This is exactly the kind of pattern that later IR cleanup tries to
simplify.

### Calls

Source:

```c
z = f(x, y);
```

Typical shape:

```text
  send(0,hl) x
  send(1,de) y
  t1 = call f (2)
  z = t1
```

Again, the printed locations depend on the ABI, but the explicit
argument flow is always visible in IR.

## Calling Convention Metadata In The IR

The IR is where `xcc` starts carrying ABI decisions in a structured way.

Important pieces:

- `ir_function::abi`
- `ir_function::stack_param_bytes`
- `icode::argreg`
- `icode::arg_loc`
- `icode::callee_abi`
- `abi_arg_loc`

The current concrete argument-location enum is:

- `STACK`
- `REG_A`
- `REG_L`
- `REG_HL`
- `REG_DE`
- `REG_DEHL`

That means the IR is already aware of modern register-first ABI
placement before the backend emits actual Z80 instructions.

## Frame Model

The IR still thinks in terms of a frame, even when later optimization
lets the backend omit parts of it.

Function-level frame metadata includes:

- `local_bytes`
- `orig_local_bytes`
- `stack_param_bytes`

And symbol operands may carry:

- `stack_offset`
- `is_param`

This is why IR dumps often show locals and parameters in IX-relative
terms even before final backend emission.

## What `--dump-ir` Shows

`xcc --dump-ir` uses the real dumper in `icode.cpp`.

Important printed forms:

- function start:
  `proc name (params=N locals=M)`
- function end:
  `endproc name`
- label:
  `label:`
- unconditional branch:
  `goto label`
- conditional:
  `if value goto A else B`
- return:
  `ret` or `ret value`
- direct call:
  `t3 = call foo (2)`
- argument send:
  `send(0,hl) t1`

That makes `--dump-ir` one of the best ways to understand where a bad
backend shape starts:

- bad IR shape means the fix is probably in lowering or IR optimization
- good IR shape but bad assembly means the fix is probably in Z80 codegen

## What The IR Is Not

This IR is not:

- SSA with PHI nodes
- machine code
- a fully target-neutral research IR
- a graph-based mid-level optimizer format

It is a practical compiler IR built specifically for this compiler and
this backend architecture.

That is a strength here. It is small enough to read and debug.

## Where To Study It In The Compiler

Best files to read next:

- [src/xc/xcc/include/ir/icode.h](/home/tstih/data/retro-vault/xyz/src/xc/xcc/include/ir/icode.h)
  Full IR data model.

- [src/xc/xcc/src/ir/icode.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/ir/icode.cpp)
  Dump format and human-readable view.

- [src/xc/xcc/src/ir/irgen_expr.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/ir/irgen_expr.cpp)
  Expression lowering.

- [src/xc/xcc/src/ir/irgen_stmt.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/ir/irgen_stmt.cpp)
  Statements, labels, loops, and branches.

- [src/xc/xcc/src/ir/irgen_lvalue.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/ir/irgen_lvalue.cpp)
  Lvalue reads, writes, address-taking, and pointer-style access.

- [src/xc/xcc/src/opt/iropt.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/opt/iropt.cpp)
  Per-function IR optimizer pipeline.

- [src/xc/xcc/src/opt/iromod.cpp](/home/tstih/data/retro-vault/xyz/src/xc/xcc/src/opt/iromod.cpp)
  Module-level helper, const-eval, and inlining passes.

## Practical Reading Advice

If you are debugging codegen quality:

1. compile with `--dump-ir -S`
2. inspect whether the IR already looks bloated or branchy
3. only then inspect the Z80 assembly

If the IR already contains:

- unnecessary boolean temps
- too many jumps
- widened byte arithmetic
- helper calls that should have folded away

then the right fix is usually before the backend.

If the IR looks clean but the assembly still explodes into:

- too much IX traffic
- too many push/pop pairs
- missed short branches
- unnecessary reloads

then the right fix is usually in Z80 code generation or late peephole
cleanup.
