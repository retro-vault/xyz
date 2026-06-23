        ;; sys_none_tmp_buf.s
        ;; Split from fileio.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module sys_none_tmp_buf
        .optsdcc -mz80 sdcccall(1)

        .globl  __sys_none_tmp_buf
        .globl  __sys_none_tmp_len

        .area   _DATA
__sys_none_tmp_buf::
        .dw     0
__sys_none_tmp_len::
        .dw     0
