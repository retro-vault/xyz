#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


MODEL_TAGS = ("model-s", "model-m", "model-l")
PRINTF_NAMES = (
    "printf",
    "fprintf",
    "sprintf",
    "snprintf",
    "vprintf",
    "vfprintf",
    "vsprintf",
    "vsnprintf",
    "scanf",
    "fscanf",
    "sscanf",
    "vscanf",
    "vfscanf",
    "vsscanf",
)

PRINTF_CALL_RE = re.compile(r"\b(?:" + "|".join(PRINTF_NAMES) + r")\s*\(")
FLOAT_FMT_RE = re.compile(r"%(?:[-+ #0*0-9.hlLjzt]*)(?:[aAeEfFgG])")
LONG_LONG_RE = re.compile(r"\blong\s+long\b")
LONG_DOUBLE_RE = re.compile(r"\blong\s+double\b")
DOUBLE_RE = re.compile(r"\bdouble\b")
FLOAT_RE = re.compile(r"\bfloat\b")
LONG_RE = re.compile(r"\blong\b")
DOUBLE_PARSE_RE = re.compile(r"\b(?:strtod|strtold|wcstod|wcstold|atof)\b")
FLOAT_PARSE_RE = re.compile(r"\b(?:strtof|wcstof)\b")
LL_LITERAL_RE = re.compile(r"\b(?:0[xX][0-9A-Fa-f]+|\d+)(?:ULL|LL|ull|ll|uLL|Ull|uLl|ULL)\b")
LONG_LITERAL_RE = re.compile(r"\b(?:0[xX][0-9A-Fa-f]+|\d+)(?:UL|LU|L|ul|lu|l)\b")
INT32_TYPE_RE = re.compile(r"\b(?:u?int32_t|u?int_least32_t|u?int_fast32_t)\b")
INT64_TYPE_RE = re.compile(r"\b(?:u?int64_t|u?int_least64_t|u?int_fast64_t)\b")
INTMAX_TYPE_RE = re.compile(r"\b(?:intmax_t|uintmax_t|imaxdiv_t)\b")
INTMAX_API_RE = re.compile(r"\b(?:strtoimax|strtoumax|wcstoimax|wcstoumax|imaxabs|imaxdiv)\b")


def normalize_key(key: str) -> str:
    return "".join(ch for ch in key.lower() if ch.isalnum())


def parse_manifest(path: Path) -> dict[str, list[str]]:
    data: dict[str, list[str]] = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        raw_key, value = (part.strip() for part in line.split("=", 1))
        key = normalize_key(raw_key)
        data.setdefault(key, []).append(value)
    return data


def strip_comments_and_strings(text: str) -> str:
    out: list[str] = []
    i = 0
    length = len(text)
    while i < length:
        ch = text[i]
        if ch == "/" and i + 1 < length and text[i + 1] == "/":
            i += 2
            while i < length and text[i] != "\n":
                i += 1
            continue
        if ch == "/" and i + 1 < length and text[i + 1] == "*":
            i += 2
            while i + 1 < length and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2
            continue
        if ch in ("'", '"'):
            quote = ch
            out.append(" ")
            i += 1
            while i < length:
                if text[i] == "\\":
                    i += 2
                elif text[i] == quote:
                    i += 1
                    break
                else:
                    i += 1
            continue
        out.append(ch)
        i += 1
    return "".join(out)


