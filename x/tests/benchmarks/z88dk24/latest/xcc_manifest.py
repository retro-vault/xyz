"""Record the actual compiler and dirty source inputs used for a comparison."""
import hashlib
import json
import pathlib
import subprocess
import sys

arguments = sys.argv[1:]
verify = arguments[0] == "--verify"
if verify:
    arguments = arguments[1:]
root, compiler, destination = map(pathlib.Path, arguments)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


if verify:
    recorded = json.loads(destination.read_text())
    assert digest(compiler) == recorded["compiler_sha256"], "XCC changed during benchmark"
    for relative, expected in recorded["source_hashes"].items():
        assert digest(root / relative) == expected, "XCC source changed during benchmark: " + relative
else:
    paths = ["x/src/xcc", "x/lib/xopt", "x/libc", "x/runtime", "x/mk", "x/Makefile", "Makefile"]
    command = ["git", "-C", str(root), "ls-files", "-z"]
    files = set()
    for flags in ([], ["--others", "--exclude-standard"]):
        output = subprocess.check_output([*command, *flags, "--", *paths])
        files.update(name.decode() for name in output.split(b"\0") if name)
    files = sorted(name for name in files if (root / name).is_file())
    recorded = {
        "compiler": str(compiler.resolve()),
        "compiler_sha256": digest(compiler),
        "source_head": subprocess.check_output(
            ["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip(),
        "dirty_status": subprocess.check_output(
            ["git", "-C", str(root), "status", "--porcelain", "--", *paths], text=True),
        "source_hashes": {name: digest(root / name) for name in files},
    }
    destination.write_text(json.dumps(recorded, indent=2) + "\n")
