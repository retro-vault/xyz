# xcc Architecture

## Pipeline

```
source.c
   │
   ▼
 Preprocessor   — built-in; #include, #define, #if/#ifdef/#elif, #error,
   │              variadic macros, #/##, __FILE__/__LINE__/__DATE__/__TIME__
   │  preprocessed text
   ▼
 Lexer          — tokenizes C11 source; one-token lookahead
   │  Token stream
   ▼
 Parser         — recursive descent; builds AST; symbol table construction;
   │              stack layout for locals and parameters
   │  TranslationUnit (list of DeclPtr)
   ▼
 Sema           — semantic analysis pass: const-assignment enforcement,
   │              duplicate switch case/default, argument count mismatch
   │  (same AST, annotated)
   ▼
 IrGen          — AST visitor; lowers to three-address IR
   │  IRModule (list of IRFunction + global table)
   ▼
 IrOpt          — IR optimizer: constant-fold, algebraic-simplify, copy-prop,
   │              DCE, strength-reduce (MUL/DIV/MOD by power-of-two → shift)
   │  (activated at -O2)
   ▼
Z80Gen         — walks IR; emits Z80 assembly via asm_emitter abstraction;
   │              bounded BC register allocation prepass at -O2
   │  text lines
   ▼
 Z80Peep        — pattern-based peephole optimizer (activated at -O1+)
   │
   ▼
 output.s       — sdasz80 or GNU as syntax (controlled by -masm=)
```

The driver in `src/driver/main.cpp` chains these passes. Each pass is
independent and communicates only through its output data structure.

### Optimization profile contract

`-Os` is the linked-size profile and may trade cycles for fewer bytes. `-Of`
is the validated measured-cycle profile and may spend bytes for speed. `-O3`
is the distinct experimental measured-cycle profile derived from `-Of`.
Byte-count-only policy remains exclusive to `-Os`; speed transformations are
developed in `-O3` and promoted to `-Of` only after validation under both
`sdcccall(1)` and `sdcccall(0)`.

---

## Directory layout

```
include/
  frontend/
    token.h          — TK enum, Token struct
    lexer.h          — Lexer class
    types.h          — Type, type_kind, type factory functions
    symtab.h         — Symbol, SymbolTable, scope_guard
    ast.h            — all AST node types + visitor interfaces
    parser.h         — Parser class
    sema.h           — Sema class (semantic analysis)
    preproc.h        — Preprocessor class
  ir/
    icode.h          — operand, icode, ir_function, ir_module
    irgen.h          — ir_gen class
  opt/
    iropt.h          — IR optimizer pipeline and pass abstractions
  backend/
    asm_emitter.h    — abstract emitter interface (dialect-independent)
    sdasz80_emitter.h — sdasz80-syntax emitter
    gnuas_emitter.h  — GNU as-syntax emitter
    z80/
      z80gen.h       — z80_gen class
      z80peep.h      — z80_peep class
      dwarf.h        — DWARF 2 debug info emitter
  driver/
    options.h        — command-line option parsing

src/
  frontend/
    lexer.cpp
    types.cpp
    symtab.cpp
    parser.cpp              — constructor, helpers, top-level and function-def parsing
    parser_declspec.cpp     — parse_declaration_specifiers, struct/enum body parsing
    parser_declarator.cpp   — parse_pointer, parse_declarator, parse_param_list
    parser_stmt.cpp         — statement parsing (all parse_*_statement methods)
    parser_init.cpp         — parse_initializer
    parser_expr.cpp         — expression parsing (precedence climbing)
    sema.cpp
    preproc.cpp
  ir/
    icode.cpp
    irgen.cpp              — constructor, helpers, module entry, emit_binop/emit_unop
    irgen_decl.cpp         — declaration visitors and gen_func
    irgen_stmt.cpp         — statement visitors
    irgen_expr.cpp         — expression visitors and arithmetic helpers
    irgen_lvalue.cpp       — lvalue read/write, index/member access
    irgen_init.cpp         — aggregate initialiser lowering
  opt/
    iropt.cpp              — CFG/value/loop analysis plus IR optimization passes
  backend/
    sdasz80_emitter.cpp
    gnuas_emitter.cpp
    z80/
      z80gen.cpp             — core utilities and icode dispatch
      z80gen_data.cpp        — module-level emission: globals, TLS, strings
      z80gen_operand.cpp     — operand addressing and register load/store helpers
      z80gen_ctrl.cpp        — control-flow icode handlers
      z80gen_mem.cpp         — memory icode handlers
      z80gen_arith.cpp       — arithmetic and bitwise icode handlers
      z80gen_regalloc.cpp    — register allocation pre-pass
      z80peep.cpp
      dwarf.cpp
  driver/
    options.cpp
    main.cpp

lib/
  runtime.s              — __mul16/__div16/__mod16/__mul32/__div32/__mod32,
                           __call_hl, soft-float stubs, 64-bit stubs,
                           atomic stubs
  include/
    stdarg.h             — va_list / va_start / va_arg / va_end for xcc ABI
    stdatomic.h          — C11 _Atomic via _Generic macros + runtime stubs
    complex.h            — _Complex / _Imaginary helpers

tests/
  run_tests.sh           — supports .expected (asm snapshot), .error (diagnostic), .opts (flags)
  data/
    core/                — t001_return_zero.c … t050_static.c (assembly snapshots, -O0)
    opt/                 — t001…t006 (assembly snapshots, -O1/-O2 optimisations)
    sema/                — t001…t004 (error tests: duplicate case, arg count, const-assign)
```

