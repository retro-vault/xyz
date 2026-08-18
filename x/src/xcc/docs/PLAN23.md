# xcc C23 Upgrade Plan

## Purpose

This document is a **prompt-first implementation plan** for upgrading xcc from C11 to C23.
Each chunk is self-contained and written so it can be handed directly to an AI coding session
to implement that chunk without re-reading the whole history.

Primary motivation: C23 `[[attributes]]` to enable per-function SDCC ABI selection
(e.g. `[[xcc::sdcccall(1)]]` to opt into the register-efficient SDCC calling convention).
Full C23 upgrade is done alongside because many features share infrastructure.

## Quick orientation (read before any chunk)

xcc is a hand-written recursive-descent C compiler targeting Z80 via SDCC assembler syntax.

```
x/src/xcc/
├── include/frontend/   token.h  lexer.h  parser.h  ast.h  types.h  symtab.h  sema.h
├── include/ir/         icode.h  irgen.h
├── include/opt/        iropt.h
├── include/backend/z80/z80gen.h  (+ subheaders)
├── src/frontend/       lexer.cpp  parser.cpp  parser_declspec.cpp  parser_declarator.cpp
│                       parser_stmt.cpp  parser_expr.cpp  parser_init.cpp
│                       types.cpp  symtab.cpp  sema.cpp
├── src/ir/             icode.cpp  irgen.cpp  irgen_decl.cpp  irgen_stmt.cpp
│                       irgen_expr.cpp  irgen_lvalue.cpp  irgen_init.cpp
├── src/opt/            iropt.cpp
├── src/backend/z80/    z80gen.cpp  z80gen_ctrl.cpp  z80gen_data.cpp  z80gen_operand.cpp
│                       z80gen_mem.cpp  z80gen_arith.cpp  z80gen_regalloc.cpp
│                       z80peep.cpp  dwarf.cpp  sdcc_debug.cpp
│                       sdasz80_emitter.cpp  gnuas_emitter.cpp
├── src/driver/         main.cpp  options.cpp
├── tests/              run_tests.sh  data/core/t001-t050  data/sema/  data/opt/
└── docs/               ARCHITECTURE.md  C11.md  IMPLEMENTATION_STATUS.md  ...
```

**Current language level:** C11 subset.  `__attribute__((...))` is lexed but discarded via
`skip_attribute()` in `src/frontend/parser.cpp:67`.  No `[[...]]` syntax exists yet.

**Build:** `make` (debug with ASan/UBSan), `make BUILD=release`, `make test` (47 core tests).
Test framework: snapshot comparison of emitted Z80 assembly.

**Fixed Z80 ABI (what we must make switchable per chunk 8):**
- Parameters: right-to-left on stack
- Return ≤1 byte: L; ≤2 bytes: HL; ≤4 bytes: DE:HL
- Frame pointer: IX; stack cleanup: caller; callee-save: IX only

---

## Chunk 1 — Attribute token infrastructure

**Goal:** Lex `[[` and `]]` as dedicated tokens, define the attribute value union, and add
attribute-list helpers used by every later chunk.  No semantic change yet.

**Files to modify:**

`include/frontend/token.h`
- Add two new token kinds after the existing operator block:
  ```cpp
  LATTR,   // [[
  RATTR,   // ]]
  ```
- Add a helper `is_attr_open()` / `is_attr_close()` on `token` similar to existing `is_keyword()`.

`src/frontend/lexer.cpp`
- In `lex_operator()` (wherever `[` is handled): if the current char is `[` and the *next* char
  (one-ahead peek without consuming) is also `[`, consume both and return `LATTR`.
  Similarly for `]` + `]` → `RATTR`.  Single `[` / `]` remain `LBRACKET` / `RBRACKET`.

`include/frontend/ast.h`
- Add an attribute representation near the top of the file:
  ```cpp
  // Attribute name resolution: "namespace::name" or just "name".
  // args is the token-literal argument list (empty when no parens).
  struct attr {
      std::string ns;       // empty for standard attrs
      std::string name;
      std::vector<std::string> args;  // raw string arguments
      source_loc loc;
  };
  using attr_list = std::vector<attr>;
  ```
- This is the canonical attribute representation used across AST, symbol table, and IR.

**Success criteria:**
- `make` (debug) succeeds with no new warnings.
- `make test` still shows all 47 core tests passing (no behaviour change yet).

---

## Chunk 2 — Attribute parser

