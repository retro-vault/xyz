#!/usr/bin/env python3
"""Download, curate, and compare C algorithm samples from TheAlgorithms/C.

The harness has two modes:

1. Selection mode (`--discover`):
   - uses the source snapshot in `tests/corpus/upstream/thealgorithms-c`
   - generates wrappers around external samples
   - runs them under host gcc and xcc+z80 emulator
   - selects a requested number of cases before observing compiler results
   - optionally writes the complete selection to `manifest.json`

2. Manifest mode (default when `manifest.json` exists):
   - syncs the pinned source commit
   - reruns the selected cases from the manifest

The goal is reproducible corpus coverage, not a best-effort live scrape on each
run.
"""

from __future__ import annotations

import argparse
import dataclasses
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import textwrap
from typing import Iterable


SOURCE_REPO = "https://github.com/TheAlgorithms/C.git"
DEFAULT_LIMIT = 60
DEFAULT_MIN_FLOAT = 5
DEFAULT_GENERIC_INPUT = "10\n5\n3\n2\n1\n0\n-1\n42\n7\n11\n13\n17\n19\nend\n"
MAX_TARGET_OBJECT_BYTES = 0x10000


def approximate_target_type_size(type_words: str) -> int:
    words = type_words.split()
    if "double" in words or words.count("long") >= 2:
        return 8
    if "float" in words or "long" in words:
        return 4
    if "char" in words:
        return 1
    return 2


def target_object_rejection_reason(source: str) -> str | None:
    # Discovery is for runnable Z80 corpus cases. Host-scale examples such as
    # `int arr[1000005]` expand into multi-megabyte stack frames and millions of
    # zeroing stores, so reject them before invoking xcc.
    decl_re = re.compile(
        r"\b((?:(?:const|volatile|static|unsigned|signed|short|long|int|char|float|double)\s+)+)"
        r"[A-Za-z_]\w*\s*\[\s*(\d+)\s*\]"
    )
    for match in decl_re.finditer(source):
        type_words = " ".join(
            word for word in match.group(1).split()
            if word not in {"const", "volatile", "static", "unsigned", "signed"}
        )
        byte_count = int(match.group(2)) * approximate_target_type_size(type_words)
        if byte_count >= MAX_TARGET_OBJECT_BYTES:
            return f"unsupported-target-object:{byte_count}-bytes"
    return None

SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
X_ROOT = SCRIPT_DIR.parents[4]
REPO_ROOT = X_ROOT.parent
ROOT = REPO_ROOT
SOURCE_DIR = X_ROOT / "tests" / "tests" / "c23" / "corpus" / "upstream" / "thealgorithms-c"
BUILD_DIR = REPO_ROOT / "build" / "corpus" / "thealgorithms-c"
MANIFEST_PATH = SCRIPT_DIR / "manifest.json"

RUNNER_SRC = X_ROOT / "tests" / "tests" / "c23" / "xcc" / "tools" / "z80emu" / "z80_exec.cpp"
RUNNER_BIN = REPO_ROOT / "build" / "bin" / "z80_exec"
DEFAULT_XCC = REPO_ROOT / "bin" / "x" / "bin" / "xcc"

GENERIC_EXCLUDED_PREFIXES = {
    "client_server/",
    "developer_tools/",
    "games/",
    "graphics/",
}

GENERIC_EXCLUDED_SUBSTRINGS = (
    "fork(",
    "exec(",
    "socket(",
    "bind(",
    "listen(",
    "accept(",
    "connect(",
    "fopen(",
    "freopen(",
    "tmpfile(",
    "popen(",
    "system(",
    "#include <windows.h>",
)


@dataclasses.dataclass(frozen=True)
class CorpusCase:
    case_id: str
    mode: str
    source: str
    float_rich: bool
    input_text: str = ""
    argv: tuple[str, ...] = ()
    skip_reason: str = ""
    cycle_budget: int = 0


@dataclasses.dataclass
class SourceMeta:
    relpath: str
    has_main: bool
    main_kind: str
    has_assert: bool
    has_simple_test: bool
    float_rich: bool
    uses_scanf: bool
    uses_getchar: bool
    uses_rand: bool
    uses_time: bool
    text: str


@dataclasses.dataclass
class CaseResult:
    case: CorpusCase
    ok: bool
    reason: str
    skipped: bool = False
    host_return: int | None = None
    xcc_return: int | None = None
    host_stdout: str = ""
    xcc_stdout: str = ""


