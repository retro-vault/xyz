        ; Load the initial disk-resident YOS process from the current drive.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module boot_shell
        .optsdcc -mz80 sdcccall(1)

        .globl  _boot_shell
        .globl  _process_load

        .area   _CODE

_boot_shell::
        ld      hl,#.shell
        jp      _process_load

        .area   _CONST
.shell:
        .asciz  "shell.sys"