**Goal:** Parse `[[attr]]`, `[[ns::attr]]`, `[[attr(args)]]` sequences at all declaration sites
and store them in the AST.  Attributes are parsed but not yet acted on.

**Files to modify:**

`src/frontend/parser.cpp`
- Replace `skip_attribute()` with a new method:
  ```cpp
  attr_list parser::parse_attr_list();
  ```
  Logic:
  1. If `check(LATTR)`, consume it.
  2. Loop: parse one attribute per iteration.
     - Read identifier → `name`.
     - If next is `COLON COLON`, consume both and read another identifier → `ns = name; name = second`.
     - If next is `LPAREN`, consume and collect comma-separated token sequences (balanced parens)
       as raw `args` strings until the matching `RPAREN`.
     - If next is `COMMA`, consume and continue the inner loop.
     - Otherwise break.
  3. Expect `RATTR`.
  4. Multiple `[[...]]` sequences in a row are concatenated.
  - Keep `skip_attribute()` as a thin wrapper that calls `parse_attr_list()` and discards the
    result — required for backwards compatibility with `__attribute__((...))`; do NOT delete it.

`include/frontend/parser.h`
- Declare `attr_list parse_attr_list();`

`src/frontend/parser_declspec.cpp`  *(declaration-specifiers)*
- In `parse_declaration_specifiers()`, wherever `__attribute__` is currently skipped, also
  detect `LATTR` and call `parse_attr_list()`, accumulating into a `attr_list` field in
  `decl_spec`.

`include/frontend/parser.h`
- Extend `struct decl_spec` to add `attr_list attrs;`.

`src/frontend/parser_declarator.cpp`  *(declarators)*
- After parsing each declarator (function prototype, variable, pointer level) check for
  a leading or trailing `[[...]]` and add the attrs to the `declarator_info`.
- Extend `struct declarator_info` to add `attr_list attrs;`.

`src/frontend/parser_stmt.cpp`  *(statements)*
- Before parsing a statement, detect `[[fallthrough]]` / `[[maybe_unused]]` at label position
  and store on the statement node (extend stmt base or as a wrapper — your choice).

`include/frontend/ast.h` — extend AST nodes:
- `func_decl`: add `attr_list attrs;`
- `var_decl`:  add `attr_list attrs;`

**Success criteria:**
- All 47 core tests still pass.
- A new sema test `data/sema/attr_parse.c` containing:
  ```c
  [[nodiscard]] int f(void);
  [[xcc::sdcccall(1)]] void g(void);
  [[deprecated("use g")]] void h(void);
  ```
  compiles without error and emits no diagnostics (attributes are stored, not yet validated).

---

## Chunk 3 — Standard C23 attribute semantics

**Goal:** Implement the six standard C23 attributes with diagnostics and where possible codegen
effects.  Vendor attributes (chunk 4) and ABI variants (chunk 8) are separate.

Standard attributes and their required semantics:

| Attribute          | Target          | Effect                                                   |
|--------------------|-----------------|----------------------------------------------------------|
| `[[noreturn]]`     | function decl   | Existing `_Noreturn` codegen; warn if function may return|
| `[[deprecated]]`   | any decl        | Diagnostic warning at each use site                      |
| `[[deprecated("m")]` | any decl      | Same, with custom message                                |
| `[[nodiscard]]`    | function / type | Warning when return value is discarded                   |
| `[[nodiscard("m")]` | function / type| Same, with custom message                                |
| `[[maybe_unused]]` | any decl        | Suppress unused-variable/param warnings                  |
| `[[fallthrough]]`  | in switch body  | Suppress implicit fallthrough warning                    |
| `[[unsequenced]]`  | function        | Informational (store, no codegen)                        |
| `[[reproducible]]` | function        | Informational (store, no codegen)                        |

**Files to modify:**

`include/frontend/symtab.h` — extend `struct symbol`:
```cpp
bool attr_noreturn   = false;
bool attr_deprecated = false;
std::string deprecated_msg;
bool attr_nodiscard  = false;
std::string nodiscard_msg;
bool attr_maybe_unused = false;
bool attr_unsequenced  = false;
bool attr_reproducible = false;
```

