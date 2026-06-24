extern int foreign_elf_add(int a, int b);

int call_foreign_archive(int a, int b) {
    return foreign_elf_add(a, b);
}
