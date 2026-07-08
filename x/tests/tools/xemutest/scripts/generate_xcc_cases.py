#!/usr/bin/env python3

from __future__ import annotations

import os
import re
import shutil
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[5]
X_ROOT = REPO_ROOT / "x"
XCC_DATA = X_ROOT / "tests" / "tests" / "c23" / "xcc" / "data"
OUT_ROOT = X_ROOT / "tests" / "tests" / "c23" / "cases" / "xcc"

FLOAT_ALL = ["ieee32", "fixed8_8", "fixed16_16", "fixed16_8"]
FLOAT_IEEE_ONLY = ["ieee32"]


def rel_to_case(path: Path, case_dir: Path) -> str:
    return os.path.relpath(path, case_dir)


def detect_float(text: str) -> bool:
    needles = [
        "float",
        "double",
        "long double",
        "_Complex",
        "strtof",
        "strtod",
        "strtold",
        "atof",
        "math.h",
        "__fs",
    ]
    return any(token in text for token in needles)


def load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def detect_float_modes(text: str) -> list[str]:
    ieee_only_needles = [
        "__fsadd",
        "__fssub",
        "__fsmul",
        "__fsdiv",
        "__fscmp",
        "xcc_float_bits",
        "sizeof(float)",
        "sizeof(double)",
        "sizeof(long double)",
        "0x3f800000",
        "0x40000000",
    ]
    if any(token in text for token in ieee_only_needles):
        return FLOAT_IEEE_ONLY
    return FLOAT_ALL


def detect_compile_timeout_seconds(text: str) -> int:
    length = len(text)
    if length >= 20000:
        return 300
    if length >= 8000:
        return 120
    return 20


def has_inline_verify_diagnostics(text: str) -> bool:
    patterns = [
        r"\bexpected-(?:error|warning|note)\b",
        r"\bcpp-error(?:-re)?\b",
        r"\b[\w-]+-(?:error|warning|note)(?:-re)?@",
    ]
    return any(re.search(pattern, text) for pattern in patterns)


def is_preprocess_output_test(text: str) -> bool:
    return re.search(r"//\s*RUN:.*(?:^|[ \t])-E(?:[ \t]|$)", text, re.MULTILINE) is not None


def split_opts(path: Path) -> list[str]:
    if not path.exists():
        return []
    return path.read_text(encoding="utf-8").split()


def normalize_compiler_args(kind: str, tokens: list[str]) -> list[str]:
    keep: list[str] = []
    for token in tokens:
        if token == "-S":
            continue
        if token.startswith("-O"):
            continue
        if token.startswith("--float-format="):
            continue
        if token.endswith((".rel", ".o", ".a", ".lib")):
            continue
        keep.append(token)
    if kind == "run":
        keep.append("-I{suite_root}/xcc/data/exec/include")
    return keep


def has_unsupported_support_inputs(tokens: list[str]) -> bool:
    return any(token.endswith((".rel", ".o", ".a", ".lib")) for token in tokens)


def case_id(parts: list[str], stem: str) -> str:
    clean_parts = [part.lower().replace("-", "_") for part in parts]
    return "_".join(["xcc"] + clean_parts + [stem.lower()])


def case_dir_for(parts: list[str], test_id: str) -> Path:
    clean_parts = [part.lower().replace("-", "_") for part in parts]
    return OUT_ROOT.joinpath(*clean_parts, test_id)


def write_manifest(case_dir: Path, lines: list[str]) -> None:
    case_dir.mkdir(parents=True, exist_ok=True)
    (case_dir / "test.cfg").write_text("".join(lines), encoding="utf-8")


def append_sidecar_manifest_lines(lines: list[str], source: Path) -> None:
    sidecar = source.with_suffix(".xemu")
    if not sidecar.exists():
        return

    for raw_line in sidecar.read_text(encoding="utf-8").splitlines():
        stripped = raw_line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        lines.append(raw_line + "\n")


def add_common_manifest_lines(
    lines: list[str],
    test_id: str,
    kind: str,
    summary: str,
    source_rel: str,
    compiler_args: list[str],
    float_present: bool,
    float_modes: list[str],
) -> None:
    lines.extend(
        [
            f"id = {test_id}\n",
            "runner = xemu\n",
            f"kind = {kind}\n",
            "component = xcc\n",
            f"summary = {summary}\n",
            f"source = {source_rel}\n",
            "matrix_opt = Os\n",
            "matrix_opt = O3\n",
        ]
    )
    for arg in compiler_args:
        lines.append(f"compiler_arg = {arg}\n")
    if float_present:
        lines.append("float_present = true\n")
        for mode in float_modes:
            lines.append(f"matrix_float = {mode}\n")


