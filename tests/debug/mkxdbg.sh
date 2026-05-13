#!/bin/sh

set -eu

if [ "$#" -ne 5 ]; then
    echo "usage: $0 <noi> <image> <xdbg> <source_c> <source_s>" >&2
    exit 1
fi

noi_file=$1
image_file=$2
xdbg_file=$3
source_c=$4
source_s=$5
adb_file=$(dirname "$noi_file")/sieve.adb

addr_of() {
    awk -v symbol_name="$1" '$1 == "DEF" && $2 == symbol_name { print $3; exit }' "$noi_file"
}

require_addr() {
    value=$(addr_of "$1")
    if [ -z "$value" ]; then
        echo "missing symbol in NOI: $1" >&2
        exit 1
    fi
    printf '%s' "$value"
}

entry_addr=$(require_addr _entry)
main_addr=$(require_addr _main)
sieve_addr=$(require_addr _sieve)
flags_addr=$(require_addr _flags)
prime_count_addr=$(require_addr _prime_count)
last_prime_addr=$(require_addr _last_prime)

local_records=$(
    while IFS= read -r record; do
        name=$(printf '%s\n' "$record" | sed -n 's/^S:Lsieve\.sieve$\([^$]*\)\$.*\[[A-Za-z][A-Za-z]*\]$/\1/p')
        reg=$(printf '%s\n' "$record" | sed -n 's/^S:Lsieve\.sieve\$[^$]*\$.*\[\([A-Za-z][A-Za-z]*\)\]$/\1/p')
        if [ -z "$name" ] || [ -z "$reg" ]; then
            continue
        fi
        decl_line=$(grep -n "unsigned char $name;" "$source_c" | cut -d: -f1 | head -n1)
        if [ -z "$decl_line" ]; then
            decl_line=0
        fi
        printf 'variable name="%s" kind=local parent="_sieve" storage=register_name register="%s" type="unsigned char" start=%s end=%s file=1 line=%s column=5 language=c\n' \
            "$name" "$reg" "$sieve_addr" "$main_addr" "$decl_line"
    done < "$adb_file"
)

line_records=$(
    awk '
        $1 == "DEF" && index($2, "C$sieve.c$") == 1 {
            split($2, parts, /\$/)
            if (parts[3] == "7" || parts[3] == "39") {
                next
            }
            printf("line address=%s file=1 line=%s column=1\n", $3, parts[3])
        }
    ' "$noi_file"
)

crt0_line_records=$(cat <<EOF
line address=$entry_addr file=2 line=8 column=9
line address=$((entry_addr + 0x3)) file=2 line=11 column=9
line address=$((entry_addr + 0x4)) file=2 line=12 column=9
EOF
)

cat > "$xdbg_file" <<EOF
xdbg 1
image path="$image_file"
entry address=$entry_addr
file id=1 path="$source_c" language=c
file id=2 path="$source_s" language=assembly
symbol name="_entry" kind=function address=$entry_addr file=2 line=7 language=assembly
symbol name="_main" kind=function address=$main_addr file=1 line=39 language=c
symbol name="_sieve" kind=function address=$sieve_addr file=1 line=7 language=c
symbol name="_flags" kind=object address=$flags_addr size=0x41 file=1 line=3 language=c
symbol name="_prime_count" kind=object address=$prime_count_addr size=0x1 file=1 line=4 language=c
symbol name="_last_prime" kind=object address=$last_prime_addr size=0x1 file=1 line=5 language=c
function name="_entry" start=$entry_addr end=$sieve_addr file=2 line=7 language=assembly
function name="_main" start=$main_addr end=$flags_addr file=1 line=39 return_type="int" language=c
function name="_sieve" start=$sieve_addr end=$main_addr file=1 line=7 return_type="void" language=c
$local_records
$crt0_line_records
$line_records
EOF
