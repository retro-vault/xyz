#!/usr/bin/env bash
set -euo pipefail

CASE_ID="${1:?usage: c1mode_case.sh case-id /path/to/xcc workdir}"
XCC="${2:?usage: c1mode_case.sh case-id /path/to/xcc workdir}"
WORKDIR="${3:?usage: c1mode_case.sh case-id /path/to/xcc workdir}"

mkdir -p "$WORKDIR"

ASM_FILE="$WORKDIR/$CASE_ID.s"
STDOUT_FILE="$WORKDIR/$CASE_ID.stdout.s"
STDERR_FILE="$WORKDIR/$CASE_ID.stderr"
ADB_FILE="$WORKDIR/$CASE_ID.adb"
SOURCE_HINT="$WORKDIR/input_hint.c"
EXTRA_HINT="$WORKDIR/extra_hint.c"

write_source_hints() {
    cat >"$SOURCE_HINT" <<'EOF'
int main(void)
{
    return 1;
}
EOF

    cat >"$EXTRA_HINT" <<'EOF'
int helper(void)
{
    return 2;
}
EOF
}

good_input() {
    cat <<'EOF'
# 1 "input_hint.c"
static int helper(int x)
{
    return x + 1;
}

int main(void)
{
    return helper(41);
}
EOF
}

bad_input() {
    cat <<'EOF'
# 1 "broken_input.c"
int main(void)
{
    return 0 + ;
}
EOF
}

assert_c1_asm() {
    local path="${1:?missing asm path}"
    grep -q 'c1-mode' "$path"
    grep -Eq '(^|[[:space:]])_main:|(^|[[:space:]])main:' "$path"
}

assert_error_for_input() {
    local logical_input="${1:?missing logical input name}"
    grep -Eq "xcc: [0-9]+ error\\(s\\) in '$logical_input'" "$STDERR_FILE"
}

assert_help_contains() {
    local path="${1:?missing help output path}"
    grep -Fq -- '--c1mode' "$path"
    grep -Fq -- '-c1-mode' "$path"
    grep -Fq -- '--opt-code-size' "$path"
    grep -Fq -- '--opt-code-speed' "$path"
    grep -Fq -- '-mz80' "$path"
    grep -Fq -- '--nostdinc' "$path"
}

write_source_hints

case "$CASE_ID" in
    xcc_driver_t002_c1mode_default_stdout)
        good_input | "$XCC" --c1mode >"$STDOUT_FILE"
        assert_c1_asm "$STDOUT_FILE"
        ;;

    xcc_driver_t003_c1mode_verbose_extra_hint)
        good_input \
            | "$XCC" --c1mode -v "$SOURCE_HINT" "$EXTRA_HINT" -o "$ASM_FILE" \
                > /dev/null 2>"$STDERR_FILE"
        assert_c1_asm "$ASM_FILE"
        grep -q 'ignoring extra c1-mode source hint' "$STDERR_FILE"
        ;;

    xcc_driver_t004_c1mode_parse_error_stdin_name)
        if bad_input | "$XCC" --c1mode > /dev/null 2>"$STDERR_FILE"; then
            echo "expected c1-mode parse failure for malformed stdin" >&2
            exit 1
        fi
        assert_error_for_input '<stdin>'
        ;;

    xcc_driver_t005_c1mode_parse_error_source_hint)
        if bad_input | "$XCC" --c1mode "$SOURCE_HINT" > /dev/null 2>"$STDERR_FILE"; then
            echo "expected c1-mode parse failure for malformed hinted stdin" >&2
            exit 1
        fi
        assert_error_for_input "$SOURCE_HINT"
        ;;

    xcc_driver_t006_c1mode_opt_code_size_compat)
        good_input \
            | "$XCC" --c1mode --opt-code-size -mz80 --std-sdcc11 --nostdinc \
                "$SOURCE_HINT" -o "$ASM_FILE"
        assert_c1_asm "$ASM_FILE"
        ;;

    xcc_driver_t007_c1mode_opt_code_speed_compat)
        good_input \
            | "$XCC" --c1mode --opt-code-speed --std-c99 -o "$ASM_FILE"
        assert_c1_asm "$ASM_FILE"
        ;;

    xcc_driver_t008_c1mode_debug_sidecar)
        good_input \
            | "$XCC" --c1mode -g "$SOURCE_HINT" -o "$ASM_FILE"
        assert_c1_asm "$ASM_FILE"
        test -s "$ADB_FILE"
        ;;

    xcc_driver_t009_c1mode_long_alias_stdout)
        good_input | "$XCC" --c1-mode -o - >"$STDOUT_FILE"
        assert_c1_asm "$STDOUT_FILE"
        ;;

    xcc_driver_t010_c1mode_gnuas_dialect)
        good_input | "$XCC" --c1mode -masm gnuas -o "$ASM_FILE"
        assert_c1_asm "$ASM_FILE"
        grep -q $'\t.text' "$ASM_FILE"
        if grep -q '\.module xcc_output' "$ASM_FILE"; then
            echo "GNU as output should not include .module header" >&2
            exit 1
        fi
        ;;

    xcc_driver_t011_c1mode_help_usage)
        "$XCC" --help >"$STDOUT_FILE" 2>"$STDERR_FILE"
        test ! -s "$STDOUT_FILE"
        assert_help_contains "$STDERR_FILE"
        ;;

    *)
        echo "unknown c1-mode driver test case: $CASE_ID" >&2
        exit 1
        ;;
esac
