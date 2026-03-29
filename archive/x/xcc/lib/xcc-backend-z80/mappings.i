/*
 * Assembler syntax mappings for various Z80 targets
 *
 * This module provides pattern-based output string mappings used to translate
 * SDCC internal IR into assembler-specific syntax. Targets include ASxxxx,
 * z80asm, ISAS, and GAS-compatible assemblers.
 *
 * Copyright (C) various contributors (see git history)
 * Released under the GNU General Public License v2 or later.
 */

static const ASM_MAPPING _asxxxx_z80_mapping[] = {
    {"area", ".area _%s"},
    {"areacode", ".area _%s"},
    {"areadata", ".area _%s"},
    {"areahome", ".area _%s"},
    {"*ixx", "%d (ix)"},
    {"*iyx", "%d (iy)"},
    {"*hl", "(hl)"},
    {"jphl", "jp (hl)"},
    {"di", "di"},
    {"ei", "ei"},
    {"ldahli",
     "ld a, (hl)\n"
     "inc\thl"},
    {"ldahld",
     "ld a, (hl)\n"
     "dec\thl"},
    {"lldahli",
     "ld (hl), a\n"
     "inc\thl"},
    {"lldahld",
     "ld (hl), a\n"
     "dec\thl"},
    {"ldahlsp",
     "ld hl, #%d\n"
     "add\thl, sp"},
    {"ldaspsp",
     "ld iy,#%d\n"
     "add\tiy,sp\n"
     "ld\tsp,iy"},
    {"mems", "(%s)"},
    {"enter",
     "push\tix\n"
     "ld\tix,#0\n"
     "add\tix,sp"},
    {"enters",
     "call\t___sdcc_enter_ix\n"},
    {"pusha",
     "push af\n"
     "push\tbc\n"
     "push\tde\n"
     "push\thl\n"
     "push\tiy"},
    {"popa",
     "pop iy\n"
     "pop\thl\n"
     "pop\tde\n"
     "pop\tbc\n"
     "pop\taf"},
    {"adjustsp", "lda sp,-%d(sp)"},
    {"here", "."},
    {"optsdcc", ".optsdcc"},
    {NULL, NULL}};

static const ASM_MAPPING _isas_mapping[] = {
    {"global", "GLOBAL %s"},
    {"extern", "GLOBAL %s"},
    {"slabeldef", "%s:"},
    {"labeldef", "%s:"},
    {"tlabeldef", "?l%05d:"},
    {"tlabel", "?l%05d"},
    {"fileprelude",
     ";Generated using the isas tokens.\n"
     "\tLPREFIX '?'  ; Treat labels starting with ? as local.\n"
     "\tONCNUM       ; Numbers are hex\n"
     "\tCAPSOFF      ; Case sensitive\n"
     "\tISDMG        ; Gameboy mode\n"
     "_CODE\tGROUP\n"
     "\t; We have to define these here as sdcc doesn't make them global by default\n"
     "\tGLOBAL __mulschar\n"
     "\tGLOBAL __muluchar\n"
     "\tGLOBAL __mulint\n"
     "\tGLOBAL __divschar\n"
     "\tGLOBAL __divuchar\n"
     "\tGLOBAL __divsint\n"
     "\tGLOBAL __divuint\n"
     "\tGLOBAL __modschar\n"
     "\tGLOBAL __moduchar\n"
     "\tGLOBAL __modsint\n"
     "\tGLOBAL __moduint\n"
     "\tGLOBAL banked_call\n"
     "\tGLOBAL banked_ret\n"},
    {"functionheader",
     "; ---------------------------------\n"
     "; Function %s\n"
     "; ---------------------------------"},
    {"functionlabeldef", "%s:"},
    {"globalfunctionlabeldef", "%s::"},
    {"zero", "$00"},
    {"one", "$01"},
    {"area", "%s\tGROUP"},
    {"areacode", "_CODE\tGROUP"},
    {"areadata", "_DATA\tGROUP"},
    {"areahome", "_CODE\tGROUP"},
    {"ascii", "DB \"%s\""},
    {"ds", "DS %d"},
    {"db", "DB"},
    {"dbs", "DB %s"},
    {"dw", "DW"},
    {"dws", "DW %s"},
    {"immed", ""},
    {"constbyte", "0x%02X"},
    {"constword", "0x%04X"},
    {"immedword", "0x%04X"},
    {"immedbyte", "0x%02X"},
    {"hashedstr", "%s"},
    {"lsbimmeds", "%s & 0xFF"},
    {"msbimmeds", "%s >> 8"},
    {"bankimmeds", "!%s"},
    {"hashedbankimmeds", "!%s"},
    {"module", "; MODULE %s"},
    {"optsdcc", "; optsdcc"},
    {NULL, NULL}};

