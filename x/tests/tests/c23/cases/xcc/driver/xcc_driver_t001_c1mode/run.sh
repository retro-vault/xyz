#!/usr/bin/env bash
set -euo pipefail

XCC="${1:?usage: run.sh /path/to/xcc workdir}"
WORKDIR="${2:?usage: run.sh /path/to/xcc workdir}"

mkdir -p "$WORKDIR"

ASM_FILE="$WORKDIR/c1mode.s"
STDOUT_FILE="$WORKDIR/c1mode.stdout.s"
BAD_STDERR="$WORKDIR/c1mode.bad.stderr"
SOURCE_HINT="$WORKDIR/input_hint.c"

cat >"$SOURCE_HINT" <<'EOF'
int main(void)
{
    return 99;
}
EOF

PREPROCESSED_INPUT=$'# 1 "input_hint.c"\nint main(void)\n{\n    return 0;\n}\n'

printf '%s' "$PREPROCESSED_INPUT" \
    | "$XCC" --c1mode -mz80 --opt-code-size "$SOURCE_HINT" -o "$ASM_FILE"

grep -q 'c1-mode' "$ASM_FILE"
grep -Eq '(^|[[:space:]])_main:|(^|[[:space:]])main:' "$ASM_FILE"

printf '%s' "$PREPROCESSED_INPUT" \
    | "$XCC" -c1-mode --opt-code-size >"$STDOUT_FILE"

grep -q 'c1-mode' "$STDOUT_FILE"
grep -Eq '(^|[[:space:]])_main:|(^|[[:space:]])main:' "$STDOUT_FILE"

if printf '%s' "$PREPROCESSED_INPUT" \
    | "$XCC" --c1mode extra.rel -o "$WORKDIR/should-not-exist.s" \
        > /dev/null 2>"$BAD_STDERR"; then
    echo "expected --c1mode with non-C positional input to fail" >&2
    exit 1
fi

grep -q 'incompatible with --c1mode' "$BAD_STDERR"
