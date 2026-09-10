"""Capture or verify the independent nightly archive and shared toolchain inputs."""
import datetime
import hashlib
import json
import pathlib
import subprocess
import sys

mode, directory = sys.argv[1:]
base = pathlib.Path(directory).resolve()
receipt_path = base / "toolchain-receipt.json"
suite = pathlib.Path(__file__).resolve().parent.parent
lock = dict(line.split("=", 1) for line in (suite / "latest.lock").read_text().splitlines()
            if line and not line.startswith("#"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def head(directory):
    return subprocess.check_output(["git", "-C", str(directory), "rev-parse", "HEAD"], text=True).strip()


if mode == "capture":
    nightly = base / "nightly-source/z88dk"
    assert digest(base / "z88dk-latest.tgz") == lock["NIGHTLY_SHA256"]
    assert digest(nightly / "zsdcc_r16639_src.tar.gz") == lock["NIGHTLY_SDCC_SOURCE_SHA256"]
    git_heads = {"z88dk-master": lock["Z88DK_CURRENT_COMMIT"], "sdcc-trunk": lock["SDCC_CURRENT_COMMIT"]}
    for relative, expected in git_heads.items():
        assert head(base / relative) == expected
    files = {base / "z88dk-latest.tgz", nightly / "zsdcc_r16639_src.tar.gz", base / "sdcc-trunk/src/sdcc"}
    for relative in ("bin", "include", "lib/config", "lib/clibs", "lib/target/test"):
        files.update(path for path in (nightly / relative).rglob("*") if path.is_file())
    files.update(path for path in (nightly / "lib").glob("*_rules.*") if path.is_file())
    versions = {}
    for name, program, flag in (
        ("nightly_sccz80", nightly / "bin/z88dk-sccz80", "-h"),
        ("nightly_80cc", nightly / "bin/z88dk-80cc", "-h"),
        ("nightly_zsdcc", nightly / "bin/z88dk-zsdcc", "--version"),
        ("official_sdcc", base / "sdcc-trunk/src/sdcc", "--version"),
    ):
        result = subprocess.run([str(program), flag], capture_output=True, text=True)
        versions[name] = result.stdout + result.stderr
    receipt = {
        "created_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "nightly_archive_url": lock["NIGHTLY_ARCHIVE_URL"],
        "nightly_archive_name": lock["NIGHTLY_ARCHIVE"],
        "git_heads": git_heads,
        "file_hashes": {str(path.relative_to(base)): digest(path) for path in sorted(files)},
        "versions": versions,
    }
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
elif mode == "verify":
    receipt = json.loads(receipt_path.read_text())
    for relative, expected in receipt["file_hashes"].items():
        assert digest(base / relative) == expected, "latest toolchain input changed: " + relative
    for relative, expected in receipt["git_heads"].items():
        assert head(base / relative) == expected, "latest Git identity changed: " + relative
    print("latest archive, compiler binaries, shared target inputs and Git identities verified")
else:
    raise SystemExit("expected capture or verify")
