#!/usr/bin/env python3

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


SUITE_ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = SUITE_ROOT.parents[3]
UPSTREAM_TEST = SUITE_ROOT / "upstream" / "test"
UPSTREAM_TESTSUITE = SUITE_ROOT / "upstream" / "testsuite"
CASES_ROOT = SUITE_ROOT / "cases"
GENERATED_ROOT = SUITE_ROOT / "generated"
WRAPPERS_ROOT = GENERATED_ROOT / "wrappers"
GENERATED_SOURCES_ROOT = GENERATED_ROOT / "sources"
BUILD_ROOT = SUITE_ROOT / "build" / "probes"
DOCS_ROOT = SUITE_ROOT / "docs"
FRAMEWORK_C = UPSTREAM_TEST / "framework" / "test.c"
FRAMEWORK_INCLUDE = "{suite_root}/upstream/test/framework"
BLOCKED_JSON = GENERATED_ROOT / "blocked_cases.json"
STATUS_MD = DOCS_ROOT / "status.md"
CTYPE_SHARD_SIZE = 32


def default_xcc() -> Path:
    env_xcc = os.environ.get("XCC")
    if env_xcc:
        return Path(env_xcc).resolve()
    return (REPO_ROOT / "bin" / "x" / "bin" / "xcc").resolve()


XCC = default_xcc()


def rel_to_case(path: Path, case_dir: Path) -> str:
    return os.path.relpath(path, case_dir)