SPECIAL_CASES: list[CorpusCase] = [
    CorpusCase(
        case_id="special_float_celsius_to_fahrenheit",
        mode="special_float_celsius_to_fahrenheit",
        source="conversions/celsius_to_fahrenheit.c",
        float_rich=True,
    ),
    CorpusCase(
        case_id="special_float_lerp",
        mode="special_float_lerp",
        source="math/lerp.c",
        float_rich=True,
    ),
    CorpusCase(
        case_id="special_float_cartesian_to_polar",
        mode="special_float_cartesian_to_polar",
        source="math/cartesian_to_polar.c",
        float_rich=True,
    ),
    CorpusCase(
        case_id="special_float_secant_method",
        mode="special_float_secant_method",
        source="numerical_methods/secant_method.c",
        float_rich=True,
    ),
    CorpusCase(
        case_id="special_float_prime_sqrt",
        mode="special_float_prime_sqrt",
        source="math/prime.c",
        float_rich=True,
    ),
]


def run_cmd(
    argv: list[str],
    *,
    cwd: pathlib.Path | None = None,
    env: dict[str, str] | None = None,
    timeout: int | None = None,
) -> subprocess.CompletedProcess[str]:
    merged_env = os.environ.copy()
    if env:
        merged_env.update(env)
    return subprocess.run(
        argv,
        cwd=str(cwd) if cwd else None,
        env=merged_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        timeout=timeout,
        check=False,
    )


def ensure_runner() -> None:
    RUNNER_BIN.parent.mkdir(parents=True, exist_ok=True)
    if RUNNER_BIN.exists() and RUNNER_BIN.stat().st_mtime >= RUNNER_SRC.stat().st_mtime:
        return
    cmd = [
        "g++",
        "-std=c++17",
        "-I",
        str(RUNNER_SRC.parent),
        "-o",
        str(RUNNER_BIN),
        str(RUNNER_SRC),
    ]
    result = run_cmd(cmd, cwd=ROOT, timeout=120)
    if result.returncode != 0:
        raise RuntimeError(f"failed to build z80_exec:\n{result.stderr}")


def source_snapshot_id() -> str:
    digest = hashlib.sha256()
    for path in sorted(SOURCE_DIR.rglob("*")):
        if not path.is_file() or ".git" in path.parts:
            continue
        relative = path.relative_to(SOURCE_DIR).as_posix().encode("utf-8")
        digest.update(len(relative).to_bytes(4, "little"))
        digest.update(relative)
        with path.open("rb") as source:
            while chunk := source.read(1024 * 1024):
                digest.update(chunk)
    return "snapshot-sha256:" + digest.hexdigest()


def ensure_source_repo(commit: str | None) -> str:
    SOURCE_DIR.parent.mkdir(parents=True, exist_ok=True)
    if not SOURCE_DIR.exists():
        result = run_cmd(
            ["git", "clone", "--depth", "1", SOURCE_REPO, str(SOURCE_DIR)],
            cwd=ROOT,
            timeout=300,
        )
        if result.returncode != 0:
            raise RuntimeError(f"failed to clone source repo:\n{result.stderr}")

    # The project normally carries a vendored snapshot without nested Git
    # metadata.  Never let `git -C` walk upward and operate on the enclosing
    # xyz checkout.
    git_marker = SOURCE_DIR / ".git"
    if not git_marker.exists():
        snapshot = source_snapshot_id()
        if commit and commit != snapshot:
            raise RuntimeError(
                f"vendored source snapshot mismatch: expected {commit}, got {snapshot}"
            )
        return snapshot

    if SOURCE_DIR.exists():
        result = run_cmd(["git", "-C", str(SOURCE_DIR), "fetch", "--depth", "1", "origin"], timeout=300)
        if result.returncode != 0 and commit is None:
            raise RuntimeError(f"failed to fetch source repo:\n{result.stderr}")

    if commit:
        have_commit = run_cmd(
            ["git", "-C", str(SOURCE_DIR), "cat-file", "-e", f"{commit}^{{commit}}"],
            timeout=30,
        )
        if have_commit.returncode != 0:
            result = run_cmd(
                ["git", "-C", str(SOURCE_DIR), "fetch", "--depth", "1", "origin", commit],
                timeout=300,
            )
            if result.returncode != 0:
                raise RuntimeError(f"failed to fetch pinned commit {commit}:\n{result.stderr}")
        result = run_cmd(["git", "-C", str(SOURCE_DIR), "checkout", "--detach", commit], timeout=120)
        if result.returncode != 0:
            raise RuntimeError(f"failed to checkout pinned commit {commit}:\n{result.stderr}")
    else:
        result = run_cmd(["git", "-C", str(SOURCE_DIR), "checkout", "--detach", "origin/HEAD"], timeout=120)
        if result.returncode != 0:
            raise RuntimeError(f"failed to checkout origin/HEAD:\n{result.stderr}")

    result = run_cmd(["git", "-C", str(SOURCE_DIR), "rev-parse", "HEAD"], timeout=30)
    if result.returncode != 0:
        raise RuntimeError(f"failed to get source commit:\n{result.stderr}")
    return result.stdout.strip()