`src/frontend/sema.cpp` — new pass: `apply_attrs()`
- Walk the translation unit after parsing.
- For each `func_decl` / `var_decl` with a non-empty `attrs` list, call `apply_attrs(sym, attrs)`.
- `apply_attrs` iterates the attr_list:
  - `noreturn` (no ns) → set `sym->attr_noreturn = true`; also accept `_Noreturn` keyword unification.
  - `deprecated` → set flag + optional message.
  - `nodiscard`  → set flag + optional message.
  - `maybe_unused` → set flag.
  - `unsequenced` / `reproducible` → set flags.
  - `fallthrough` → validate that parent statement is inside a switch body.
  - Unknown attribute with no namespace → emit **warning** (C23 requires unknown standard attrs
    to be diagnosed, but not to be fatal).
  - Unknown attribute with a namespace → silently ignore (extension space).

`src/frontend/sema.cpp` — use site checks:
- Call-expression resolution: if callee symbol has `attr_deprecated`, emit a warning.
- Call-expression: if callee has `attr_nodiscard` and the call is used as an expression-statement,
  emit a warning.

`src/ir/irgen_decl.cpp`
- When emitting a function, copy `attr_noreturn` from symbol to `ir_function` (add a bool field
  there).  The Z80 backend will use this in chunk 8's prologue/epilogue logic.

**Success criteria:**
- New sema test `data/sema/attr_deprecated.c`:
  ```c
  [[deprecated("old API")]] void legacy(void);
  void caller(void) { legacy(); }  /* expected: warning: 'legacy' is deprecated */
  ```
- New sema test `data/sema/attr_nodiscard.c`:
  ```c
  [[nodiscard]] int important(void);
  void caller(void) { important(); }  /* expected: warning: return value discarded */
  ```
- All 47 core tests still pass.

---

## Chunk 4 — Vendor attributes for SDCC ABI selection

**Goal:** Add the `xcc::` vendor namespace and the first custom attribute: `[[xcc::sdcccall(N)]]`
where N is 0 (stack ABI, current default) or 1 (register ABI).  Also add `[[xcc::naked]]` for
functions that need no prologue/epilogue at all.

**Files to modify:**

`include/frontend/symtab.h` — extend `struct symbol`:
```cpp
enum class call_abi { DEFAULT, SDCCCALL0, SDCCCALL1, NAKED };
call_abi abi = call_abi::DEFAULT;
```

`src/frontend/sema.cpp` — in `apply_attrs()`:
- If `ns == "xcc"` and `name == "sdcccall"`:
  - Expect exactly one integer argument.
  - Parse it: `0` → `SDCCCALL0`; `1` → `SDCCCALL1`; otherwise error.
  - Set `sym->abi`.
- If `ns == "xcc"` and `name == "naked"`:
  - No arguments allowed.
  - Set `sym->abi = call_abi::NAKED`.
- If `ns == "xcc"` and name is anything else → warning "unknown xcc attribute".

`include/ir/icode.h` — extend `struct ir_function`:
```cpp
enum class call_abi { DEFAULT, SDCCCALL0, SDCCCALL1, NAKED };
call_abi abi = call_abi::DEFAULT;
```

`src/ir/irgen_decl.cpp` — when building `ir_function` from `func_decl`, copy `sym->abi` to
`ir_func->abi`.

**Success criteria:**
- New test `data/core/t051_sdcccall_attr.c`:
  ```c
  [[xcc::sdcccall(1)]] int add(int a, int b);
  ```
  compiles without error; the symbol table entry for `add` has `abi == SDCCCALL1`.
  (Backend does not yet use it; that is chunk 8.)
- `[[xcc::sdcccall(2)]]` produces a compile error.
- All 47 core tests still pass.

---

## Chunk 5 — New C23 type keywords

**Goal:** Add the C23 keywords that enter the language proper: `bool`, `true`, `false`, `nullptr`,
`typeof`, `typeof_unqual`, `constexpr`.

### 5a — `bool`, `true`, `false` as first-class keywords

C23 makes `bool`, `true`, `false` keywords (no longer requiring `<stdbool.h>`).

`include/frontend/token.h`:
- Add `KW_BOOL`, `KW_TRUE`, `KW_FALSE` to the keyword range.

`src/frontend/lexer.cpp` — keyword map:
- Add `"bool"` → `KW_BOOL`, `"true"` → `KW_TRUE`, `"false"` → `KW_FALSE`.

`src/frontend/parser_declspec.cpp`:
- `KW_BOOL` → same handling as existing `KW__BOOL` (they become aliases).

`src/frontend/parser_expr.cpp`:
- `KW_TRUE`  → `int_literal_expr(1)` with type `_Bool`.
- `KW_FALSE` → `int_literal_expr(0)` with type `_Bool`.