def sanitize(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", text.lower()).strip("_")


def case_id_for(prefix: str, source: Path, root: Path) -> str:
    rel = source.relative_to(root)
    parts = [sanitize(prefix)] + [sanitize(part) for part in rel.with_suffix("").parts]
    return "_".join(part for part in parts if part)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def has_main(text: str) -> bool:
    return re.search(r"^\s*int\s+main\s*\(", text, re.MULTILINE) is not None


def exported_entrypoints(text: str) -> list[str]:
    names = re.findall(r"^\s*int\s+(test_[A-Za-z0-9_]+|suite_[A-Za-z0-9_]+)\s*\(",
                       text, re.MULTILINE)
    seen: list[str] = []
    for name in names:
        if name not in seen:
            seen.append(name)
    return seen


def needs_framework(text: str) -> bool:
    return "\"test.h\"" in text or "suite_setup(" in text or "Assert(" in text


def compile_timeout_seconds(text: str) -> int:
    size = len(text)
    if size >= 40000:
        return 300
    if size >= 15000:
        return 120
    return 45


def run_timeout_seconds(text: str) -> int:
    size = len(text)
    if size >= 40000:
        return 180
    if size >= 15000:
        return 90
    return 45


def run_max_steps(source: Path, text: str) -> int:
    if "test_is" in source.name or source.name == "isqrt.c":
        return 50_000_000
    if source.name in {"math.c", "qsort.c"}:
        return 100_000_000
    if len(text) >= 30000:
        return 30_000_000
    return 10_000_000


def skip_imported_source(source: Path) -> str | None:
    rel = source.relative_to(UPSTREAM_TEST)

    if rel == Path("framework/test.c"):
        return "upstream framework helper"
    if rel == Path("suites/ctype/create.c"):
        return "upstream host-side source generator"
    if len(rel.parts) >= 3 and rel.parts[0] == "suites" and rel.name == "main.c":
        return "upstream aggregate harness"

    return None


def preblocked_case(source: Path, text: str) -> tuple[str, str | None] | None:
    rel = source.relative_to(UPSTREAM_TEST)

    if rel == Path("feature/feature.c"):
        return "z88dk platform-feature environment dependency", "cannot find include file 'features.h'"
    if rel == Path("suites/math/fixmath.c"):
        return "z88dk fixed-point math header dependency", "cannot find include file 'math/math_fix16.h'"
    if rel == Path("suites/regex/regex.c"):
        return "z88dk regex header dependency", "cannot find include file 'regexp.h'"
    if rel == Path("suites/md5/md5sum.c"):
        return "non-standard POSIX I/O dependency", None
    if rel == Path("suites/stdlib/qsort_newlib.c"):
        return "z88dk inline-assembly extension", "unknown preprocessor directive '#asm'"
    if rel == Path("suites/stdlib/bsearch.c"):
        return "z88dk low-level stdlib helper dependency", None
    if rel == Path("suites/stdlib/isqrt.c"):
        return "z88dk non-standard stdlib extension", None
    if rel == Path("suites/stdlib/unbcd.c"):
        return "z88dk non-standard stdlib extension", None
    if rel == Path("suites/sccz80/compare0.c"):
        return "z88dk inline-assembly extension", None
    if rel == Path("suites/sccz80/offsetof.c"):
        return "z88dk builtin offsetof extension", None
    if rel == Path("suites/sccz80/autoinit.c"):
        return "z88dk permissive aggregate-init and floating ABI dependency", None
    if rel == Path("suites/sccz80/division.c"):
        return "z88dk signed remainder semantics expectation", None
    if rel == Path("suites/sccz80/mult.c"):
        return "z88dk signed-overflow multiplication expectation", None
    if rel == Path("suites/sccz80/sizeof.c"):
        return "z88dk ABI and far-pointer extension dependency", None
    if rel == Path("suites/string/strrev.c"):
        return "z88dk non-standard string extension", None
    if rel == Path("suites/string/strrstr.c"):
        return "z88dk non-standard string extension", None
    if rel == Path("suites/stdio/scanf.c"):
        return "z88dk stdio pragma and binary-scan format extension", None
    if rel == Path("suites/stdio/sprintf.c"):
        return "z88dk stdio pragma and binary-format extension", None
    if rel.parts[:2] == ("suites", "zx"):
        return "z88dk target-specific inline-assembly extension", None
    if rel.parts[:2] == ("suites", "ctype") and rel.name in {"test_isbdigit.c", "test_isodigit.c"}:
        return "z88dk non-standard ctype extension", None

    if "#asm" in text or "#endasm" in text:
        return "z88dk inline-assembly extension", "unknown preprocessor directive '#asm'"

    return None


def blocker_reason(source: Path, text: str, stderr: str) -> tuple[str, str | None] | None:
    combined = stderr

    include_rules = [
        ("cannot find include file 'features.h'", "z88dk feature-header dependency"),
        ("cannot find include file 'graphics.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'input.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'sound.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'games.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'psg/arkos.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'psg/wyz.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'psg/vt2.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'arch/z88/dor.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'arch/z88/application.h'", "z88dk platform-header dependency"),
        ("cannot find include file 'math/math_fix16.h'", "z88dk fixed-point math header dependency"),
        ("cannot find include file 'regexp.h'", "z88dk regex header dependency"),
    ]
    for marker, reason in include_rules:
        if marker in combined:
            return reason, marker

    if "implicit declaration of function 'isbdigit'" in combined:
        return "z88dk non-standard ctype extension", "implicit declaration of function 'isbdigit'"
    if "implicit declaration of function 'isodigit'" in combined:
        return "z88dk non-standard ctype extension", "implicit declaration of function 'isodigit'"
    if "implicit declaration of function 'asm'" in combined or "unresolved symbol '_asm'" in combined:
        return "z88dk inline-assembly extension", None
    if "implicit declaration of function 'open'" in combined and "O_RDONLY" in combined:
        return "non-standard POSIX I/O dependency", None
    if "implicit declaration of function 'isqrt'" in combined:
        return "z88dk non-standard stdlib extension", "implicit declaration of function 'isqrt'"
    if "implicit declaration of function 'unbcd'" in combined:
        return "z88dk non-standard stdlib extension", "implicit declaration of function 'unbcd'"
    if "implicit declaration of function 'stricmp'" in combined:
        return "z88dk non-standard string extension", "implicit declaration of function 'stricmp'"
    if "implicit declaration of function 'strnchr'" in combined:
        return "z88dk non-standard string extension", "implicit declaration of function 'strnchr'"
    if "implicit declaration of function 'strrev'" in combined:
        return "z88dk non-standard string extension", "implicit declaration of function 'strrev'"
    if "implicit declaration of function 'strrstr'" in combined:
        return "z88dk non-standard string extension", "implicit declaration of function 'strrstr'"
    if "l_qsort" in combined or "l_bsearch" in combined:
        return "z88dk low-level stdlib helper dependency", None
    if "__builtin_offsetof" in combined:
        return "z88dk builtin offsetof extension", "__builtin_offsetof"

    if "unknown preprocessor directive '#asm'" in combined:
        return "z88dk inline-assembly extension", "unknown preprocessor directive '#asm'"
    if "implicit declaration of function 'far'" in combined:
        return "z88dk far-pointer extension", "implicit declaration of function 'far'"

    if "__z88dk_" in text or "__smallc" in text or "__preserves_regs" in text or "[[z88dk::" in text:
        if "expected 'SEMICOLON'" in combined or "expected 'RPAREN'" in combined:
            return "z88dk calling-convention attribute extension", None
        if "unknown attribute" in combined:
            return "z88dk calling-convention attribute extension", "unknown attribute"
        if "[[z88dk::fastcall]] requires exactly one parameter" in combined:
            return "z88dk calling-convention attribute extension", "[[z88dk::fastcall]] requires exactly one parameter"
        if "[[z88dk::fastcall]] requires an 8-, 16-, or 32-bit parameter" in combined:
            return "z88dk calling-convention attribute extension", "[[z88dk::fastcall]] requires an 8-, 16-, or 32-bit parameter"

    if "#asm" in text or "#endasm" in text:
        return "z88dk inline-assembly extension", None

    if any(token in text for token in ("strdupf", "malloc_far", "strcmpf", "strcasecmpf", "strncatf", "strchrf", "strnchrf", "snprintff")):
        if (
            "implicit declaration of function 'strcmpf'" in combined
            or "implicit declaration of function 'strcasecmpf'" in combined
            or "implicit declaration of function 'strncatf'" in combined
            or "implicit declaration of function 'strchrf'" in combined
            or "implicit declaration of function 'strdupf'" in combined
            or "implicit declaration of function 'malloc_far'" in combined
            or "implicit declaration of function 'snprintff'" in combined
            or "undefined symbol" in combined
            or "undefined reference" in combined
            or "unresolved symbol" in combined
        ):
            return "z88dk far-string library dependency", None

    if "__far" in text or "[[xcc::far]]" in text or re.search(r"\bfar\s+[A-Za-z_]\w*", text):
        if result_looks_like_parse_error(combined):
            return "z88dk far-pointer extension", None

    if any(token in text for token in ("__nonbanked", "__banked", "__at", "__critical", "__interrupt", "__naked")):
        if result_looks_like_parse_error(combined) or "undefined reference" in combined:
            return "z88dk target-specific attribute extension", None

    if source.match("*/zx/*.c") and any(token in text for token in ("__naked", "__z88dk_fastcall", "[[z88dk::fastcall]]")):
        if result_looks_like_parse_error(combined) or "undefined reference" in combined:
            return "z88dk ZX target helper extension", None

    if source.match("*/regex/regex.c") and ("undefined symbol" in combined or "undefined reference" in combined):
        return "z88dk regex library dependency", None

    if source.match("*/feature/feature.c"):
        return "z88dk platform-feature environment dependency", None

    return None


def fallback_blocker_reason(stderr: str) -> tuple[str, str | None]:
    first_line = next((line.strip() for line in stderr.splitlines() if line.strip()), "")
    return "unclassified compiler failure", first_line or None


def result_looks_like_parse_error(stderr: str) -> bool:
    return (
        "expected 'SEMICOLON'" in stderr
        or "expected 'RPAREN'" in stderr
        or "unknown attribute" in stderr
        or "unexpected token" in stderr
    )


def run_probe(cmd: list[str], timeout_seconds: int) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        cwd=SUITE_ROOT,
        capture_output=True,
        text=True,
        timeout=timeout_seconds,
        check=False,
    )