def generate_compile_case(source: Path, suite_parts: list[str]) -> bool:
    stem = source.stem
    opts_path = source.with_suffix(".opts")
    tokens = split_opts(opts_path)
    if has_unsupported_support_inputs(tokens):
        return False

    text = load_text(source)
    if is_preprocess_output_test(text):
        return False
    if has_inline_verify_diagnostics(text) and not source.with_suffix(".error").exists():
        return False
    float_present = detect_float(text) or any(
        token.startswith("--float-format=") for token in tokens
    )
    float_modes = FLOAT_ALL if float_present else []
    compiler_args = normalize_compiler_args("compile", tokens)
    if (source.parent / "Inputs").is_dir():
        compiler_args.append("-I{source_dir}/Inputs")
    timeout_seconds = detect_compile_timeout_seconds(text)
    test_id = case_id(suite_parts, stem)
    case_dir = case_dir_for(suite_parts, test_id)
    source_rel = rel_to_case(source, case_dir)

    lines: list[str] = []
    add_common_manifest_lines(
        lines,
        test_id,
        "compile",
        f"xcc {'/'.join(suite_parts)} {stem}",
        source_rel,
        compiler_args,
        float_present,
        float_modes,
    )

    error_path = source.with_suffix(".error")
    warning_path = source.with_suffix(".warning")
    nowarning_path = source.with_suffix(".nowarning")
    if error_path.exists():
        lines.append("expect_compile = failure\n")
        lines.append(
            f"stderr_contains = {error_path.read_text(encoding='utf-8').strip()}\n"
        )
    else:
        lines.append("expect_compile = success\n")
    if warning_path.exists():
        lines.append(
            f"stderr_contains = {warning_path.read_text(encoding='utf-8').strip()}\n"
        )
    if nowarning_path.exists():
        lines.append(
            f"stderr_not_contains = {nowarning_path.read_text(encoding='utf-8').strip()}\n"
        )
    if timeout_seconds != 20:
        lines.append(f"timeout_seconds = {timeout_seconds}\n")

    append_sidecar_manifest_lines(lines, source)
    write_manifest(case_dir, lines)
    return True


def generate_run_case(source: Path, suite_parts: list[str]) -> None:
    stem = source.stem
    tokens = split_opts(source.with_suffix(".opts"))
    text = load_text(source)
    float_present = detect_float(text)
    float_modes = detect_float_modes(text) if float_present else []
    compiler_args = normalize_compiler_args("run", tokens)
    test_id = case_id(suite_parts, stem)
    case_dir = case_dir_for(suite_parts, test_id)
    source_rel = rel_to_case(source, case_dir)

    lines: list[str] = []
    add_common_manifest_lines(
        lines,
        test_id,
        "run",
        f"xcc {'/'.join(suite_parts)} {stem}",
        source_rel,
        compiler_args,
        float_present,
        float_modes,
    )
    lines.extend(
        [
            "timeout_seconds = 30\n",
            "max_steps = 10000000\n",
        ]
    )

    append_sidecar_manifest_lines(lines, source)
    write_manifest(case_dir, lines)


def iter_sources(root: Path) -> list[Path]:
    return sorted(path for path in root.rglob("*.c") if path.is_file())


def main() -> None:
    if OUT_ROOT.exists():
        shutil.rmtree(OUT_ROOT)
    OUT_ROOT.mkdir(parents=True, exist_ok=True)

    moved = 0
    skipped = 0

    for source in iter_sources(XCC_DATA / "core"):
        if generate_compile_case(source, ["core"]):
            moved += 1
        else:
            skipped += 1

    for source in iter_sources(XCC_DATA / "sema"):
        if generate_compile_case(source, ["sema"]):
            moved += 1
        else:
            skipped += 1

    for source in iter_sources(XCC_DATA / "external"):
        rel = source.relative_to(XCC_DATA / "external")
        suite_parts = ["external"] + [part.lower() for part in rel.parts[:-1]]
        if generate_compile_case(source, suite_parts):
            moved += 1
        else:
            skipped += 1

    for source in iter_sources(XCC_DATA / "exec"):
        rel = source.relative_to(XCC_DATA / "exec")
        suite_parts = ["exec"] + [part.lower() for part in rel.parts[:-1]]
        generate_run_case(source, suite_parts)
        moved += 1

    print(f"generated {moved} xcc case manifests")
    if skipped:
        print(f"skipped {skipped} compile-only support/codegen cases")


if __name__ == "__main__":
    main()
