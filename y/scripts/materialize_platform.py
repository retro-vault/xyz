#!/usr/bin/env python3
"""Materialize a platform's YOS sources; only section placement may vary.

The manifest lists shared modules and hardware-specific sources explicitly.
No function bodies are patched by this tool. Downstream builds own the output
folder, while this repository owns every source file listed in the manifest.
"""
import argparse
import json
from pathlib import Path
import re

YROOT = Path(__file__).resolve().parents[1]
AREA = re.compile(r'(\.area\s+)(_(?:CODE|KEXT|EXTRA|ROM_\w+|HEADER_DATA))\b')


def files_for(platform):
    path = YROOT/'platforms'/platform/'sources.json'
    return json.loads(path.read_text())


def source_for(entry):
    path = (YROOT/entry['source']).resolve()
    if not path.is_relative_to(YROOT.resolve()):
        raise ValueError('source escapes YOS: ' + entry['source'])
    return path


def materialize(platform, output):
    entries = files_for(platform)
    output = output.resolve()
    def target_for(name):
        target = (output/name).resolve()
        if not target.is_relative_to(output) or target == output:
            raise ValueError('output escapes staging directory: ' + name)
        return target
    # Validate every input before changing the caller's generated tree.
    staged = {}
    for name, entry in entries.items():
        target = target_for(name)
        data = source_for(entry).read_bytes()
        if 'areas' in entry:
            text = data.decode()
            areas = entry['areas']
            if len(AREA.findall(text)) != len(areas):
                raise ValueError('section layout changed: ' + name)
            if any(not re.fullmatch(r'_(?:CODE|KEXT|EXTRA|ROM_\w+|HEADER_DATA)', area)
                   for area in areas):
                raise ValueError('invalid section name: ' + name)
            replacements = iter(areas)
            data = AREA.sub(lambda match: match[1]+next(replacements), text).encode()
        staged[target] = data
    index = output/'.yos-sources.json'
    previous = json.loads(index.read_text()) if index.exists() else []
    stale = [target_for(name) for name in previous if name not in entries]
    for target, data in staged.items():
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != data:
            target.write_bytes(data)
    for target in stale:
        target.unlink(missing_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    inventory = json.dumps(sorted(entries), indent=2)+'\n'
    if not index.exists() or index.read_text() != inventory:
        index.write_text(inventory)
    return entries


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('platform', choices=('next', 'partner', 'cpc'))
    p.add_argument('--output', type=Path)
    p.add_argument('--list', action='store_true')
    p.add_argument('--inputs', action='store_true')
    a = p.parse_args()
    if a.list:
        print(' '.join(files_for(a.platform)))
    elif a.inputs:
        print(' '.join(str(source_for(e)) for e in files_for(a.platform).values()))
    elif a.output:
        materialize(a.platform, a.output)
    else:
        p.error('choose --output, --list, or --inputs')


if __name__ == '__main__':
    main()