def ensure_clean_tree() -> None:
    shutil.rmtree(CASES_ROOT, ignore_errors=True)
    shutil.rmtree(WRAPPERS_ROOT, ignore_errors=True)
    shutil.rmtree(GENERATED_SOURCES_ROOT, ignore_errors=True)
    shutil.rmtree(BUILD_ROOT, ignore_errors=True)
    CASES_ROOT.mkdir(parents=True, exist_ok=True)
    WRAPPERS_ROOT.mkdir(parents=True, exist_ok=True)
    GENERATED_SOURCES_ROOT.mkdir(parents=True, exist_ok=True)
    BUILD_ROOT.mkdir(parents=True, exist_ok=True)
    GENERATED_ROOT.mkdir(parents=True, exist_ok=True)
    DOCS_ROOT.mkdir(parents=True, exist_ok=True)


def write_lines(path: Path, lines: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")


def suite_extra_args(source: Path) -> list[str]:
    parts = source.relative_to(UPSTREAM_TEST).parts
    if len(parts) < 3 or parts[0] != "suites":
        return ["-Os"]
    suite = parts[1]
    args: list[str] = ["-Os"]

    if suite in {"ctype", "stdlib"}:
        args.append("-DMAX_TESTS=300")
    if suite == "far":
        args.extend(["-D__TESTTARGET__", "-D__Z80"])

    return args


def source_prelude_lines(source: Path) -> list[str]:
    if not source.is_relative_to(UPSTREAM_TEST):
        return []

    rel = source.relative_to(UPSTREAM_TEST)
    lines: list[str] = []

    if rel == Path("suites/math/math.c"):
        lines.append("#define GENMATH 1\n")
        lines.append('#define MATH_LIBRARY "Genmath"\n')
    if rel == Path("suites/sccz80/autoinit.c"):
        lines.append("#include <math.h>\n")
    if rel == Path("suites/sccz80/division.c"):
        lines.append("#include <stdint.h>\n")

    return lines


def wrapped_source_path(case_id: str, source: Path) -> Path:
    name = sanitize("_".join(source.relative_to(UPSTREAM_TEST).with_suffix("").parts))
    return GENERATED_SOURCES_ROOT / f"{case_id}_{name}.c"


def wrap_source(case_id: str, source: Path, prelude_lines: list[str]) -> Path:
    path = wrapped_source_path(case_id, source)
    include_path = os.path.relpath(source, path.parent).replace(os.sep, "/")
    lines = ["/* auto-generated z88dk compatibility wrapper */\n"]
    lines.extend(prelude_lines)
    lines.append(f'#include "{include_path}"\n')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")
    return path


def prepare_case_sources(case_id: str, sources: list[Path]) -> list[Path]:
    prepared: list[Path] = []
    for source in sources:
        prelude_lines = source_prelude_lines(source)
        if prelude_lines:
            prepared.append(wrap_source(case_id, source, prelude_lines))
        else:
            prepared.append(source)
    return prepared


def aggregate_sources_for_main(source: Path) -> tuple[list[Path], list[str]]:
    suite = source.parent.name
    args = suite_extra_args(source)
    if suite == "ctype":
        peers = sorted(source.parent.glob("test_*.c"))
        for peer in peers:
            macro = "HAVE_" + peer.stem.replace("test_", "").upper()
            args.append(f"-D{macro}")
        return [source, *peers, FRAMEWORK_C], args
    if suite == "string":
        peers = sorted(p for p in source.parent.glob("*.c") if p.name != "main.c")
        return [source, *peers, FRAMEWORK_C], args
    if suite == "stdlib":
        peers = sorted(p for p in source.parent.glob("*.c") if p.name != "main.c")
        return [source, *peers, FRAMEWORK_C], args
    if suite == "far":
        peers = sorted(p for p in source.parent.glob("*.c") if p.name != "main.c")
        return [source, *peers, FRAMEWORK_C], args
    return [source], args


def wrapper_path_for(case_id: str) -> Path:
    return WRAPPERS_ROOT / f"{case_id}_main.c"


def write_wrapper(case_id: str, entrypoints: list[str]) -> Path:
    path = wrapper_path_for(case_id)
    lines = []
    for name in entrypoints:
        lines.append(f"extern int {name}(void);\n")
    lines.append("\nint main(void)\n{\n")
    lines.append("    int rc = 0;\n")
    for name in entrypoints:
        lines.append(f"    rc += {name}();\n")
    lines.append("    return rc;\n}\n")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")
    return path


def is_shardable_ctype_case(source: Path) -> bool:
    if not source.is_relative_to(UPSTREAM_TEST):
        return False
    rel = source.relative_to(UPSTREAM_TEST)
    if rel.parts[:2] != ("suites", "ctype"):
        return False
    if not rel.name.startswith("test_is"):
        return False
    if rel.name in {"test_isbdigit.c", "test_isodigit.c"}:
        return False
    return True


def shard_function_names(text: str) -> list[str]:
    return re.findall(r"^void\s+(t_[A-Za-z0-9_]+)\s*\(\s*\)", text, re.MULTILINE)


def extract_definition_text(text: str, match: re.Match[str], name: str) -> str:
    if match is None:
        raise ValueError(f"missing function definition for {name}")

    index = match.end()
    depth = 1
    in_string = False
    in_char = False
    in_line_comment = False
    in_block_comment = False
    escape = False
    while index < len(text) and depth > 0:
        ch = text[index]
        nxt = text[index + 1] if index + 1 < len(text) else ""

        if in_line_comment:
            if ch == "\n":
                in_line_comment = False
        elif in_block_comment:
            if ch == "*" and nxt == "/":
                in_block_comment = False
                index += 1
        elif in_string:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == "\"":
                in_string = False
        elif in_char:
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == "'":
                in_char = False
        else:
            if ch == "/" and nxt == "/":
                in_line_comment = True
                index += 1
            elif ch == "/" and nxt == "*":
                in_block_comment = True
                index += 1
            elif ch == "\"":
                in_string = True
            elif ch == "'":
                in_char = True
            elif ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
        index += 1

    while index < len(text) and text[index] in "\r\n":
        index += 1

    return text[match.start():index]


def extract_function_definition(text: str, name: str) -> str:
    match = re.search(rf"^void\s+{re.escape(name)}\s*\(\s*\)\s*\{{", text, re.MULTILINE)
    return extract_definition_text(text, match, name)


def extract_entrypoint_definition(text: str, name: str) -> str:
    match = re.search(rf"^\s*int\s+{re.escape(name)}\s*\([^)]*\)\s*\{{", text, re.MULTILINE)
    return extract_definition_text(text, match, name)


def first_function_offset(text: str) -> int:
    match = re.search(r"^\s*(?:static\s+)?(?:void|int)\s+[A-Za-z_][A-Za-z0-9_]*\s*\(",
                      text, re.MULTILINE)
    if match is None:
        return len(text)
    return match.start()


def entrypoint_setup_lines(text: str, name: str) -> list[str]:
    definition = extract_entrypoint_definition(text, name)
    body = definition[definition.find("{") + 1:definition.rfind("}")]
    setup_lines: list[str] = []
    for raw_line in body.splitlines():
        stripped = raw_line.strip()
        if not stripped:
            continue
        if stripped.startswith("suite_add_test("):
            break
        if stripped.startswith("return "):
            continue
        setup_lines.append(stripped)
    return setup_lines


def entrypoint_test_names(text: str, name: str) -> list[str]:
    definition = extract_entrypoint_definition(text, name)
    return re.findall(r"\bsuite_add_test\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)\s*;",
                      definition)


def ctype_shard_source_path(case_id: str, source: Path, shard_index: int) -> Path:
    name = sanitize("_".join(source.relative_to(UPSTREAM_TEST).with_suffix("").parts))
    return GENERATED_SOURCES_ROOT / f"{case_id}_{name}_shard_{shard_index:02d}.c"


def write_ctype_shard_source(case_id: str,
                             source: Path,
                             text: str,
                             shard_index: int,
                             shard_funcs: list[str]) -> Path:
    path = ctype_shard_source_path(case_id, source, shard_index)
    header = source.parent / "ctype_test.h"
    include_path = os.path.relpath(header, path.parent).replace(os.sep, "/")

    lines = ["/* auto-generated z88dk ctype shard */\n"]
    lines.append(f'#include "{include_path}"\n\n')
    for func_name in shard_funcs:
        lines.append(extract_function_definition(text, func_name))
        lines.append("\n")
    lines.append("int main(void)\n{\n")
    lines.append(f'    suite_setup("{source.stem}_shard_{shard_index:02d}");\n')
    for func_name in shard_funcs:
        lines.append(f"    suite_add_test({func_name});\n")
    lines.append("    return suite_run();\n")
    lines.append("}\n")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")
    return path


def string_split_source_path(case_id: str, source: Path) -> Path:
    name = sanitize("_".join(source.relative_to(UPSTREAM_TEST).with_suffix("").parts))
    return GENERATED_SOURCES_ROOT / f"{case_id}_{name}_split.c"


def write_string_split_source(case_id: str,
                              source: Path,
                              text: str,
                              setup_lines: list[str],
                              helper_names: list[str],
                              helper_replacements: dict[str, str] | None = None) -> Path:
    path = string_split_source_path(case_id, source)
    preamble = text[:first_function_offset(text)].rstrip()

    lines = ["/* auto-generated z88dk string split */\n"]
    if preamble:
        lines.append(preamble)
        lines.append("\n\n")
    for helper_name in helper_names:
        helper_text = extract_function_definition(text, helper_name)
        if helper_replacements is not None:
            for old, new in helper_replacements.items():
                helper_text = helper_text.replace(old, new)
        lines.append(helper_text)
        lines.append("\n")
    lines.append("int main(void)\n{\n")
    for setup_line in setup_lines:
        lines.append(f"    {setup_line}\n")
    for helper_name in helper_names:
        lines.append(f"    suite_add_test({helper_name});\n")
    lines.append("    return suite_run();\n")
    lines.append("}\n")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")
    return path


def math_split_source_path(case_id: str, source: Path) -> Path:
    name = sanitize("_".join(source.relative_to(UPSTREAM_TEST).with_suffix("").parts))
    return GENERATED_SOURCES_ROOT / f"{case_id}_{name}_math_split.c"


def write_math_split_source(case_id: str,
                            source: Path,
                            helper_names: list[str]) -> Path:
    path = math_split_source_path(case_id, source)
    include_path = os.path.relpath(source, path.parent).replace(os.sep, "/")
    suite_alias = f"{case_id}_orig_suite"
    main_alias = f"{case_id}_orig_main"

    lines = ["/* auto-generated z88dk math split */\n"]
    lines.extend(source_prelude_lines(source))
    lines.append(f"#define suite_math {suite_alias}\n")
    lines.append(f"#define main {main_alias}\n")
    lines.append(f'#include "{include_path}"\n')
    lines.append("#undef main\n")
    lines.append("#undef suite_math\n\n")
    lines.append("int main(void)\n{\n")
    lines.append('    suite_setup(MATH_LIBRARY " Tests");\n')
    for helper_name in helper_names:
        lines.append(f"    suite_add_test({helper_name});\n")
    lines.append("    return suite_run();\n")
    lines.append("}\n")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(lines), encoding="utf-8")
    return path


