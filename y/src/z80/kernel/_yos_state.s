        ; Shared YOS syscall-table storage.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module _yos_state
        ; The immutable ABI table is published directly from ROM by
        ; _syscall_table_init.s, so no writable mirror is allocated.