---

## Frontend

### Preprocessor (`src/frontend/preproc.h/cpp`)

Built-in single-pass preprocessor. Handles:
- Object-like and function-like macros (`#define`/`#undef`)
- `#include` with `<>` and `""` forms; `-I` search paths
- `#if`/`#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`; `defined()` operator
- `#error`
- Predefined macros: `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`
- Variadic macros: `__VA_ARGS__`
- Stringify `#` and token-paste `##`
- `_Pragma("...")` — accepted as no-op
- Recursion guard and depth limit (32)
- Emits `# linenum "file"` markers consumed by the lexer

### Lexer (`src/frontend/lexer.h/cpp`)

Hand-written character-by-character scanner. Tracks `file_`, `line_`, `col_`
for diagnostics.

Key methods:
- `next()` — consume and return the next token
- `peek()` — return the lookahead token without consuming
- `lex_one()` — internal; classifies the next character and dispatches

Notable behaviors:
- Lines beginning with `# N "file"` update `file_` and `line_` (preprocessor markers).
- Unicode/wide literal prefixes (`L`, `u`, `U`, `u8`) are consumed; the literal is
  stored with a `char_width` field indicating element byte size.
- All C11 keywords map to dedicated `TK::KW_*` variants.
- GNU aliases (`__volatile__`, `__inline__`, etc.) map to their standard equivalents.

### Token (`src/frontend/token.h`)

```cpp
struct Token {
    TK          kind;
    std::string text;
    int64_t     ival;       // INT_LIT, CHAR_LIT
    double      fval;       // FLOAT_LIT
    std::string sval;       // STRING_LIT (decoded)
    int         char_width; // 1/2/4 for char/char16_t/char32_t strings
    std::string file;
    int         line, col;
};
```

### Types (`src/frontend/types.h/cpp`)

```cpp
enum class type_kind { VOID, BOOL, CHAR, SHORT, INT, LONG, LLONG,
                       FLOAT, DOUBLE, COMPLEX, POINTER, ARRAY, FUNCTION,
                       STRUCT, UNION, ENUM };

struct type {
    type_kind            kind;
    bool                 is_unsigned;
    bool                 is_const, is_volatile, is_restrict;
    type_ptr             base;         // POINTER/ARRAY element; FUNCTION return
    int                  array_len;
    std::vector<type_ptr> params;      // FUNCTION parameter types
    bool                 variadic;
    std::string          tag;          // STRUCT/UNION/ENUM tag name
    std::vector<field>   fields;       // STRUCT/UNION fields (name, type, offset)
    int                  size_bytes;   // cached; -1 = not yet computed
    bool                 is_vla;       // variable-length array flag
};
```

`type->size()` — byte size using Z80 layout rules.
`type->align()` — always 1 on Z80.
`type->is_unsigned()` — true for unsigned integer kinds.

### Symbol Table (`src/frontend/symtab.h/cpp`)

Scoped hash map. Each scope is `std::unordered_map<std::string, symbol_ptr>`.
Scopes are pushed/popped around function bodies and compound statements.

```cpp
enum class sym_kind { VAR, FUNC, TYPE, ENUMERATOR, ENUM_CONST };

struct symbol {
    std::string name;
    sym_kind    kind;
    type_ptr    type;
    int         stack_offset; // IX-relative for locals; 0 for globals
    bool        is_global;
    bool        is_param;
    bool        is_tls;       // _Thread_local
    int64_t     enum_val;     // ENUM_CONST value
    symbol_ptr  vla_size_sym; // hidden size local for VLAs
    storage_class storage;    // NONE, AUTO, STATIC, EXTERN, REGISTER
};
```