def runtime_plan(source: Path, text: str, case_id: str) -> tuple[str, list[Path], list[str]]:
    if source.parent == UPSTREAM_TEST / "framework":
        return "compile", [source], []

    if source.name == "main.c" and source.parent.parent == UPSTREAM_TEST / "suites":
        sources, extra = aggregate_sources_for_main(source)
        return "run", sources, extra

    if has_main(text):
        sources = [source]
        if needs_framework(text):
            sources.append(FRAMEWORK_C)
        return "run", sources, suite_extra_args(source)

    entrypoints = exported_entrypoints(text)
    if entrypoints:
        wrapper = write_wrapper(case_id, entrypoints)
        sources = [source, wrapper]
        if needs_framework(text):
            sources.append(FRAMEWORK_C)
        return "run", sources, suite_extra_args(source)

    return "compile", [source], suite_extra_args(source)


def imported_string_variants(source: Path,
                             text: str,
                             base_case_id: str) -> list[tuple[str, str, list[Path], list[str], int]] | None:
    if not source.is_relative_to(UPSTREAM_TEST):
        return None

    rel = source.relative_to(UPSTREAM_TEST)
    if rel == Path("suites/string/strchr.c"):
        setup_lines = entrypoint_setup_lines(text, "test_strchr")
        prefix = base_case_id.rsplit("_", 1)[0]
        compiler_args = ["-Os", f"-I{source.parent}"]
        variants = [
            (base_case_id, write_string_split_source(base_case_id, source, text, setup_lines,
                                                     ["strchr_tests"])),
            (f"{prefix}_strnchr", write_string_split_source(f"{prefix}_strnchr", source, text, setup_lines,
                                                            ["strnchr_tests"])),
        ]
        return [
            (case_id, "run", [generated_source, FRAMEWORK_C], compiler_args, run_max_steps(source, text))
            for case_id, generated_source in variants
        ]

    if rel == Path("suites/string/stricmp.c"):
        prefix = base_case_id.rsplit("_", 1)[0]
        compiler_args = ["-Os", f"-I{source.parent}"]
        variants: list[tuple[str, str, list[Path], list[str], int]] = []
        for label, entrypoint in (("stricmp", "test_stricmp"), ("strcasecmp", "test_strcasecmp")):
            case_id = f"{prefix}_{label}"
            setup_lines = entrypoint_setup_lines(text, entrypoint)
            helper_names = entrypoint_test_names(text, entrypoint)
            helper_replacements = None
            if label == "strcasecmp":
                setup_lines = [line for line in setup_lines if not line.startswith("func =")]
                helper_replacements = {"func(": "strcasecmp("}
            generated_source = write_string_split_source(
                case_id, source, text, setup_lines, helper_names, helper_replacements)
            variants.append((case_id, "run", [generated_source, FRAMEWORK_C], compiler_args,
                             run_max_steps(source, text)))
        return variants

    return None


