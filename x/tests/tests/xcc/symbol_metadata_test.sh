#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
XCC="${XCC:-$ROOT/x/bin/x/bin/xcc}"
XAS="${XAS:-$ROOT/x/bin/x/bin/xas}"

if [[ ! -x "$XCC" && -x "$ROOT/bin/x/bin/xcc" ]]; then
    XCC="$ROOT/bin/x/bin/xcc"
fi
if [[ ! -x "$XAS" && -x "$ROOT/bin/x/bin/xas" ]]; then
    XAS="$ROOT/bin/x/bin/xas"
fi

command -v readelf >/dev/null 2>&1 || {
    echo "skip: readelf not available"
    exit 0
}

tmpdir="$(mktemp -d /tmp/xcc-symbol-metadata.XXXXXX)"
trap 'rm -rf "$tmpdir"' EXIT

cat > "$tmpdir/symbols.c" <<'SRC'
int global_counter = 7;
char global_buf[3];
static int hidden_word = 9;
_Thread_local char tls_flag;

int add_one(int v) {
    return v + global_counter + hidden_word;
}

const char *message(void) {
    return "ok";
}
SRC

"$XCC" -S --mode=gnu -masm=gnuas "$tmpdir/symbols.c" -o "$tmpdir/symbols.s"

grep -q $'\t.type _add_one, @function' "$tmpdir/symbols.s"
grep -q $'\t.size _add_one, . - _add_one' "$tmpdir/symbols.s"
grep -q $'\t.type _global_counter, @object' "$tmpdir/symbols.s"
grep -q $'\t.size _global_buf, 3' "$tmpdir/symbols.s"
grep -q $'\t.type __xcc_str_0, @object' "$tmpdir/symbols.s"
grep -q $'\t.size __xcc_str_0, 3' "$tmpdir/symbols.s"

"$XAS" --mode=gnu "$tmpdir/symbols.s" -o "$tmpdir/symbols.o"
readelf -s "$tmpdir/symbols.o" > "$tmpdir/symbols.sym"

check_sym() {
    local name="$1"
    local size="$2"
    local type="$3"
    local bind="$4"
    awk -v name="$name" -v size="$size" -v type="$type" -v bind="$bind" '
        $8 == name {
            count++
            if ($3 == size && $4 == type && $5 == bind && $7 != "UND")
                ok = 1
        }
        END { exit !(count == 1 && ok) }
    ' "$tmpdir/symbols.sym"
}

check_func() {
    local name="$1"
    awk -v name="$name" '
        $8 == name {
            count++
            if ($3 > 0 && $4 == "FUNC" && $5 == "GLOBAL" && $7 != "UND")
                ok = 1
        }
        END { exit !(count == 1 && ok) }
    ' "$tmpdir/symbols.sym"
}

check_sym _global_counter 2 OBJECT GLOBAL
check_sym _global_buf 3 OBJECT GLOBAL
check_sym _hidden_word 2 OBJECT LOCAL
check_sym __xcc_str_0 3 OBJECT LOCAL
check_sym __tls_template 1 OBJECT GLOBAL
check_func _add_one
check_func _message

echo "xcc symbol metadata test: ok"