static const ASM_MAPPING _z80asm_mapping[] = {
    {"global", "XDEF %s"},
    {"extern", "XREF %s"},
    {"slabeldef", "\n.%s"},
    {"labeldef", "\n.%s"},
    {"tlabeldef", "\n.l%N%05d"},
    {"tlabel", "l%N%05d"},
    {"fileprelude",
     "; Generated using the z80asm/z88 tokens.\n"
     "\tXREF __muluchar_rrx_s\n"
     "\tXREF __mulschar_rrx_s\n"
     "\tXREF __mulint_rrx_s\n"
     "\tXREF __mullong_rrx_s\n"
     "\tXREF __divuchar_rrx_s\n"
     "\tXREF __divschar_rrx_s\n"
     "\tXREF __divsint_rrx_s\n"
     "\tXREF __divuint_rrx_s\n"
     "\tXREF __divulong_rrx_s\n"
     "\tXREF __divslong_rrx_s\n"
     "\tXREF __rrulong_rrx_s\n"
     "\tXREF __rrslong_rrx_s\n"
     "\tXREF __rlulong_rrx_s\n"
     "\tXREF __rlslong_rrx_s\n"},
    {"functionheader",
     "; ---------------------------------\n"
     "; Function %s\n"
     "; ---------------------------------"},
    {"functionlabeldef", ".%s"},
    {"globalfunctionlabeldef", ".%s"},
    {"zero", "$00"},
    {"one", "$01"},
    {"ascii", "DEFM \"%s\""},
    {"ds", "DEFS %d"},
    {"db", "DEFB"},
    {"dbs", "DEFB %s"},
    {"dw", "DEFW"},
    {"dws", "DEFB %s"},
    {"immed", ""},
    {"constbyte", "$%02X"},
    {"constword", "$%04X"},
    {"immedword", "$%04X"},
    {"immedbyte", "$%02X"},
    {"hashedstr", "%s"},
    {"lsbimmeds", "%s ~ $FF"},
    {"msbimmeds", "%s / 256"},

    {"bankimmeds", "BANK(%s)"},
    {"hashedbankimmeds", "BANK(%s)"},
    {"module", "MODULE %s"},
    {"area", "; Area  %s"},
    {"areadata", "; Aread BSS"},
    {"areacode", "; Area CODE"},
    {"areahome", "; Area HOME"},
    {"optsdcc", "; optsdcc"},
    {NULL, NULL}};

static const ASM_MAPPING _z80asm_z80_mapping[] = {
    {"*ixx", "(ix%+d)"},
    {"*iyx", "(iy%+d)"},
    {"*hl", "(hl)"},
    {"jphl", "jp (hl)"},
    {"di", "di"},
    {"ei", "ei"},
    {"ldahli",
     "ld a, (hl)\n"
     "inc\thl"},
    {"ldahld",
     "ld a, (hl)\n"
     "dec\thl"},
    {"ldahli",
     "ld (hl), a\n"
     "inc\thl"},
    {"ldahld",
     "ld (hl), a\n"
     "dec\thl"},
    {"ldahlsp",
     "ld hl, %d\n"
     "add\thl, sp"},
    {"ldaspsp",
     "ld iy, %d\n"
     "add\tiy, sp\n"
     "ld\tsp, iy"},
    {"mems", "(%s)"},
    {"enter",
     "push\tix\n"
     "ld\tix,0\n"
     "add\tix,sp"},
    {"enters",
     "call\t___sdcc_enter_ix\n"},
    {"pusha",
     "push af\n"
     "push\tbc\n"
     "push\tde\n"
     "push\thl\n"
     "push\tiy"},
    {"popa",
     "pop\tiy\n"
     "pop\thl\n"
     "pop\tde\n"
     "pop\tbc\n"
     "pop\taf"},
    {"adjustsp", "lda sp, (sp%+d)"},
    {"optsdcc", "; optsdcc"},
    {NULL, NULL}};

static const ASM_MAPPING _gas_z80_mapping[] = {
    {"immed", "#"},
    {"zero", "#0x00"},
    {"one", "#0x01"},
    {"area", ".area\t%s"},
    {"areacode", ".area\t%s"},
    {"areadata", ".area\t%s"},
    {"areahome", ".area\t%s"},
    {"constbyte", "0x%02x"},
    {"constword", "0x%04x"},
    {"immedword", "#0x%04x"},
    {"immedbyte", "#0x%02x"},
    {"hashedstr", "#%s"},
    {"bankimmeds", "%s >> 16"},
    {"*ixx", "%d (ix)"},
    {"*iyx", "%d (iy)"},
    {"*hl", "(hl)"},
    {"di", "di"},
    {"ei", "ei"},
    {"ldahli",
     "ld\ta,(hl)\n"
     "inc\thl"},
    {"ldahld",
     "ld\ta,(hl)\n"
     "dec\thl"},
    {"lldahli",
     "ld\t(hl),a\n"
     "inc\thl"},
    {"lldahld",
     "ld\t(hl),a\n"
     "dec\thl"},
    {"ldahlsp",
     "ld\thl, #%d\n"
     "add\thl, sp"},
    {"ldaspsp",
     "ld\tiy,#%d\n"
     "add\tiy,sp\n"
     "ld\tsp,iy"},
    {"mems", "(%s)"},
    {"enter",
     "push\tix\n"
     "ld\tix,#0\n"
     "add\tix,sp"},
    {"enters", "call\t___sdcc_enter_ix\n"},
    {"pusha",
     "push\taf\n"
     "push\tbc\n"
     "push\tde\n"
     "push\thl\n"
     "push\tiy"},
    {"popa",
     "pop\tiy\n"
     "pop\thl\n"
     "pop\tde\n"
     "pop\tbc\n"
     "pop\taf"},
    {"adjustsp", "lda\tsp,-%d (sp)"},
    {"optsdcc", "; optsdcc"},
    {NULL, NULL}};

static const ASM_MAPPINGS _isas = {
    NULL,
    _isas_mapping};

const ASM_MAPPINGS _asxxxx_z80 = {
    &asm_asxxxx_mapping,
    _asxxxx_z80_mapping};

static const ASM_MAPPINGS _z80asm = {
    NULL,
    _z80asm_mapping};

const ASM_MAPPINGS _z80asm_z80 = {
    &_z80asm,
    _z80asm_z80_mapping};

const ASM_MAPPINGS _gas_z80 = {
    &asm_gas_mapping,
    _gas_z80_mapping};