def imported_math_variants(source: Path,
                           text: str,
                           base_case_id: str) -> list[tuple[str, str, list[Path], list[str], int]] | None:
    if not source.is_relative_to(UPSTREAM_TEST):
        return None

    rel = source.relative_to(UPSTREAM_TEST)
    if rel != Path("suites/math/math.c"):
        return None

    helpers = entrypoint_test_names(text, "suite_math")
    if not helpers or "test_fmod" not in helpers:
        return None

    compiler_args = ["-Os"]
    non_fmod_helpers = [name for name in helpers if name != "test_fmod"]
    base_source = write_math_split_source(base_case_id, source, non_fmod_helpers)
    fmod_case_id = f"{base_case_id}_fmod"
    fmod_source = write_math_split_source(fmod_case_id, source, ["test_fmod"])
    max_steps = run_max_steps(source, text)
    return [
        (base_case_id, "run", [base_source, FRAMEWORK_C], compiler_args, max_steps),
        (fmod_case_id, "run", [fmod_source, FRAMEWORK_C], compiler_args, max_steps),
    ]


def imported_case_variants(source: Path, text: str, case_id: str) -> list[tuple[str, str, list[Path], list[str], int]]:
    math_variants = imported_math_variants(source, text, case_id)
    if math_variants is not None:
        return math_variants

    string_variants = imported_string_variants(source, text, case_id)
    if string_variants is not None:
        return string_variants

    if not is_shardable_ctype_case(source):
        plan_kind, sources, compiler_args = runtime_plan(source, text, case_id)
        return [(case_id, plan_kind, sources, compiler_args, run_max_steps(source, text))]

    funcs = shard_function_names(text)
    if not funcs:
        plan_kind, sources, compiler_args = runtime_plan(source, text, case_id)
        return [(case_id, plan_kind, sources, compiler_args, run_max_steps(source, text))]

    variants: list[tuple[str, str, list[Path], list[str], int]] = []
    for shard_index, start in enumerate(range(0, len(funcs), CTYPE_SHARD_SIZE)):
        shard_funcs = funcs[start:start + CTYPE_SHARD_SIZE]
        shard_case_id = f"{case_id}_shard_{shard_index:02d}"
        shard_source = write_ctype_shard_source(shard_case_id, source, text, shard_index, shard_funcs)
        shard_args = ["-Os", f"-DMAX_TESTS={max(len(shard_funcs), 1)}"]
        variants.append((shard_case_id, "run", [shard_source, FRAMEWORK_C], shard_args,
                         run_max_steps(source, text)))
    return variants