### 5b — `nullptr`

`include/frontend/token.h`: Add `KW_NULLPTR`.
`src/frontend/lexer.cpp`: `"nullptr"` → `KW_NULLPTR`.
`src/frontend/parser_expr.cpp`:
- `KW_NULLPTR` → `int_literal_expr(0)` typed as `void *` (null pointer constant).

### 5c — `typeof` and `typeof_unqual`

xcc already implements `__typeof__`.  C23 adds spellings without underscores.

`include/frontend/token.h`: Add `KW_TYPEOF`, `KW_TYPEOF_UNQUAL`.
`src/frontend/lexer.cpp`: `"typeof"` → `KW_TYPEOF`, `"typeof_unqual"` → `KW_TYPEOF_UNQUAL`.
`src/frontend/parser_expr.cpp`:
- Both `KW_TYPEOF` and `KW___TYPEOF__` call the same existing typeof parsing logic.
- `KW_TYPEOF_UNQUAL` does the same but strips top-level qualifiers from the resulting type
  (call `type::unqualified()` or equivalent).

### 5d — `constexpr`

C23 `constexpr` at file scope declares an object that must be initialised with a constant
expression and is implicitly `const`.

`include/frontend/token.h`: Add `KW_CONSTEXPR`.
`src/frontend/lexer.cpp`: `"constexpr"` → `KW_CONSTEXPR`.
`src/frontend/parser_declspec.cpp`:
- Treat `KW_CONSTEXPR` as storage class `STATIC` (file scope) or `AUTO` (block scope) combined
  with `const` qualifier.  Set a `bool is_constexpr` flag in `decl_spec` for sema.
`src/frontend/sema.cpp`:
- If `is_constexpr`, require that the initialiser is a constant expression (already validated for
  array sizes; extract the helper and reuse it).

**Success criteria:**
- `bool b = true;` compiles without `<stdbool.h>`.
- `int *p = nullptr;` compiles; `p` is a null pointer.
- `typeof(1 + 1) x = 2;` works (reuses `__typeof__` logic).
- `typeof_unqual(const int) y = 3;` gives a non-const int.
- `constexpr int N = 8;` at file scope; `int arr[N];` compiles.
- All 47 core tests still pass.

---

## Chunk 6 — Binary literals and digit separators

**Goal:** C23 adds `0b101010` binary integer literals and `'` digit separators (e.g. `1'000'000`).

`src/frontend/lexer.cpp` — in `lex_number()`:
- **Binary:** if the prefix is `0b` or `0B`, consume binary digits (`0`/`1`) and parse as base 2.
  Same suffix handling as hex (`u`, `U`, `l`, `L`, `ll`, `LL`).
- **Digit separators:** when scanning any numeric literal (decimal, hex, octal, binary), skip
  `'` characters between digits.  Two consecutive `'` or a leading/trailing `'` is a lex error.

**Success criteria:**
- `int x = 0b1010;` → x == 10.
- `int y = 1'000'000;` → y == 1000000.
- `int z = 0xFF'FF;` → z == 65535.
- New core test `data/core/t052_binary_literals.c` passes.
- All 47 core tests still pass.

---

## Chunk 7 — `_BitInt(N)` bit-precise integers

**Goal:** Parse and type-check `_BitInt(N)` and `unsigned _BitInt(N)`.  Codegen falls back to the
nearest standard integer type (8/16/32) with a stub comment.  This is a C23 required type.

`include/frontend/token.h`: Add `KW__BITINT`.
`src/frontend/lexer.cpp`: `"_BitInt"` → `KW__BITINT`.

`include/frontend/types.h`:
- Add a new type kind `BITINT`.
- Add field `int bitint_width` to `struct type`.
- Add factory `type::make_bitint(int width, bool is_unsigned)`.

`src/frontend/parser_declspec.cpp`:
- Parse `_BitInt(constant-expression)` → call `type::make_bitint(N, is_unsigned)`.
- N must be ≥ 1 and ≤ 64 (or platform max).

`src/frontend/types.cpp`:
- `size_of(BITINT)`:  `(width + 7) / 8`, clamped to {1, 2, 4} (Z80 platform max 32-bit useful).
- `align_of(BITINT)`: same as `size_of`.

`src/ir/irgen_decl.cpp` / `src/ir/irgen_expr.cpp`:
- Treat `BITINT` as its backing integer type for IR purposes.

