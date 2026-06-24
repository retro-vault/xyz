        ;; stdio_cases_dispatch.s
        ;;
        ;; Hand-written top-level dispatcher for the stdio integration test.
        ;; The individual subcases are already validated on their own; this
        ;; wrapper just sequences them and preserves the expected stage tags.

        .module stdio_cases_dispatch
        .optsdcc -mz80 sdcccall(1)

        .globl  _stdio_cases
        .globl  __stdio_stdin_handle
        .globl  __stdio_stdout_handle
        .globl  __stdio_stderr_handle
        .globl  _stdio_format_cases
        .globl  _stdio_console_input_cases
        .globl  _stdio_file_cases
        .globl  _stdio_misc_cases

        .area   _DATA
__stdio_cases_in:
        .dw     0
__stdio_cases_out:
        .dw     0

        .area   _CODE

_stdio_cases::
        call    __stdio_stdin_handle
        ld      (__stdio_cases_in),de
        call    __stdio_stdout_handle
        ld      (__stdio_cases_out),de
        call    __stdio_stderr_handle
        ld      hl,(__stdio_cases_out)
        call    _stdio_format_cases
        ld      a,d
        or      e
        jr      z,stdio_cases_console
        ld      hl,#100
        add     hl,de
        ex      de,hl
        jr      stdio_cases_done

stdio_cases_console:
        ld      hl,(__stdio_cases_in)
        call    _stdio_console_input_cases
        ld      a,d
        or      e
        jr      z,stdio_cases_file
        ld      hl,#200
        add     hl,de
        ex      de,hl
        jr      stdio_cases_done

stdio_cases_file:
        call    _stdio_file_cases
        ld      a,d
        or      e
        jr      z,stdio_cases_misc
        ld      hl,#300
        add     hl,de
        ex      de,hl
        jr      stdio_cases_done

stdio_cases_misc:
        ld      hl,(__stdio_cases_out)
        call    _stdio_misc_cases
        ld      a,d
        or      e
        jr      z,stdio_cases_ok
        ld      hl,#400
        add     hl,de
        ex      de,hl
        jr      stdio_cases_done

stdio_cases_ok:
        ld      de,#0x0000

stdio_cases_done:
        ret