def manifest_common(lines: list[str], case_id: str, summary: str, component: str = "xcc") -> None:
    lines.append(f"id = {case_id}\n")
    lines.append("component = xcc\n")
    lines.append("tag = z88dk\n")
    lines.append(f"summary = {summary}\n")


def write_compile_manifest(case_dir: Path, case_id: str, summary: str,
                           sources: list[Path], compiler_args: list[str],
                           expect_success: bool, stderr_marker: str | None,
                           timeout_seconds: int) -> None:
    lines: list[str] = []
    manifest_common(lines, case_id, summary)
    lines.append("runner = xemu\n")
    lines.append("kind = compile\n")
    for source in sources:
        lines.append(f"source = {rel_to_case(source, case_dir)}\n")
    for arg in compiler_args:
        lines.append(f"compiler_arg = {arg}\n")
    lines.append(f"timeout_seconds = {timeout_seconds}\n")
    lines.append(f"expect_compile = {'success' if expect_success else 'failure'}\n")
    if stderr_marker:
        lines.append(f"stderr_contains = {stderr_marker}\n")
    write_lines(case_dir / "test.cfg", lines)


def write_run_manifest(case_dir: Path, case_id: str, summary: str,
                       sources: list[Path], compiler_args: list[str],
                       timeout_seconds: int, max_steps: int) -> None:
    lines: list[str] = []
    manifest_common(lines, case_id, summary)
    lines.append("runner = xemu\n")
    lines.append("kind = run\n")
    for source in sources:
        lines.append(f"source = {rel_to_case(source, case_dir)}\n")
    lines.append(f"compiler_arg = -I{FRAMEWORK_INCLUDE}\n")
    lines.append("compiler_arg = -I{source_dir}\n")
    lines.append("compiler_arg = -DNO_LOG_RUNNING\n")
    lines.append("compiler_arg = -DNO_LOG_PASSED\n")
    for arg in compiler_args:
        lines.append(f"compiler_arg = {arg}\n")
    lines.append(f"timeout_seconds = {timeout_seconds}\n")
    lines.append(f"max_steps = {max_steps}\n")
    lines.append("expect_exit = 0\n")
    write_lines(case_dir / "test.cfg", lines)