**Success criteria:**
- `_BitInt(7) x = 0;` parses and compiles.
- `unsigned _BitInt(16) y = 0xFFFF;` parses and compiles.
- `_BitInt(0)` is a compile error.
- `_BitInt(65)` is a compile error (exceeds platform support, emit an error).
- New sema test `data/sema/bitint.c` covering the error cases.
- All 47 core tests still pass.

---

## Chunk 8 — Z80 backend ABI variants

**Goal:** Implement the actual calling-convention variants controlled by the attributes from
chunk 4.  This is the payoff chunk: functions decorated with `[[xcc::sdcccall(1)]]` will use
a register-based ABI; `[[xcc::naked]]` functions will have no prologue/epilogue.

### 8a — SDCCCALL(0): explicit stack ABI (current default)

This is the existing behaviour.  The only change is to make it explicit: when `ir_function::abi`
is `SDCCCALL0` or `DEFAULT`, the current code path is taken unchanged.

### 8b — SDCCCALL(1): register-based ABI

XCC's `[[sdcc::sdccall(1)]]` attribute passes arguments in registers (IY, HL, DE, BC in order) and returns
in HL/DE:HL.  Implement a matching variant.

`include/backend/z80/z80gen.h`:
- Add `enum class z80_abi { STACK, REG, NAKED };`
- Add `z80_abi abi_of(const ir_function &) const;` helper that maps `call_abi` → `z80_abi`.
- Add `void emit_prologue_reg(...)` and `void emit_epilogue_reg(...)` alongside the existing
  `emit_prologue` / `emit_epilogue`.
- Add `void emit_prologue_naked(...)` (empty body) and `emit_epilogue_naked(...)` (empty body).

`src/backend/z80/z80gen_ctrl.cpp`:
- In the function-entry dispatch, call `abi_of(fn)` and branch to the appropriate prologue.
- **REG prologue:**
  - No `push ix` / `ld ix, #0` / `add ix, sp`.
  - Parameters arrive in registers; emit `ld (ix+offset), reg` stores to put them in the
    frame where the rest of codegen expects them.  Order: first param in HL, second in DE,
    third in BC, fourth on stack (fallback).
- **REG epilogue:**
  - Load return value into HL (or DE:HL).
  - No frame teardown (`pop ix` not needed if no push ix in prologue).
  - `ret`.
- **NAKED prologue/epilogue:** emit nothing.

`src/backend/z80/z80gen.cpp` — call sites:
- Wherever the compiler emits a *call instruction* to a function whose symbol has `abi == SDCCCALL1`,
  use the register-passing calling sequence instead of stack pushes.
  - This requires looking up the callee symbol's ABI from the symbol in the call IR node.
  - Add `call_abi callee_abi` to the call IR node if not already present.

`src/ir/irgen_expr.cpp` — call-expression visitor:
- When emitting `CALL` IR for a call to a function with `abi == SDCCCALL1`, annotate the IR
  call node with `callee_abi = SDCCCALL1` so the backend can see it without re-looking up the
  symbol.

**Success criteria:**
- New test `data/core/t053_sdcccall1.c`:
  ```c
  [[xcc::sdcccall(1)]] int add(int a, int b) { return a + b; }
  int main(void) { return add(1, 2); }
  ```
  Emitted assembly: `add`'s prologue has no `push ix`; parameters arrive in HL, DE;
  call site loads HL/DE before `call _add` instead of pushing.
- New test `data/core/t054_naked.c`:
  ```c
  [[xcc::naked]] void isr(void) { __asm__("reti"); }
  ```
  Emitted assembly has no prologue/epilogue; just the inline asm.
- All 47 core tests still pass (default ABI is untouched).

---

## Chunk 9 — Empty parameter list change (C23 semantics)

**Goal:** In C23, `int f()` means `int f(void)` — an empty parameter list is not an unprototyped
function.  In C11 `int f()` was an old-style unprototyped declaration.

`src/frontend/parser_declarator.cpp` — in `parse_param_list()`:
- Currently an empty `()` is stored as "unprototyped" (zero params, no prototype flag).
- Change: empty `()` sets `is_variadic = false` and produces an empty-but-prototyped param list
  (same as `(void)`).

`include/frontend/parser.h` — in `declarator_info`:
- Remove or repurpose the `is_unprototyped` flag; in C23 mode there are no unprototyped functions.