Tag namespace (struct/union/enum tags) is a parallel scope stack.

### AST (`src/frontend/ast.h`)

All nodes inherit from:

```cpp
struct decl  { virtual void accept(decl_visitor  &) = 0; location loc; };
struct stmt  { virtual void accept(stmt_visitor  &) = 0; location loc; };
struct expr  { virtual void accept(expr_visitor  &) = 0; location loc; type_ptr type; };
```

**Declaration nodes**: `translation_unit`, `func_decl`, `var_decl`, `typedef_decl`,
`static_assert_decl`

**Statement nodes**: `compound_stmt`, `expr_stmt`, `return_stmt`, `if_stmt`,
`while_stmt`, `do_stmt`, `for_stmt`, `switch_stmt`, `case_stmt`, `default_stmt`,
`break_stmt`, `continue_stmt`, `goto_stmt`, `label_stmt`, `asm_stmt`

**Expression nodes**: `int_literal_expr`, `float_literal_expr`, `char_literal_expr`,
`string_literal_expr`, `ident_expr`, `binary_expr`, `unary_expr`, `cast_expr`,
`call_expr`, `index_expr`, `member_expr`, `sizeof_expr`, `ternary_expr`,
`comma_expr`, `init_list_expr`, `compound_literal_expr`, `generic_selection_expr`

### Parser (`src/frontend/parser.h/cpp`)

Recursive descent with one-token lookahead via `lex_.peek()` / `lex_.next()`.

**Key fields**:
- `syms_` — `symbol_table`; typedef resolution and scope management
- `cur_func_` — pointer to the `func_decl` being parsed
- `local_offset_` — next IX-relative slot for locals (grows negative: −2, −4, …)
- `param_offset_` — next IX-relative slot for parameters (grows positive: +4, +6, …)

**Known limitations**:
- One-token lookahead prevents detection of `IDENT ':'` labels in statement
  position; named label declarations (`foo:`) do not parse inside function bodies.
  `goto foo;` parses correctly but `foo:` itself triggers a parse error.

### Sema (`src/frontend/sema.h/cpp`)

Single-pass AST walker run after parsing. Current checks:
- `const` enforcement: assignment to a `const`-qualified lvalue is an error.

---

## IR

### Operand and icode (`src/ir/icode.h`)

```cpp
enum class operand_kind { NONE, TEMP, SYMBOL, INT_CONST, FLOAT_CONST, LABEL_REF };

struct operand {
    operand_kind  kind;
    int           temp_id;   // TEMP
    symbol_ptr    sym;       // SYMBOL
    int64_t       ival;      // INT_CONST
    double        fval;      // FLOAT_CONST
    std::string   label;     // LABEL_REF
    type_ptr      type;
    int           byte_offset; // for _Complex component addressing
};
```

```cpp
struct icode {
    icode_op     op;
    operand      result, left, right;
    std::string  func_name;    // CALL: non-empty = direct call
    std::string  label_name;   // LABEL / GOTO / JUMP_IF_*
    std::string  asm_text;     // INLINE_ASM verbatim text
    int          num_params;   // CALL: argument count for stack cleanup
    int          line;         // source line (for DWARF)
};
```

**`SET_VALUE_AT` convention**: `ic.result` = pointer operand (address to write to),
`ic.left` = value to store.  Swapping these fields silently stores to address 0.

### ir_optimizer (`include/opt/iropt.h`, `src/opt/iropt.cpp`)

Runs a fixed-point IR pipeline built from pass objects:
1. `cfg_cleanup` — fold constant branches and remove unreachable blocks
2. `value_propagation` — SSA-style reaching-value propagation across basic blocks
3. `constant_fold` — evaluate binary/unary ops on INT_CONST operands
4. `algebraic_simplify` — fold identities: `x+0→x`, `x*0→0`, `x&0→0`, etc.
5. `loop_licm` — hoist loop-invariant pure computations
6. `loop_induction` — rewrite canonical loop multiplies into running values
7. `strength_reduce` — replace constant multiplies/divides/mods with cheaper shifts/adds
8. `dead_code_elim` — remove removable instructions whose results are not live

Activated at `-O2` and above.

---

## Backend

### asm_emitter (`include/backend/asm_emitter.h`)

Abstract base class separating instruction semantics from assembler-dialect syntax.
Key virtual methods:

| Method | Purpose |
|--------|---------|
| `instr(text)` | emit an instruction line |
| `label(name, global)` | emit a label |
| `comment(text)` | emit a comment |
| `raw(text)` | emit verbatim text (inline asm) |
| `section_code/data/rodata/tls()` | emit section switch |
| `global_decl(name)` | declare a global symbol |
| `symbol_assign(name, val)` | emit symbol = constant |
| `db/dw/dl/ds(...)` | emit data |
| `imm(n)` | format an immediate value |
| `ix_rel(offset)` | format an IX-relative address |
| `indir_global(name, offset)` | format an indirect global address |
| `module_header()` | emit file-level preamble (sdasz80 only) |

Two concrete implementations:

- **`sdasz80_emitter`**: `#N` immediates, `N(ix)` addresses, `.area`/`.globl`,
  `.db/.dw/.dl/.ds`, `.module xcc_output` header
- **`gnuas_emitter`**: `N` immediates, `(ix+N)` addresses, `.text`/`.global`,
  `.byte/.short/.long/.space`, no module header, `.set` for symbol assignment

Select with `-masm=sdasz80` (default) or `-masm=gnuas`.

### Z80Gen (`src/backend/z80/z80gen.h/cpp`)

Walks `ir_module` and emits Z80 assembly through the `asm_emitter` interface.
Each `ir_function` becomes an assembly function with an IX-based stack frame.

**Frame layout** (xcc Z80 ABI):

```
higher addresses
  IX+6   param 2
  IX+4   param 1 (first / leftmost argument)
  IX+2   return address
  IX+0   saved old IX (push ix)
  IX-2   local 1
  IX-4   local 2
  ...    temporaries (below locals; 2 bytes each)
lower addresses
```

Caller pushes parameters right-to-left, calls, callee does
`push ix` / `ld ix,#0` / `add ix,sp`.  Caller cleans the stack.

**Key helpers**:
- `load_hl(op)` — load operand into HL (from IX offset, global, constant, temp slot)
- `load_de(op)` — load into DE
- `load_a(op)` — load into A (1-byte values)
- `store_hl(op)` — store HL to operand location
- `alloc_temp(id)` — assign a stack slot for a TEMP (grows downward, 2 bytes)

**Optimization at -O2**: `regalloc_prepass()` performs a bounded linear-scan
allocation of one 16-bit temp to BC inside short straight-line windows. Live
interval analysis plus backend hazard checks keep the stable allocator away
from CALL / DIV / MUL / shift-helper windows and known BC-scratch sites.
Register homes are tracked with the `temp_home` enum (`stack`, `main_bc`,
`alt_a`, and reserved `alt_bc`/`alt_de`/`alt_hl` for future EXX-region support).

**Inline constant shifts**: shift-by-0 → no-op; shift-by-1/2 → unrolled bit ops;
shift-by-8 → byte-move trick; shift by other constants → B-register counted loop.

**`inc`/`dec` for ±1**: `ADD(x, 1)` → `inc hl`; `SUB(x, 1)` → `dec hl`.

### Z80Peep (`src/backend/z80/z80peep.h/cpp`)

Text-level peephole optimizer.  Input/output: assembly text string.
Runs multiple passes until a fixed point is reached (up to 10 passes).

Each line is parsed into an `asm_line` struct (`label`, `mnemonic`, `operands`, `comment`).

**Current rules**:

| Rule | Pattern | Replacement |
|------|---------|-------------|
| `rule_redundant_ld` | `ld r, r` | removed (self-load no-op) |
| `rule_push_pop_hl` | `push hl; pop hl` | removed |
| `rule_push_hl_pop_de` | `push hl; pop de` | `ex de,hl` |
| `rule_push_hl_load_pop_de` | `push hl; ld hl,X; pop de` | `ex de,hl; ld hl,X` |
| `rule_push_hl_ix_pop_de` | `push hl; ld l,N(ix); ld h,N+1(ix); pop de` | `ex de,hl; ld l,N(ix); ld h,N+1(ix)` |
| `rule_push_hl_de_load` | `push hl; ld de,#imm; pop hl` | `ld de,#imm` |
| `rule_jp_next` | `jp label; label:` | removed |
| `rule_or_a_or_a` | `or a,a; or a,a` | remove second |
| `rule_temp_store_reload` | `dec sp;dec sp;ld N(ix),l;ld N+1(ix),h;ld l,N(ix);ld h,N+1(ix)` | removed |
| `rule_self_store` | `ld a,(ix+N); ld (ix+N),a` | removed |
| `rule_dead_hl_load` | `ld hl,#imm; ld hl,X` | remove first |
| `rule_ld_a_zero` | `ld a,#0` (not before conditional branch) | `xor a` |
| `rule_ix_store_reload` | `ld N(ix),l;ld N+1(ix),h;ld l,N(ix);ld h,N+1(ix)` | first 2 only |
| `rule_ix_byte_store_reload` | `ld N(ix),a; ld a,N(ix)` | first only |
| `rule_dead_hl_ix_load` | `ld l,N(ix);ld h,N+1(ix);ld l,M(ix);ld h,M+1(ix)` | last 2 only |
| `rule_push_hl_pop_bc` | `push hl; pop bc` | `ld b,h; ld c,l` |
| `rule_jp_to_jr` | `jp [cc,] L` (L within ±30 lines) | `jr [cc,] L` |
| `rule_zero_cmp_optimize` | `push hl;ld hl,#0;pop de;or a,a;sbc hl,de` | `ld a,h; or a,l` |
| `rule_ex_de_hl_load_double` | `ld l,A(ix);ld h,A+1(ix);ex de,hl;ld l,B(ix);ld h,B+1(ix);ex de,hl` | 4 direct IX loads |
| `rule_bool_ifx_shortcircuit` | boolean-gen + IFX test sequence (9 insns) | single direct branch |
| `rule_invert_branch_skip` | `jr cc,L; jr L_end; L:` | `jr !cc,L_end; L:` |

Activated at `-O1` and above.

### DWARF (`src/backend/z80/dwarf.h/cpp`)

Emits DWARF 2 debug info sections when compiled with `-g`:
`.debug_abbrev`, `.debug_info`, `.debug_aranges`.
Inline `.file`/`.loc` directives track source positions.

---

## Runtime Library (`lib/runtime.s`)

SDAS Z80 assembler source.  Link with every xcc-compiled program.

| Symbol | Description | Status |
|--------|-------------|--------|
| `__mul16` | 16-bit signed multiply; args pushed on stack; result in HL | working |
| `__div16` | 16-bit signed divide; result in HL, remainder in DE | working |
| `__mod16` | 16-bit signed modulo; result in DE | working |
| `__mul32` | 32-bit multiply; args pushed as DE:HL pairs; result in DE:HL | working |
| `__div32` | 32-bit divide; result in DE:HL | working |
| `__mod32` | 32-bit modulo; result in DE:HL | working |
| `__call_hl` | Indirect call trampoline: `jp (hl)` | working |
| `__fsadd/__fssub/__fsmul/__fsdiv` | Soft-float arithmetic | working |
| `__fitosf/__fstoi` | Integer ↔ soft-float conversion | working |
| `__mulll/__divll/__modll` | 64-bit multiply/divide/modulo | working |
| `__atomic_*` (18 symbols) | Atomic ops via DI/EI | **stub** |

---

## ABI: xcc Z80 calling convention

| Item | Convention |
|------|------------|
| Parameter passing | Right-to-left, all on stack |
| Return ≤ 1 byte | L register |
| Return ≤ 2 bytes | HL |
| Return ≤ 4 bytes | DE:HL (high in DE, low in HL) |
| Frame pointer | IX |
| Stack cleanup | ABI-sensitive: `sdcccall(0)` is caller-clean; non-variadic `sdcccall(1)` follows SDCC's return-sensitive callee-clean rule |
| First param | `IX+4` |
| First local | `IX-2` |
| Callee-save | IX only (via `push ix` / `pop ix` in prologue/epilogue) |

---

## How to extend

### Add a new IR operation

1. Add the opcode to `icode_op` in `include/ir/icode.h`.
2. Emit it in `ir_gen` at the appropriate AST visitor (`src/ir/irgen.cpp`).
3. Handle it in `z80_gen`'s main dispatch switch (`src/backend/z80/z80gen.cpp`).
4. Optionally add a peephole rule in `lib/xopt/src/z80peep.cpp`.
5. Add or update a test in `tests/data/core/`.

### Add a new statement

1. Add an AST node inheriting from `stmt` in `include/frontend/ast.h`.
   Add `accept` and the `visit` pure virtual to all visitor interfaces.
2. Parse it in `parser::parse_statement()` in `src/frontend/parser.cpp`.
3. Implement `ir_gen::visit(new_stmt &)` in `src/ir/irgen.cpp`.
4. No backend changes needed unless you add new IR ops.

### Regenerate test baselines

After an intentional codegen change:

```sh
GENERATE=1 bash tests/run_tests.sh ./build/bin/xcc
```

This overwrites the `.expected` files in `tests/data/core/`.
Review the diff before committing.