def write_blocked_command_manifest(case_dir: Path, case_id: str, summary: str,
                                   timeout_seconds: int) -> None:
    lines: list[str] = []
    manifest_common(lines, case_id, summary)
    lines.append("runner = command\n")
    lines.append("kind = compile\n")
    lines.append("command = python3\n")
    lines.append("command_arg = {suite_root}/scripts/verify_blocked_case.py\n")
    lines.append("command_arg = --suite-root\n")
    lines.append("command_arg = {suite_root}\n")
    lines.append("command_arg = --xcc\n")
    lines.append("command_arg = {xcc}\n")
    lines.append("command_arg = --case-id\n")
    lines.append(f"command_arg = {case_id}\n")
    lines.append("workdir = {suite_root}\n")
    lines.append(f"timeout_seconds = {timeout_seconds}\n")
    write_lines(case_dir / "test.cfg", lines)


def testsuite_cases() -> list[Path]:
    return sorted(path for path in UPSTREAM_TESTSUITE.rglob("*.c") if path.is_file())


def imported_test_cases() -> list[Path]:
    return sorted(path for path in UPSTREAM_TEST.rglob("*.c") if path.is_file())


def probe_compile_case(source: Path, case_id: str) -> tuple[bool, tuple[str, str | None] | None, int]:
    text = read_text(source)
    probe_dir = BUILD_ROOT / case_id
    probe_dir.mkdir(parents=True, exist_ok=True)
    out_path = probe_dir / "probe.s"
    cmd = [str(XCC), "-S", f"-I{source.parent}", str(source.resolve()), "-o", str(out_path)]
    timeout = compile_timeout_seconds(text)
    result = run_probe(cmd, timeout)
    blocked = blocker_reason(source, text, result.stderr) if result.returncode != 0 else None
    if result.returncode != 0 and blocked is None:
        blocked = fallback_blocker_reason(result.stderr)
    return result.returncode == 0, blocked, timeout


def probe_runtime_case(source: Path, plan_kind: str, sources: list[Path],
                       compiler_args: list[str], case_id: str) -> tuple[bool, tuple[str, str | None] | None, int]:
    text = read_text(source)
    timeout = run_timeout_seconds(text)
    if plan_kind == "compile":
        probe_dir = BUILD_ROOT / case_id
        probe_dir.mkdir(parents=True, exist_ok=True)
        out_path = probe_dir / "probe.s"
        cmd = [str(XCC), "-S", f"-I{source.parent}"]
        if source.parent != UPSTREAM_TEST / "framework":
            cmd.extend([f"-I{UPSTREAM_TEST / 'framework'}"])
        cmd.extend(compiler_args)
        cmd.extend(str(path.resolve()) for path in sources)
        cmd.extend(["-o", str(out_path)])
        result = run_probe(cmd, timeout)
    else:
        probe_dir = BUILD_ROOT / case_id
        probe_dir.mkdir(parents=True, exist_ok=True)
        out_path = probe_dir / "probe.bin"
        cmd = [str(XCC), "--platform=emu", "--oformat=binary", f"-I{UPSTREAM_TEST / 'framework'}",
               f"-I{source.parent}", "-DNO_LOG_RUNNING", "-DNO_LOG_PASSED"]
        cmd.extend(compiler_args)
        cmd.extend(str(path.resolve()) for path in sources)
        cmd.extend(["-o", str(out_path)])
        result = run_probe(cmd, timeout)
    blocked = blocker_reason(source, text, result.stderr) if result.returncode != 0 else None
    if result.returncode != 0 and blocked is None:
        blocked = fallback_blocker_reason(result.stderr)
    return result.returncode == 0, blocked, timeout


def write_status(supported_compile: list[str], blocked_compile: list[tuple[str, str]],
                 supported_run: list[str], blocked_run: list[tuple[str, str]],
                 skipped_sources: list[tuple[str, str]]) -> None:
    lines = [
        "# z88dk Import Status\n\n",
        "This report is generated by `scripts/generate_z88dk_cases.py`.\n\n",
        f"- supported compile cases: {len(supported_compile)}\n",
        f"- blocked compile cases: {len(blocked_compile)}\n",
        f"- supported run cases: {len(supported_run)}\n",
        f"- blocked run cases: {len(blocked_run)}\n\n",
        f"- skipped helper or harness sources: {len(skipped_sources)}\n\n",
    ]
    if skipped_sources:
        lines.append("## Skipped Helper Or Harness Sources\n\n")
        for rel, reason in skipped_sources:
            lines.append(f"- `{rel}`: {reason}\n")
        lines.append("\n")
    if blocked_compile:
        lines.append("## Blocked Compile Cases\n\n")
        for case_id, reason in blocked_compile:
            lines.append(f"- `{case_id}`: {reason}\n")
        lines.append("\n")
    if blocked_run:
        lines.append("## Blocked Run Cases\n\n")
        for case_id, reason in blocked_run:
            lines.append(f"- `{case_id}`: {reason}\n")
        lines.append("\n")
    STATUS_MD.write_text("".join(lines), encoding="utf-8")


def preblocked_testsuite_case(source: Path) -> tuple[str, str | None] | None:
    rel = source.relative_to(UPSTREAM_TESTSUITE)

    if rel == Path("Issue_1141_Namespaces.c"):
        return "z88dk address-space namespace extension", None
    if rel == Path("Issue_1178_kr_main_stdc.c"):
        return "legacy K&R function definition syntax", None
    if rel in {
        Path("Issue_1466_float16.c"),
        Path("Issue_1466_float16_addition.c"),
        Path("Issue_1466_float16_compare.c"),
    }:
        return "unsupported _Float16 extension", None
    if rel == Path("Issue_2238_accum.c"):
        return "unsupported _Accum fixed-point extension", None
    if rel == Path("offsetof.c"):
        return "z88dk builtin offsetof extension", None

    return None


