        ; Load the initial disk-resident YOS process from the current drive.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module boot_shell
        .optsdcc -mz80 sdcccall(1)

        .globl  _boot_shell
        .globl  _process_load

        .area   _HEADER_DATA

_boot_shell::
        ld      hl,#.shell
        jp      _process_load

        ; Keep the complete immutable name with its tail-jumping caller.
        ; This packs ordinary ROM, not a firmware reservation.
        .area _CODE
.shell:
        .if YOS_BOOT_ALTO
        .if YOS_BOOT_DIVMMC
        .asciz  "/ALTO/SYS/start.sys"
        .else
        .asciz  "/ALTO/SYS/shell.sys"
        .endif
        .else
        .asciz  "shell.sys"
        .endif