def load_text(path: pathlib.Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def detect_main_kind(text: str) -> tuple[bool, str]:
    match = re.search(r"\bint\s+main\s*\(([^)]*)\)", text, re.S)
    if not match:
        return False, "none"
    params = " ".join(match.group(1).split())
    if params in ("", "void"):
        return True, "void"
    return True, "argv"


def analyze_source(relpath: str) -> SourceMeta:
    src = SOURCE_DIR / relpath
    text = load_text(src)
    has_main, main_kind = detect_main_kind(text)
    return SourceMeta(
        relpath=relpath,
        has_main=has_main,
        main_kind=main_kind,
        has_assert="assert(" in text,
        has_simple_test=bool(
            re.search(
                r"\b(?:static\s+)?void\s+test\s*\(\s*(?:void\s*)?\)",
                text,
            )
        ),
        float_rich=bool(re.search(r"\b(float|double)\b", text)),
        uses_scanf="scanf(" in text,
        uses_getchar="getchar(" in text,
        uses_rand="rand(" in text or "srand(" in text,
        uses_time="time(" in text,
        text=text,
    )


def generic_candidates() -> list[SourceMeta]:
    special_sources = {case.source for case in SPECIAL_CASES}
    metas: list[SourceMeta] = []
    for src in SOURCE_DIR.rglob("*.c"):
        rel = str(src.relative_to(SOURCE_DIR))
        if rel in special_sources:
            continue
        if any(rel.startswith(prefix) for prefix in GENERIC_EXCLUDED_PREFIXES):
            continue
        meta = analyze_source(rel)
        if not meta.has_main:
            continue
        if any(token in meta.text for token in GENERIC_EXCLUDED_SUBSTRINGS):
            continue
        metas.append(meta)

    def score(meta: SourceMeta) -> tuple[int, int, int, str]:
        return (
            0 if meta.has_assert else 1,
            0 if not meta.uses_scanf and not meta.uses_getchar else 1,
            0 if meta.float_rich else 1,
            meta.relpath,
        )

    metas.sort(key=score)
    return metas


def manifest_cases(manifest: dict) -> list[CorpusCase]:
    return [
        CorpusCase(
            case_id=entry["case_id"],
            mode=entry["mode"],
            source=entry["source"],
            float_rich=bool(entry.get("float_rich", False)),
            input_text=entry.get("input_text", ""),
            argv=tuple(entry.get("argv", [])),
            skip_reason=entry.get("skip_reason", ""),
            cycle_budget=int(entry.get("cycle_budget", 0)),
        )
        for entry in manifest.get("cases", [])
    ]


def corpus_prelude(input_text: str) -> str:
    escaped_input = json.dumps(input_text)
    return textwrap.dedent(
        f"""
        #include <stddef.h>
        #include <stdarg.h>
        #include <time.h>

        #ifdef __XCC__
        #include <assert.h>
        static _Noreturn void corpus_target_assert_fail(unsigned int line) {{
            volatile unsigned int *result =
                (volatile unsigned int *)0xff00u;
            volatile unsigned char *done =
                (volatile unsigned char *)0xff02u;
            *result = 0x8000u | (line & 0x7fffu);
            *done = 0xa5u;
            for (;;) {{}}
        }}
        #undef assert
        #define assert(condition) \
            ((condition) ? (void)0 : corpus_target_assert_fail(__LINE__))
        #endif

        static const char corpus_input_data[] = {escaped_input};
        static const char *corpus_input_cursor = corpus_input_data;
        static unsigned long corpus_rand_state = 1u;

        static int corpus_is_space(int ch) {{
            return ch == ' ' || ch == '\\t' || ch == '\\n' || ch == '\\r' ||
                   ch == '\\f' || ch == '\\v';
        }}

        static void corpus_skip_space(void) {{
            while (*corpus_input_cursor && corpus_is_space((unsigned char)*corpus_input_cursor))
                ++corpus_input_cursor;
        }}

        static int corpus_parse_signed(long long *out) {{
            long long sign = 1;
            long long value = 0;
            int saw_digit = 0;

            corpus_skip_space();
            if (*corpus_input_cursor == '+' || *corpus_input_cursor == '-') {{
                if (*corpus_input_cursor == '-')
                    sign = -1;
                ++corpus_input_cursor;
            }}
            while (*corpus_input_cursor >= '0' && *corpus_input_cursor <= '9') {{
                saw_digit = 1;
                value = value * 10 + (long long)(*corpus_input_cursor - '0');
                ++corpus_input_cursor;
            }}
            if (!saw_digit)
                return 0;
            *out = sign * value;
            return 1;
        }}

        static int corpus_parse_unsigned(unsigned long long *out) {{
            unsigned long long value = 0;
            int saw_digit = 0;

            corpus_skip_space();
            if (*corpus_input_cursor == '+')
                ++corpus_input_cursor;
            while (*corpus_input_cursor >= '0' && *corpus_input_cursor <= '9') {{
                saw_digit = 1;
                value = value * 10u + (unsigned long long)(*corpus_input_cursor - '0');
                ++corpus_input_cursor;
            }}
            if (!saw_digit)
                return 0;
            *out = value;
            return 1;
        }}

        static int corpus_parse_double(double *out) {{
            double sign = 1.0;
            double int_part = 0.0;
            double frac_part = 0.0;
            double scale = 1.0;
            int saw_digit = 0;
            int exponent_sign = 1;
            int exponent_value = 0;

            corpus_skip_space();
            if (*corpus_input_cursor == '+' || *corpus_input_cursor == '-') {{
                if (*corpus_input_cursor == '-')
                    sign = -1.0;
                ++corpus_input_cursor;
            }}

            while (*corpus_input_cursor >= '0' && *corpus_input_cursor <= '9') {{
                saw_digit = 1;
                int_part = int_part * 10.0 + (double)(*corpus_input_cursor - '0');
                ++corpus_input_cursor;
            }}

            if (*corpus_input_cursor == '.') {{
                ++corpus_input_cursor;
                while (*corpus_input_cursor >= '0' && *corpus_input_cursor <= '9') {{
                    saw_digit = 1;
                    frac_part = frac_part * 10.0 + (double)(*corpus_input_cursor - '0');
                    scale *= 10.0;
                    ++corpus_input_cursor;
                }}
            }}

            if (!saw_digit)
                return 0;

            *out = sign * (int_part + frac_part / scale);

            if (*corpus_input_cursor == 'e' || *corpus_input_cursor == 'E') {{
                ++corpus_input_cursor;
                if (*corpus_input_cursor == '+' || *corpus_input_cursor == '-') {{
                    if (*corpus_input_cursor == '-')
                        exponent_sign = -1;
                    ++corpus_input_cursor;
                }}
                while (*corpus_input_cursor >= '0' && *corpus_input_cursor <= '9') {{
                    exponent_value = exponent_value * 10 + (*corpus_input_cursor - '0');
                    ++corpus_input_cursor;
                }}
            }}

            while (exponent_value > 0) {{
                if (exponent_sign > 0)
                    *out *= 10.0;
                else
                    *out /= 10.0;
                --exponent_value;
            }}
            return 1;
        }}

        void corpus_srand(unsigned int seed) {{
            corpus_rand_state = seed ? seed : 1u;
        }}

        int corpus_rand(void) {{
            corpus_rand_state = corpus_rand_state * 1103515245u + 12345u;
            return (int)((corpus_rand_state >> 16) & 0x7fffu);
        }}

        time_t corpus_time(time_t *t) {{
            time_t now = (time_t)123456789;
            if (t)
                *t = now;
            return now;
        }}

        clock_t corpus_clock(void) {{
            return (clock_t)0;
        }}

        int corpus_getchar(void) {{
            if (!*corpus_input_cursor)
                return -1;
            return (unsigned char)*corpus_input_cursor++;
        }}

        int corpus_scanf(const char *fmt, ...) {{
            va_list ap;
            int assigned = 0;

            va_start(ap, fmt);
            while (*fmt) {{
                if (corpus_is_space((unsigned char)*fmt)) {{
                    while (corpus_is_space((unsigned char)*fmt))
                        ++fmt;
                    corpus_skip_space();
                    continue;
                }}

                if (*fmt != '%') {{
                    if (*corpus_input_cursor != *fmt)
                        break;
                    ++fmt;
                    if (*corpus_input_cursor)
                        ++corpus_input_cursor;
                    continue;
                }}

                ++fmt;
                int length = 0;
                if (*fmt == 'l') {{
                    ++length;
                    ++fmt;
                    if (*fmt == 'l') {{
                        ++length;
                        ++fmt;
                    }}
                }}

                if (*fmt == '%') {{
                    if (*corpus_input_cursor != '%')
                        break;
                    ++fmt;
                    if (*corpus_input_cursor)
                        ++corpus_input_cursor;
                    continue;
                }}

                switch (*fmt) {{
                case 'd':
                case 'i': {{
                    long long value = 0;
                    if (!corpus_parse_signed(&value)) {{
                        va_end(ap);
                        return assigned;
                    }}
                    if (length >= 2)
                        *va_arg(ap, long long *) = value;
                    else if (length == 1)
                        *va_arg(ap, long *) = (long)value;
                    else
                        *va_arg(ap, int *) = (int)value;
                    ++assigned;
                    break;
                }}
                case 'u': {{
                    unsigned long long value = 0;
                    if (!corpus_parse_unsigned(&value)) {{
                        va_end(ap);
                        return assigned;
                    }}
                    if (length >= 2)
                        *va_arg(ap, unsigned long long *) = value;
                    else if (length == 1)
                        *va_arg(ap, unsigned long *) = (unsigned long)value;
                    else
                        *va_arg(ap, unsigned int *) = (unsigned int)value;
                    ++assigned;
                    break;
                }}
                case 'f':
                case 'g':
                case 'e': {{
                    double value = 0.0;
                    if (!corpus_parse_double(&value)) {{
                        va_end(ap);
                        return assigned;
                    }}
                    if (length >= 1)
                        *va_arg(ap, double *) = value;
                    else
                        *va_arg(ap, float *) = (float)value;
                    ++assigned;
                    break;
                }}
                case 'c': {{
                    if (!*corpus_input_cursor) {{
                        va_end(ap);
                        return assigned;
                    }}
                    *va_arg(ap, char *) = *corpus_input_cursor++;
                    ++assigned;
                    break;
                }}
                case 's': {{
                    char *out = va_arg(ap, char *);
                    corpus_skip_space();
                    if (!*corpus_input_cursor) {{
                        va_end(ap);
                        return assigned;
                    }}
                    while (*corpus_input_cursor &&
                           !corpus_is_space((unsigned char)*corpus_input_cursor)) {{
                        *out++ = *corpus_input_cursor++;
                    }}
                    *out = '\\0';
                    ++assigned;
                    break;
                }}
                default:
                    va_end(ap);
                    return assigned;
                }}
                ++fmt;
            }}
            va_end(ap);
            return assigned;
        }}
        """
    ).strip()


def generic_wrapper(case: CorpusCase, meta: SourceMeta) -> str:
    include_path = SOURCE_DIR / case.source
    lines = [
        corpus_prelude(case.input_text),
        "#define rand corpus_rand",
        "#define srand corpus_srand",
        "#define time corpus_time",
        "#define clock corpus_clock",
        "#define scanf corpus_scanf",
        "#define main ta_algorithm_main",
        f'#include "{include_path}"',
        "#undef main",
        "#undef scanf",
        "#undef clock",
        "#undef time",
        "#undef srand",
        "#undef rand",
    ]
    if meta.has_assert and meta.has_simple_test:
        lines.append("int main(void) { corpus_srand(1u); test(); return 0; }")
        return "\n".join(lines) + "\n"
    if meta.main_kind == "void":
        lines.append("int main(void) { corpus_srand(1u); return ta_algorithm_main(); }")
    else:
        argv = ("ta_algorithm",) + case.argv
        argv_init = ", ".join(json.dumps(arg) for arg in argv)
        lines.append(
            f"int main(void) {{ const char *argv[] = {{{argv_init}, NULL}}; "
            f"corpus_srand(1u); return ta_algorithm_main({len(argv)}, "
            "(char **)argv); }"
        )
    return "\n".join(lines) + "\n"


def special_wrapper(case: CorpusCase) -> str:
    source = SOURCE_DIR / case.source
    common_prefix = corpus_prelude(case.input_text)
    if case.mode == "special_float_celsius_to_fahrenheit":
        body = f"""
        {common_prefix}
        #include <stdio.h>
        #define main ta_algorithm_main
        #include "{source}"
        #undef main
        static int q100(double x) {{
            return x >= 0 ? (int)(x * 100.0 + 0.5) : (int)(x * 100.0 - 0.5);
        }}
        int main(void) {{
            printf("%d %d %d\\n",
                   q100(celcius_to_fahrenheit(0.0)),
                   q100(celcius_to_fahrenheit(100.0)),
                   q100(celcius_to_fahrenheit(22.5)));
            return 0;
        }}
        """
        return textwrap.dedent(body).strip() + "\n"
    if case.mode == "special_float_lerp":
        body = f"""
        {common_prefix}
        #include <stdio.h>
        #define main ta_algorithm_main
        #include "{source}"
        #undef main
        static int q1000(float x) {{
            return x >= 0.0f ? (int)(x * 1000.0f + 0.5f)
                             : (int)(x * 1000.0f - 0.5f);
        }}
        int main(void) {{
            printf("%d %d\\n",
                   q1000(lerp(0.0f, 5.0f, 0.25f)),
                   q1000(lerp_precise(2, 10, 0.375f)));
            return 0;
        }}
        """
        return textwrap.dedent(body).strip() + "\n"
    if case.mode == "special_float_cartesian_to_polar":
        body = f"""
        {common_prefix}
        #include <stdio.h>
        #ifndef M_PI
        #define M_PI 3.14159265358979323846
        #endif
        #define main ta_algorithm_main
        #include "{source}"
        #undef main
        static int q100(double x) {{
            return x >= 0 ? (int)(x * 100.0 + 0.5) : (int)(x * 100.0 - 0.5);
        }}
        int main(void) {{
            double r = 0.0, theta = 0.0;
            to_polar(0.0, 5.0, &r, &theta);
            printf("%d %d\\n", q100(r), q100(theta));
            return 0;
        }}
        """
        return textwrap.dedent(body).strip() + "\n"
    if case.mode == "special_float_secant_method":
        body = f"""
        {common_prefix}
        #include <stdio.h>
        #define main ta_algorithm_main
        #include "{source}"
        #undef main
        static int q1000(double x) {{
            return x >= 0 ? (int)(x * 1000.0 + 0.5) : (int)(x * 1000.0 - 0.5);
        }}
        int main(void) {{
            printf("%d %d\\n",
                   q1000(secant_method(0.2, 0.5, TOLERANCE)),
                   q1000(secant_method(-2.0, -5.0, TOLERANCE)));
            return 0;
        }}
        """
        return textwrap.dedent(body).strip() + "\n"
    if case.mode == "special_float_prime_sqrt":
        body = f"""
        {common_prefix}
        #define main ta_algorithm_main
        #include "{source}"
        #undef main
        int main(void) {{ test(); return 0; }}
        """
        return textwrap.dedent(body).strip() + "\n"
    raise ValueError(f"unknown special mode: {case.mode}")


def wrapper_for_case(case: CorpusCase) -> str:
    if case.mode == "generic_main":
        meta = analyze_source(case.source)
        return generic_wrapper(case, meta)
    return special_wrapper(case)


def compile_host(
    wrapper_path: pathlib.Path,
    host_cc: str,
    host_opt: str,
    out_path: pathlib.Path,
) -> subprocess.CompletedProcess[str]:
    cmd = [host_cc, "-std=c11", host_opt, str(wrapper_path), "-lm", "-o", str(out_path)]
    return run_cmd(cmd, cwd=ROOT, timeout=120)


def compile_xcc(
    wrapper_path: pathlib.Path,
    xcc_path: pathlib.Path,
    xcc_opt: str,
    out_path: pathlib.Path,
) -> subprocess.CompletedProcess[str]:
    cmd = [str(xcc_path), "--platform=emu", "--oformat=binary", xcc_opt, str(wrapper_path), "-o", str(out_path)]
    return run_cmd(
        cmd,
        cwd=ROOT,
        env={"ASAN_OPTIONS": "detect_leaks=0"},
        timeout=180,
    )


def normalize_stdout(data: str) -> str:
    return data.replace("\r\n", "\n")


def run_case(
    case: CorpusCase,
    *,
    host_cc: str,
    host_opt: str,
    xcc_path: pathlib.Path,
    xcc_opt: str,
    cycles: int,
    keep_wrappers: bool,
) -> CaseResult:
    case_dir = BUILD_DIR / "cases" / case.case_id
    if case_dir.exists():
        shutil.rmtree(case_dir)
    case_dir.mkdir(parents=True, exist_ok=True)

    if case.skip_reason:
        return CaseResult(
            case=case,
            ok=False,
            reason=case.skip_reason,
            skipped=True,
        )

    wrapper_path = case_dir / "wrap.c"
    wrapper_text = wrapper_for_case(case)
    wrapper_path.write_text(wrapper_text, encoding="utf-8")

    unsupported_reason = target_object_rejection_reason(wrapper_text)
    if unsupported_reason:
        return CaseResult(case=case, ok=False, reason=unsupported_reason)
    source_path = SOURCE_DIR / case.source
    if source_path.exists():
        unsupported_reason = target_object_rejection_reason(
            source_path.read_text(encoding="utf-8", errors="ignore")
        )
        if unsupported_reason:
            return CaseResult(case=case, ok=False, reason=unsupported_reason)

    host_bin = case_dir / "host"
    host_compile = compile_host(wrapper_path, host_cc, host_opt, host_bin)
    (case_dir / "host.compile.log").write_text(host_compile.stdout + host_compile.stderr, encoding="utf-8")
    if host_compile.returncode != 0:
        return CaseResult(case=case, ok=False, reason="host-compile-failed")

    try:
        host_run = run_cmd([str(host_bin)], cwd=ROOT, timeout=20)
    except subprocess.TimeoutExpired:
        return CaseResult(case=case, ok=False, reason="host-run-timeout")

    host_stdout = normalize_stdout(host_run.stdout)
    (case_dir / "host.stdout").write_text(host_stdout, encoding="utf-8")
    (case_dir / "host.stderr").write_text(host_run.stderr, encoding="utf-8")
    if host_run.returncode != 0:
        return CaseResult(
            case=case,
            ok=False,
            reason="host-run-failed",
            host_return=host_run.returncode,
            host_stdout=host_stdout,
        )

    target_bin = case_dir / "target.bin"
    xcc_compile = compile_xcc(wrapper_path, xcc_path, xcc_opt, target_bin)
    xcc_compile_log = xcc_compile.stdout + xcc_compile.stderr
    (case_dir / "xcc.compile.log").write_text(xcc_compile_log, encoding="utf-8")
    if xcc_compile.returncode != 0:
        if "area placement exceeds 64 KiB address space" in xcc_compile_log:
            return CaseResult(
                case=case,
                ok=False,
                reason="target-image-exceeds-16-bit-address-space-for-profile",
                skipped=True,
                host_return=host_run.returncode,
                host_stdout=host_stdout,
            )
        return CaseResult(
            case=case,
            ok=False,
            reason="xcc-compile-failed",
            host_return=host_run.returncode,
            host_stdout=host_stdout,
        )

    target_stdout_path = case_dir / "target.stdout"
    try:
        target_run = run_cmd(
            [
                str(RUNNER_BIN),
                "--bin",
                "--cycles",
                str(case.cycle_budget or cycles),
                "--stdout",
                str(target_stdout_path),
                str(target_bin),
            ],
            cwd=ROOT,
            timeout=30,
        )
    except subprocess.TimeoutExpired:
        return CaseResult(
            case=case,
            ok=False,
            reason="xcc-run-timeout",
            host_return=host_run.returncode,
            host_stdout=host_stdout,
        )

    summary = target_run.stdout.strip()
    (case_dir / "xcc.summary").write_text(summary + "\n", encoding="utf-8")
    xcc_stdout = normalize_stdout(
        target_stdout_path.read_text(encoding="utf-8", errors="ignore")
        if target_stdout_path.exists()
        else ""
    )
    match = re.search(r"return=(\d+)", summary)
    xcc_return = int(match.group(1)) if match else None

    if target_run.returncode != 0:
        return CaseResult(
            case=case,
            ok=False,
            reason="xcc-run-failed",
            host_return=host_run.returncode,
            xcc_return=xcc_return,
            host_stdout=host_stdout,
            xcc_stdout=xcc_stdout,
        )

    if xcc_return != host_run.returncode:
        if xcc_return is not None and xcc_return & 0x8000:
            return CaseResult(
                case=case,
                ok=False,
                reason=f"target-assertion-failed:line-{xcc_return & 0x7fff}",
                host_return=host_run.returncode,
                xcc_return=xcc_return,
                host_stdout=host_stdout,
                xcc_stdout=xcc_stdout,
            )
        return CaseResult(
            case=case,
            ok=False,
            reason="return-mismatch",
            host_return=host_run.returncode,
            xcc_return=xcc_return,
            host_stdout=host_stdout,
            xcc_stdout=xcc_stdout,
        )

    if xcc_stdout != host_stdout:
        return CaseResult(
            case=case,
            ok=False,
            reason="stdout-mismatch",
            host_return=host_run.returncode,
            xcc_return=xcc_return,
            host_stdout=host_stdout,
            xcc_stdout=xcc_stdout,
        )

    if not keep_wrappers:
        for extra in ("host", "target.bin"):
            path = case_dir / extra
            if path.exists():
                path.unlink()

    return CaseResult(
        case=case,
        ok=True,
        reason="pass",
        host_return=host_run.returncode,
        xcc_return=xcc_return,
        host_stdout=host_stdout,
        xcc_stdout=xcc_stdout,
    )


def discovery_cases(limit: int) -> list[CorpusCase]:
    cases = list(SPECIAL_CASES)
    for meta in generic_candidates():
        cases.append(
            CorpusCase(
                case_id="generic_" + re.sub(r"[^a-z0-9]+", "_", meta.relpath.lower()).strip("_"),
                mode="generic_main",
                source=meta.relpath,
                float_rich=meta.float_rich,
                input_text=DEFAULT_GENERIC_INPUT if meta.uses_scanf or meta.uses_getchar else "",
            )
        )
    return cases[: max(limit * 4, len(SPECIAL_CASES))]


def discover(
    *,
    limit: int,
    min_float: int,
    host_cc: str,
    host_opt: str,
    xcc_path: pathlib.Path,
    xcc_opt: str,
    cycles: int,
    keep_wrappers: bool,
) -> tuple[list[CorpusCase], list[CaseResult]]:
    selected = discovery_cases(limit)[:limit]
    results: list[CaseResult] = []

    if len(selected) < limit:
        raise RuntimeError(
            f"selection found only {len(selected)} eligible cases, need {limit}"
        )
    float_count = sum(1 for case in selected if case.float_rich)
    if float_count < min_float:
        raise RuntimeError(
            f"selection found only {float_count} float-rich cases, need {min_float}"
        )

    # The selection is fixed before running either compiler.  Failed cases
    # remain in the manifest and in the reported result.
    for case in selected:
        result = run_case(
            case,
            host_cc=host_cc,
            host_opt=host_opt,
            xcc_path=xcc_path,
            xcc_opt=xcc_opt,
            cycles=cycles,
            keep_wrappers=keep_wrappers,
        )
        results.append(result)
        status = "PASS" if result.ok else "FAIL"
        print(f"{status} {case.case_id} [{case.source}] {result.reason}",
              flush=True)

    return selected, results


def write_manifest(source_commit: str, cases: Iterable[CorpusCase]) -> None:
    data = {
        "source_repo": SOURCE_REPO,
        "source_commit": source_commit,
        "cases": [
            {
                "case_id": case.case_id,
                "mode": case.mode,
                "source": case.source,
                "float_rich": case.float_rich,
                "input_text": case.input_text,
                **({"argv": list(case.argv)} if case.argv else {}),
                **({"skip_reason": case.skip_reason} if case.skip_reason else {}),
                **({"cycle_budget": case.cycle_budget}
                   if case.cycle_budget else {}),
            }
            for case in cases
        ],
    }
    MANIFEST_PATH.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def load_manifest() -> dict:
    return json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))


