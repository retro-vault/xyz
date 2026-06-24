# xcc Legacy Harness

These files are the pre-unification `xcc` suite entrypoints.

They remain here for:

- reference
- codegen benchmark reuse
- future migration work where an old harness still captures behavior the new semantic suite does not

Not migrated into the active unified semantic run:

- `x/tests/tests/xcc/data/opt/` optimizer/codegen shape checks
- compile-only imported-ABI/codegen cases that depended on support artifacts:
  - `t080_imported_rel_abi`
  - `t081_imported_rel_abi_override`
  - `t082_imported_elf_abi`
  - `t083_imported_text_lib_abi`
  - `t084_imported_gnu_archive_abi`
