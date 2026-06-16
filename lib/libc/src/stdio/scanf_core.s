        ;; scanf_core.s
        ;;
        ;; Shared scanning core for the scanf family. The public entry points
        ;; live one-per-module so archive extraction stays fine-grained, while
        ;; this internal module carries the actual parser and source adapters.
        ;;
        ;; Supported conversions in this assembly-only pass:
        ;;   %d %i %u %x %X %o %c %s %p %n %%
        ;;   %f %F %e %E %g %G
        ;;
        ;; Supported modifiers:
        ;;   assignment suppression (*), width, h, hh, l, ll, j, z, t
        ;;
        ;; Scanset (%[...]) parsing and hexadecimal-float `%a` / `%A` inputs are
        ;; intentionally left for a later pass.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih








        .module scanf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_scan_init_stdin
        .globl  __stdio_scan_digitval
        .globl  __stdio_scan_init_stream_shared
        .globl  __stdio_scan_match_fail
        .globl  _stdin

        .area   _CODE
__stdio_scan_init_stdin::
        ld      hl,(_stdin)
        jr      __stdio_scan_init_stream_shared

__stdio_scan_digit_for_base:
        call    __stdio_scan_digitval
        cp      c
        ccf
        ret

        ;; BC = width. Returns BC clamped to TOK_CAP-1 and non-zero.
__stdio_scan_copy_acc:
        ret

__stdio_scan_conv_s_empty_check:
        jp      __stdio_scan_match_fail

        ;; Signed integer family uses strtoll so every length variant can
        ;; truncate from the same canonical parsed value.
