#!/usr/bin/env python3
"""
Compile and execute the generated C23 suite to produce a compatibility matrix.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tests.spec.c23_suite import FEATURES


BUILD_ROOT = ROOT / "build"
RESULT_ROOT = BUILD_ROOT / "results"
MATRIX_PATH = ROOT / "docs" / "dist" / "c23-compatibility-matrix.md"

COMMON_FLAGS = ["-Wall", "-Wextra", "-pedantic"]
STD_CANDIDATES = ["-std=c23", "-std=c2x", "-std=gnu2x"]
SOURCE_NAME = {
    "run": "run.c",
    "compile": "pass.c",
    "negative-compile": "fail.c",
}
DEFAULT_COMPILE_TEMPLATE = [
    "{compiler}",
    "{std_flag}",
    "{common_flags}",
    "{profile_flags}",
    "{include_flags}",
    "{extra_cflags}",
    "{source}",
    "-o",
    "{executable}",
]
DEFAULT_RUN_TEMPLATE = ["{executable}"]


@dataclass
class FeatureResult:
    feature_id: str
    category: str
    title: str
    kind: str
    status: str
    detail: str
    compile_returncode: int
    run_returncode: int | None = None
    stdout: str = ""
    stderr: str = ""


@dataclass
class RunnerProfile:
    compiler: str
    label: str | None = None
    std_flag: str | None = None
    detect_std_flags: list[str] | None = None
    artifact_name: str = "case.exe"
    compile_template: list[str] = field(default_factory=lambda: list(DEFAULT_COMPILE_TEMPLATE))
    run_template: list[str] = field(default_factory=lambda: list(DEFAULT_RUN_TEMPLATE))
    compile_flags: list[str] = field(default_factory=list)
    common_flags: list[str] = field(default_factory=lambda: list(COMMON_FLAGS))
    version_command: list[str] | None = None
    run_success_returncodes: list[int] = field(default_factory=lambda: [0])
    profile_path: str | None = None


def feature_dir(feature: dict[str, object]) -> Path:
    return ROOT / "tests" / "cases" / str(feature["category"]) / str(feature["id"])


def detect_compiler(compiler: str) -> str:
    path = shutil.which(compiler)
    if path is None:
        raise FileNotFoundError(f"Compiler '{compiler}' is not available.")
    return path


def load_profile(path: str) -> RunnerProfile:
    profile_path = Path(path)
    payload = json.loads(profile_path.read_text(encoding="utf-8"))
    return RunnerProfile(
        compiler=payload["compiler"],
        label=payload.get("label"),
        std_flag=payload.get("std_flag"),
        detect_std_flags=payload.get("detect_std_flags"),
        artifact_name=payload.get("artifact_name", "case.exe"),
        compile_template=payload.get("compile_template", list(DEFAULT_COMPILE_TEMPLATE)),
        run_template=payload.get("run_template", list(DEFAULT_RUN_TEMPLATE)),
        compile_flags=payload.get("compile_flags", []),
        common_flags=payload.get("common_flags", list(COMMON_FLAGS)),
        version_command=payload.get("version_command"),
        run_success_returncodes=payload.get("run_success_returncodes", [0]),
        profile_path=str(profile_path),
    )


def detect_std_flag(compiler_path: str, flags: list[str]) -> str:
    for flag in flags:
        command = [compiler_path, flag, "-x", "c", "-", "-o", os_devnull()]
        completed = subprocess.run(
            command,
            input="int main(void) { return 0; }\n",
            text=True,
            capture_output=True,
        )
        if completed.returncode == 0:
            return flag
    raise RuntimeError(f"Unable to find a working C23 flag for {compiler_path}.")


def os_devnull() -> str:
    return "/dev/null"


def expand_template(items: list[str], context: dict[str, object]) -> list[str]:
    expanded_items: list[str] = []

    for item in items:
        if item.startswith("{") and item.endswith("}"):
            key = item[1:-1]
            if key in context and isinstance(context[key], list):
                expanded_items.extend(str(value) for value in context[key] if str(value))
                continue

            if key in context and not isinstance(context[key], list) and str(context[key]):
                expanded_items.append(str(context[key]))
                continue

        expanded = item
        for key, value in context.items():
            token = "{" + key + "}"
            if isinstance(value, list):
                continue
            expanded = expanded.replace(token, str(value))

        if expanded:
            expanded_items.append(expanded)

    return expanded_items


def compiler_label(compiler_path: str, profile: RunnerProfile) -> str:
    if profile.label:
        return profile.label

    command = profile.version_command or ["{compiler}", "--version"]
    expanded = expand_template(command, {"compiler": compiler_path})
    completed = subprocess.run(
        expanded,
        capture_output=True,
        text=True,
        check=False,
    )
    first_line = completed.stdout.splitlines()[0].strip() if completed.stdout else compiler_path
    cleaned = first_line.replace("/", "_").replace(" ", "_")
    return cleaned


def compile_feature(
    profile: RunnerProfile,
    compiler_path: str,
    std_flag: str,
    feature: dict[str, object],
    label: str,
) -> tuple[subprocess.CompletedProcess[str], Path, Path, Path]:
    case_dir = feature_dir(feature)
    build_dir = RESULT_ROOT / label / str(feature["category"]) / str(feature["id"])
    source_path = case_dir / SOURCE_NAME[str(feature["kind"])]
    executable = build_dir / profile.artifact_name
    extra_flags = list(feature["cflags"])

    build_dir.mkdir(parents=True, exist_ok=True)

    context: dict[str, object] = {
        "compiler": compiler_path,
        "std_flag": std_flag,
        "source": str(source_path),
        "executable": str(executable),
        "case_dir": str(case_dir),
        "build_dir": str(build_dir),
        "tests_data_dir": str(ROOT / "tests" / "data"),
        "root": str(ROOT),
        "common_flags": list(profile.common_flags),
        "profile_flags": list(profile.compile_flags),
        "extra_cflags": extra_flags,
        "include_flags": [
            "-I",
            str(case_dir),
            "-I",
            str(ROOT / "tests" / "data"),
        ],
    }
    command = expand_template(profile.compile_template, context)

    completed = subprocess.run(command, capture_output=True, text=True, check=False)
    return completed, executable, build_dir, case_dir


def run_executable(
    profile: RunnerProfile,
    executable: Path,
    build_dir: Path,
    case_dir: Path,
) -> subprocess.CompletedProcess[str]:
    context: dict[str, object] = {
        "compiler": profile.compiler,
        "executable": str(executable),
        "case_dir": str(case_dir),
        "build_dir": str(build_dir),
        "tests_data_dir": str(ROOT / "tests" / "data"),
        "root": str(ROOT),
    }
    command = expand_template(profile.run_template, context)
    return subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=False,
        cwd=str(build_dir),
    )


def evaluate_feature(
    profile: RunnerProfile,
    compiler_path: str,
    std_flag: str,
    feature: dict[str, object],
    label: str,
    run_mode: str,
) -> FeatureResult:
    compile_result, executable, build_dir, case_dir = compile_feature(
        profile,
        compiler_path,
        std_flag,
        feature,
        label,
    )
    kind = str(feature["kind"])

    if kind == "negative-compile":
        if compile_result.returncode != 0:
            return FeatureResult(
                str(feature["id"]),
                str(feature["category"]),
                str(feature["title"]),
                kind,
                "PASS",
                "Compilation failed as expected.",
                compile_result.returncode,
                stdout=compile_result.stdout,
                stderr=compile_result.stderr,
            )

        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "FAIL",
            "Compilation unexpectedly succeeded.",
            compile_result.returncode,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
        )

    if compile_result.returncode != 0:
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "FAIL",
            "Compilation failed.",
            compile_result.returncode,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
        )

    if kind == "compile":
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "PASS",
            "Compilation succeeded.",
            compile_result.returncode,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
        )

    if run_mode == "never":
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "NOT-RUN",
            "Compilation succeeded, but execution was skipped.",
            compile_result.returncode,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
        )

    if not profile.run_template:
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "NOT-RUN",
            "Compilation succeeded, but no runner is configured.",
            compile_result.returncode,
            stdout=compile_result.stdout,
            stderr=compile_result.stderr,
        )

    run_result = run_executable(profile, executable, build_dir, case_dir)
    allowed_stdout = list(feature["allowed_stdout"])

    if run_result.returncode not in profile.run_success_returncodes:
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "FAIL",
            "Executable returned a non-zero status.",
            compile_result.returncode,
            run_result.returncode,
            run_result.stdout,
            run_result.stderr,
        )

    if run_result.stdout not in allowed_stdout:
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "FAIL",
            "Program stdout did not match the expected output.",
            compile_result.returncode,
            run_result.returncode,
            run_result.stdout,
            run_result.stderr,
        )

    if run_result.stdout.startswith("NOT-CLAIMED "):
        return FeatureResult(
            str(feature["id"]),
            str(feature["category"]),
            str(feature["title"]),
            kind,
            "NOT-CLAIMED",
            "Optional feature is not claimed by this implementation.",
            compile_result.returncode,
            run_result.returncode,
            run_result.stdout,
            run_result.stderr,
        )

    return FeatureResult(
        str(feature["id"]),
        str(feature["category"]),
        str(feature["title"]),
        kind,
        "PASS",
        "Compilation and execution succeeded.",
        compile_result.returncode,
        run_result.returncode,
        run_result.stdout,
        run_result.stderr,
    )


def write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def write_matrix(label: str, results: list[FeatureResult]) -> None:
    lines = [
        "# C23 Compatibility Matrix",
        "",
        f"Compiler label: `{label}`",
        "",
        "Legend: `PASS`, `FAIL`, `NOT-CLAIMED`, `NOT-RUN`.",
        "",
        "| Feature | Category | Kind | Status | Detail |",
        "| --- | --- | --- | --- | --- |",
    ]

    for result in results:
        lines.append(
            "| "
            f"`{result.feature_id}` | "
            f"`{result.category}` | "
            f"`{result.kind}` | "
            f"`{result.status}` | "
            f"{result.detail} |"
        )

    lines.append("")
    MATRIX_PATH.parent.mkdir(parents=True, exist_ok=True)
    MATRIX_PATH.write_text("\n".join(lines), encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--compiler", help="Compiler executable to use.")
    parser.add_argument("--profile", help="Path to a compiler profile JSON file.")
    parser.add_argument(
        "--run-mode",
        choices=["auto", "never"],
        default="auto",
        help="Whether to execute runnable tests after compilation.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.profile:
        profile = load_profile(args.profile)
    else:
        profile = RunnerProfile(compiler="gcc")

    if args.compiler:
        profile.compiler = args.compiler

    compiler_path = detect_compiler(profile.compiler)
    if profile.std_flag is None:
        detect_flags = profile.detect_std_flags or list(STD_CANDIDATES)
        std_flag = detect_std_flag(compiler_path, detect_flags)
    else:
        std_flag = profile.std_flag

    label = compiler_label(compiler_path, profile)
    results: list[FeatureResult] = []

    for feature in FEATURES:
        results.append(
            evaluate_feature(
                profile,
                compiler_path,
                std_flag,
                feature,
                label,
                args.run_mode,
            )
        )

    json_payload = {
        "compiler": profile.compiler,
        "compiler_path": compiler_path,
        "compiler_label": label,
        "profile_path": profile.profile_path,
        "std_flag": std_flag,
        "run_mode": args.run_mode,
        "results": [result.__dict__ for result in results],
    }

    write_json(RESULT_ROOT / f"{label}.json", json_payload)
    write_matrix(label, results)


if __name__ == "__main__":
    main()