def rerun_manifest(
    cases: list[CorpusCase],
    *,
    host_cc: str,
    host_opt: str,
    xcc_path: pathlib.Path,
    xcc_opt: str,
    cycles: int,
    keep_wrappers: bool,
) -> list[CaseResult]:
    results: list[CaseResult] = []
    for case in cases:
        result = run_case(
            case,
            host_cc=host_cc,
            host_opt=host_opt,
            xcc_path=xcc_path,
            xcc_opt=xcc_opt,
            cycles=cycles,
            keep_wrappers=keep_wrappers,
        )
        results.append(result)
        status = "SKIP" if result.skipped else ("PASS" if result.ok else "FAIL")
        print(f"{status} {case.case_id} [{case.source}] {result.reason}",
              flush=True)
    return results


def summarize(results: list[CaseResult]) -> int:
    passed = sum(1 for r in results if r.ok)
    skipped = sum(1 for r in results if r.skipped)
    failed = len(results) - passed - skipped
    float_cases = sum(1 for r in results if r.case.float_rich and r.ok)
    print()
    print(
        f"Results: {passed} passed, {failed} failed, {skipped} skipped, "
        f"{float_cases} float-rich passed"
    )
    if failed:
        print("Failures:")
        for result in results:
            if not result.ok and not result.skipped:
                print(f"  - {result.case.case_id}: {result.reason}")
    if skipped:
        print("Not applicable:")
        for result in results:
            if result.skipped:
                print(f"  - {result.case.case_id}: {result.reason}")
    return 0 if failed == 0 else 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--discover", action="store_true", help="select a deterministic case set instead of using the manifest")
    parser.add_argument("--write-manifest", action="store_true", help="write the complete selected set to manifest.json")
    parser.add_argument("--limit", type=int, default=DEFAULT_LIMIT, help="number of cases to select before testing")
    parser.add_argument("--min-float", type=int, default=DEFAULT_MIN_FLOAT, help="minimum number of float-rich cases")
    parser.add_argument("--cycles", type=int, default=200_000_000, help="emulator cycle budget")
    parser.add_argument("--keep-wrappers", action="store_true", help="keep generated host/target artifacts")
    parser.add_argument("--source-commit", default=None, help="pin the source repository to a specific commit")
    parser.add_argument("--xcc", default=os.environ.get("XCC", str(DEFAULT_XCC)), help="path to xcc binary")
    parser.add_argument("--xcc-opt", default=os.environ.get("XCC_OPT", "-Os"), help="xcc optimization flag")
    parser.add_argument("--host-cc", default=os.environ.get("HOST_CC", "gcc"), help="host C compiler")
    parser.add_argument("--host-opt", default=os.environ.get("HOST_OPT", "-O2"), help="host compiler optimization flag")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    ensure_runner()

    manifest = load_manifest() if MANIFEST_PATH.exists() else None
    if manifest is None and not args.discover:
        raise RuntimeError(
            f"corpus manifest not found: {MANIFEST_PATH}; "
            "run once with --discover --write-manifest"
        )
    pinned_commit = args.source_commit or (manifest.get("source_commit") if manifest and not args.discover else None)
    source_commit = ensure_source_repo(pinned_commit)

    xcc_path = pathlib.Path(args.xcc).resolve()
    if not xcc_path.exists():
        raise RuntimeError(f"xcc binary not found: {xcc_path}")

    if args.discover or manifest is None:
        selected, results = discover(
            limit=args.limit,
            min_float=args.min_float,
            host_cc=args.host_cc,
            host_opt=args.host_opt,
            xcc_path=xcc_path,
            xcc_opt=args.xcc_opt,
            cycles=args.cycles,
            keep_wrappers=args.keep_wrappers,
        )
        if args.write_manifest:
            write_manifest(source_commit, selected)
        return summarize(results)

    cases = manifest_cases(manifest)
    results = rerun_manifest(
        cases,
        host_cc=args.host_cc,
        host_opt=args.host_opt,
        xcc_path=xcc_path,
        xcc_opt=args.xcc_opt,
        cycles=args.cycles,
        keep_wrappers=args.keep_wrappers,
    )
    return summarize(results)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise SystemExit(1)
