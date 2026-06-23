        ;; libc_tgammaf_core.s
        ;; Split from gammaf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module libc_tgammaf_core
        .optsdcc -mz80 sdcccall(1)

        .globl  __libc_tgammaf_core
        .globl  ___libc_fpclassifyf
        .globl  __gcf_load_x
        .globl  __gcf_ret_nan
        .globl  __gcf_ret_pinf
        .globl  __gcf_tgamma_not_inf

        .area   _CODE
__libc_tgammaf_core::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      c,l
        ld      b,h
        ld      hl,#-21
        add     hl,sp
        ld      sp,hl
        ld      -21(ix),e
        ld      -20(ix),d
        ld      -19(ix),c
        ld      -18(ix),b
        call    __gcf_load_x
        call    ___libc_fpclassifyf
        ld      a,e
        cp      #0                      ; NaN
        jp      z,__gcf_ret_nan
        cp      #1                      ; +/-Inf
        jp      nz,__gcf_tgamma_not_inf
        ld      a,-18(ix)
        and     #0x80
        jp      nz,__gcf_ret_nan
        jp      __gcf_ret_pinf
