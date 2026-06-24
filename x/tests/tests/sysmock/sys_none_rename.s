        ;; sys_none_rename.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_rename
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_rename
        .globl  __sys_none_find_mount
        .globl  __sys_none_search_name
        .globl  __sys_none_tmp_mount
        .globl  __sys_none_tmp_ptr

        .area   _CODE
__sys_none_rename:
        ld      a,h
        or      l
        jr      z,__sys_none_rename_fail
        ld      a,d
        or      e
        jr      z,__sys_none_rename_fail
        ld      (__sys_none_tmp_ptr),de
        ld      (__sys_none_search_name),hl
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      z,__sys_none_rename_fail
        ld      (__sys_none_tmp_mount),hl
        ld      hl,(__sys_none_tmp_ptr)
        call    __sys_none_find_mount
        ld      a,h
        or      l
        jr      z,__sys_none_rename_new_free
        jr      __sys_none_rename_fail
__sys_none_rename_new_free:
        ld      hl,(__sys_none_tmp_mount)
        ld      de,(__sys_none_tmp_ptr)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      de,#0x0000
        ret
__sys_none_rename_fail:
        ld      de,#0xffff
        ret