`src/frontend/sema.cpp`:
- Remove any suppression of argument-count checks for unprototyped functions.

**Note:** This may require updating a handful of existing tests where the expected assembly
comment or symbol metadata reflected the old "unprototyped" distinction.  Run `make test` with
`GENERATE=1` after verifying the new behaviour is correct.

**Success criteria:**
- `int f(); int main(void) { f(1); }` now produces "too many arguments" error (it was silently
  allowed in C11 unprototyped mode).
- All 47 core tests still pass (none should rely on old-style unprototyped semantics).

---

## Chunk 10 — `auto` type deduction

**Goal:** C23 allows `auto x = expr;` where the type is deduced from the initialiser (like C++
`auto`).  This is distinct from the storage-class `auto` which remains valid.

`src/frontend/parser_declspec.cpp`:
- Distinguish between `auto` used as storage class (which remains legal) and `auto` used as a
  type specifier: if `KW_AUTO` appears **without** any other type specifier and the declaration
  has an initialiser, treat it as deduced type.
- Store a `bool is_deduced = true` flag in `decl_spec` when this case is detected.

`src/frontend/parser.cpp` / `src/frontend/parser_declarator.cpp`:
- When `is_deduced`, do not try to resolve the type from `decl_spec`; instead leave type as
  `nullptr` in the `var_decl` AST node.

`src/frontend/sema.cpp` — new `resolve_deduced_types()` pass after parsing:
- For each `var_decl` with `type == nullptr` and a non-null `init`, set `decl->type = init->type`.
- If there is no initialiser, emit error: "`auto` variable requires an initialiser".

`src/frontend/symtab.cpp`:
- After the sema pass resolves the type, update the symbol's type pointer.

**Success criteria:**
- `auto x = 42;` → x has type `int`.
- `auto p = (void *)0;` → p has type `void *`.
- `auto y;` → compile error.
- `auto z = 1; auto z2 = z;` → z2 has type `int`.
- New test `data/core/t055_auto_deduction.c`.
- All 47 core tests still pass.

---

## Chunk 11 — Documentation update

**Goal:** Bring the existing docs up to date with the new language level.

Files to update:

`README.md`:
- Change language level claim from "C11" to "C23 (subset)".
- Add C23 feature table alongside the existing C11 table.
- Document the `[[xcc::sdcccall(N)]]` and `[[xcc::naked]]` vendor attributes with examples.

`docs/C11.md`:
- Rename or supplement with a `docs/C23.md` covering C23 feature status using the same format.
- Explicitly note which C23 features are: fully implemented, partially implemented, parsed-only,
  or not yet planned.

`docs/IMPLEMENTATION_STATUS.md`:
- Add C23 rows for: `[[attributes]]`, `bool/true/false`, `nullptr`, `typeof/typeof_unqual`,
  `constexpr`, `_BitInt(N)`, binary literals, digit separators, auto deduction, empty param list.

`docs/ARCHITECTURE.md`:
- Add a section describing how attributes flow: parser → attr_list → sema apply_attrs →
  symbol flags → IR → Z80 backend ABI variant dispatch.

**No success criteria beyond review** — documentation does not affect test results.

---

## Chunk ordering and dependencies

```
Chunk 1  (tokens)
    └─► Chunk 2  (parser)
            ├─► Chunk 3  (standard attrs + sema)
            │       └─► Chunk 4  (vendor attrs)
            │                   └─► Chunk 8  (Z80 ABI variants)  ← primary goal
            ├─► Chunk 5  (new keywords)
            ├─► Chunk 6  (binary literals)   ← independent of attrs
            ├─► Chunk 7  (_BitInt)           ← independent of attrs
            ├─► Chunk 9  (empty param list)  ← independent of attrs
            └─► Chunk 10 (auto deduction)
Chunk 11 (docs)  ← any time after all others complete
```

Chunks 6, 7, 9 are independent of the attribute pipeline and can be done in parallel with
chunks 3–4 if desired.

---

## How to use this document

To implement a chunk, start a new session and give the AI agent this prompt prefix:

> "Read `x/src/xcc/docs/PLAN23.md` in the repository.
> Read the 'Quick orientation' section and then implement **Chunk N — [name]** exactly as
> described.  After implementation run `make` and `make test`; fix any failures before reporting
> done.  Do not implement any other chunk."

Each chunk lists the files to modify, the exact struct/enum changes to make, and the success
criteria.  Treat the success criteria as the definition of done.
