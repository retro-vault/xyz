#!/usr/bin/env python3
"""
Generate the C23 feature test tree and human-readable catalog documents.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tests.spec.c23_suite import FEATURES

FEATURE_ROOT = ROOT / "tests" / "cases"
CATALOG_PATH = ROOT / "docs" / "research" / "c23-feature-catalog.md"
MANIFEST_PATH = ROOT / "tests" / "cases" / "manifest.json"


KIND_TO_SOURCE = {
    "run": "run.c",
    "compile": "pass.c",
    "negative-compile": "fail.c",
}


def write_text(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def feature_dir(feature: dict[str, object]) -> Path:
    category = str(feature["category"])
    feature_id = str(feature["id"])
    return FEATURE_ROOT / category / feature_id


def feature_readme(feature: dict[str, object]) -> str:
    feature_id = str(feature["id"])
    category = str(feature["category"])
    title = str(feature["title"])
    kind = str(feature["kind"])
    support = str(feature["support"])
    references = feature["references"]
    source_file = KIND_TO_SOURCE[kind]

    lines = [
        f"# {feature_id}",
        "",
        title,
        "",
        f"- Category: `{category}`",
        f"- Support level: `{support}`",
        f"- Primary test file: `{source_file}`",
        f"- Test kind: `{kind}`",
    ]

    if kind == "run":
        allowed = ", ".join(repr(item.strip()) for item in feature["allowed_stdout"])
        lines.append(f"- Allowed stdout: `{allowed}`")
    elif kind == "compile":
        lines.append("- Expected outcome: compilation succeeds.")
    else:
        lines.append("- Expected outcome: compilation fails.")

    lines.extend(["", "## References", ""])
    for reference in references:
        lines.append(f"- {reference}")

    return "\n".join(lines) + "\n"


def catalog_markdown() -> str:
    lines = [
        "# C23 Feature Catalog",
        "",
        "This catalog is generated from `tests/spec/c23_suite.py`.",
        "",
        "| Feature | Category | Kind | Support | Notes |",
        "| --- | --- | --- | --- | --- |",
    ]

    for feature in FEATURES:
        notes = "; ".join(str(ref) for ref in feature["references"])
        lines.append(
            "| "
            f"`{feature['id']}` | "
            f"`{feature['category']}` | "
            f"`{feature['kind']}` | "
            f"`{feature['support']}` | "
            f"{notes} |"
        )

    lines.append("")
    return "\n".join(lines)


def manifest_json() -> str:
    public_manifest = []

    for feature in FEATURES:
        item = {
            "id": feature["id"],
            "category": feature["category"],
            "title": feature["title"],
            "kind": feature["kind"],
            "support": feature["support"],
            "references": feature["references"],
            "path": str(feature_dir(feature).relative_to(ROOT)),
            "source_file": KIND_TO_SOURCE[str(feature["kind"])],
            "cflags": feature["cflags"],
        }

        if feature["kind"] == "run":
            item["allowed_stdout"] = feature["allowed_stdout"]

        public_manifest.append(item)

    return json.dumps(public_manifest, indent=2) + "\n"


def main() -> None:
    for feature in FEATURES:
        case_dir = feature_dir(feature)
        source_name = KIND_TO_SOURCE[str(feature["kind"])]

        for name, content in feature["files"].items():
            target_name = source_name if name == "main.c" else name
            write_text(case_dir / target_name, str(content))

        write_text(case_dir / "README.md", feature_readme(feature))

    write_text(CATALOG_PATH, catalog_markdown())
    write_text(MANIFEST_PATH, manifest_json())


if __name__ == "__main__":
    main()