def main() -> int:
    if not XCC.exists():
        print(f"xcc not found: {XCC}", file=sys.stderr)
        return 1

    ensure_clean_tree()

    blocked_runtime_catalog: dict[str, object] = {}
    supported_compile: list[str] = []
    blocked_compile: list[tuple[str, str]] = []
    supported_run: list[str] = []
    blocked_run: list[tuple[str, str]] = []
    skipped_sources: list[tuple[str, str]] = []

    for source in testsuite_cases():
        case_id = case_id_for("z88dk_testsuite", source, UPSTREAM_TESTSUITE)
        case_dir = CASES_ROOT / "testsuite" / case_id
        summary = f"z88dk testsuite {source.relative_to(UPSTREAM_TESTSUITE)}"
        text = read_text(source)
        blocked = preblocked_testsuite_case(source)
        timeout = compile_timeout_seconds(text)
        if blocked is None:
            ok, blocked, timeout = probe_compile_case(source, case_id)
        else:
            ok = False
        compiler_args = ["-I{source_dir}"]
        if ok:
            supported_compile.append(case_id)
            write_compile_manifest(case_dir, case_id, summary, [source], compiler_args, True, None, timeout)
        else:
            blocked_compile.append((case_id, blocked[0]))
            write_compile_manifest(case_dir, case_id, summary, [source], compiler_args, False, blocked[1], timeout)

    for source in imported_test_cases():
        skip_reason = skip_imported_source(source)
        if skip_reason is not None:
            skipped_sources.append((str(source.relative_to(UPSTREAM_TEST)), skip_reason))
            continue

        text = read_text(source)
        blocked = preblocked_case(source, text)
        base_case_id = case_id_for("z88dk_test", source, UPSTREAM_TEST)
        variants = imported_case_variants(source, text, base_case_id)

        for case_id, plan_kind, sources, compiler_args, max_steps in variants:
            case_dir = CASES_ROOT / "test" / case_id
            summary = f"z88dk test {source.relative_to(UPSTREAM_TEST)}"
            if case_id != base_case_id:
                summary += f" [{case_id.rsplit('_', 2)[-2]}_{case_id.rsplit('_', 2)[-1]}]"

            variant_blocked = blocked
            if source.is_relative_to(UPSTREAM_TEST):
                rel = source.relative_to(UPSTREAM_TEST)
                if rel == Path("suites/math/math.c") and case_id.endswith("_fmod"):
                    variant_blocked = ("double fmod precision downgrade", None)

            sources = prepare_case_sources(case_id, sources)
            timeout = run_timeout_seconds(text) if plan_kind == "run" else compile_timeout_seconds(text)
            preclassified_runtime_blocker = variant_blocked is not None
            if variant_blocked is None:
                ok, blocked_reason, timeout = probe_runtime_case(
                    source, plan_kind, sources, compiler_args, case_id)
            else:
                ok = False
                blocked_reason = variant_blocked

            if plan_kind == "compile":
                compile_args = ["-I{source_dir}"]
                if source.parent != UPSTREAM_TEST / "framework":
                    compile_args.append(f"-I{FRAMEWORK_INCLUDE}")
                compile_args.extend(compiler_args)
                if ok:
                    supported_compile.append(case_id)
                    write_compile_manifest(case_dir, case_id, summary, sources, compile_args, True, None, timeout)
                else:
                    blocked_compile.append((case_id, blocked_reason[0]))
                    write_compile_manifest(
                        case_dir, case_id, summary, sources, compile_args, False, blocked_reason[1], timeout)
                continue

            if ok:
                supported_run.append(case_id)
                write_run_manifest(
                    case_dir,
                    case_id,
                    summary,
                    sources,
                    compiler_args,
                    timeout,
                    max_steps,
                )
            else:
                blocked_run.append((case_id, blocked_reason[0]))
                blocked_runtime_catalog[case_id] = {
                    "stage": "link",
                    "reason": blocked_reason[0],
                    "marker": blocked_reason[1],
                    "expect_build_failure": not preclassified_runtime_blocker,
                    "timeout_seconds": timeout,
                    "compiler_args": [
                        f"-I{UPSTREAM_TEST / 'framework'}",
                        f"-I{source.parent}",
                        "-DNO_LOG_RUNNING",
                        "-DNO_LOG_PASSED",
                        *compiler_args,
                    ],
                    "sources": [
                        str(path.relative_to(SUITE_ROOT))
                        for path in sources
                    ],
                }
                write_blocked_command_manifest(case_dir, case_id, summary, timeout)

    BLOCKED_JSON.write_text(
        json.dumps(blocked_runtime_catalog, indent=2, sort_keys=True),
        encoding="utf-8",
    )
    write_status(supported_compile, blocked_compile, supported_run, blocked_run, skipped_sources)

    print(f"generated {len(supported_compile) + len(blocked_compile)} compile manifests")
    print(f"generated {len(supported_run)} run manifests")
    print(f"generated {len(blocked_run)} blocked run manifests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