def source_features(source_paths: list[Path]) -> set[str]:
    features: set[str] = set()
    for source_path in source_paths:
        text = source_path.read_text(encoding="utf-8")
        stripped = strip_comments_and_strings(text)

        if PRINTF_CALL_RE.search(text) and FLOAT_FMT_RE.search(text):
            features.add("stdio-float")

        if LONG_LONG_RE.search(stripped) or LL_LITERAL_RE.search(stripped):
            features.add("longlong")
        if LONG_DOUBLE_RE.search(stripped):
            features.add("longdouble")
        if INT64_TYPE_RE.search(stripped):
            features.add("longlong")
        if INTMAX_TYPE_RE.search(stripped) or INTMAX_API_RE.search(stripped):
            features.add("longlong")
        if INT32_TYPE_RE.search(stripped):
            features.add("long")

        no_long_variants = LONG_DOUBLE_RE.sub(" ", stripped)
        no_long_variants = LONG_LONG_RE.sub(" ", no_long_variants)

        if DOUBLE_RE.search(no_long_variants):
            features.add("double")
        if FLOAT_RE.search(no_long_variants):
            features.add("float")
        if LONG_RE.search(no_long_variants) or LONG_LITERAL_RE.search(no_long_variants):
            features.add("long")
        if DOUBLE_PARSE_RE.search(no_long_variants):
            features.add("double-parse")
        if FLOAT_PARSE_RE.search(no_long_variants):
            features.add("float-parse")

    return features


def resolve_sources(manifest_path: Path, manifest: dict[str, list[str]]) -> list[Path]:
    return [(manifest_path.parent / rel).resolve() for rel in manifest.get("source", [])]


def determine_model_tags(manifest_path: Path, manifest: dict[str, list[str]]) -> tuple[str, ...]:
    kind = manifest.get("kind", ["run"])[0].lower()
    runner = manifest.get("runner", ["xemu"])[0].lower()
    if kind != "run" or runner == "command":
        return MODEL_TAGS

    tags = set(manifest.get("tag", []))
    features = source_features(resolve_sources(manifest_path, manifest))

    if "double" in tags or "double-regression" in tags:
        features.add("double")
    if "long" in tags or "int32" in tags:
        features.add("long")
    if "int64" in tags:
        features.add("longlong")

    models = {"S", "M", "L"}
    if {"double", "double-parse", "longdouble", "longlong", "stdio-float"} & features:
        models = {"L"}
    else:
        if {"float", "float-parse", "long"} & features:
            models.discard("S")

    return tuple(f"model-{model.lower()}" for model in ("S", "M", "L") if model in models)


def refresh_manifest(path: Path) -> bool:
    manifest = parse_manifest(path)
    desired_tags = determine_model_tags(path, manifest)

    original_lines = path.read_text(encoding="utf-8").splitlines()
    filtered_lines: list[str] = []
    for line in original_lines:
        stripped = line.strip()
        if stripped and not stripped.startswith("#") and "=" in stripped:
            raw_key, value = (part.strip() for part in stripped.split("=", 1))
            if normalize_key(raw_key) == "tag" and value in MODEL_TAGS:
                continue
        filtered_lines.append(line)

    insert_at = -1
    fallback_at = -1
    for index, line in enumerate(filtered_lines):
        stripped = line.strip()
        if not stripped or stripped.startswith("#") or "=" not in stripped:
            continue
        raw_key, _ = (part.strip() for part in stripped.split("=", 1))
        key = normalize_key(raw_key)
        if key == "tag":
            insert_at = index + 1
        elif key == "summary" and fallback_at < 0:
            fallback_at = index + 1

    if insert_at < 0:
        insert_at = fallback_at if fallback_at >= 0 else len(filtered_lines)

    model_lines = [f"tag = {tag}" for tag in desired_tags]
    updated_lines = filtered_lines[:insert_at] + model_lines + filtered_lines[insert_at:]
    updated_text = "\n".join(updated_lines) + "\n"
    original_text = path.read_text(encoding="utf-8")
    if updated_text == original_text:
        return False
    path.write_text(updated_text, encoding="utf-8")
    return True


def main() -> int:
    repo_root = Path(__file__).resolve().parents[5]
    suite_root = repo_root / "x" / "tests" / "tests" / "c23" / "cases"
    changed = 0
    for manifest_path in sorted(suite_root.rglob("test.cfg")):
        if refresh_manifest(manifest_path):
            changed += 1
    print(f"updated {changed} manifests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
