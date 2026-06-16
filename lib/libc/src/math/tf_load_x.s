        ;; tf_load_x.s
        ;; Split from transf_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module tf_load_x
        .optsdcc -mz80 sdcccall(1)

        .globl  __tf_leave
        .globl  __tf_load_u
        .globl  __tf_load_x
        .globl  __tf_ret_nan
        .globl  __tf_ret_pinf

TF_UHI  .equ -16
TF_ULO  .equ -18
TF_XHI  .equ -24
TF_XLO  .equ -26

        .area   _CODE
__tf_load_x::
        ld      e,TF_XLO(ix)
        ld      d,TF_XLO + 1(ix)
        ld      l,TF_XHI(ix)
        ld      h,TF_XHI + 1(ix)
        ret

__tf_load_u::
        ld      e,TF_ULO(ix)
        ld      d,TF_ULO + 1(ix)
        ld      l,TF_UHI(ix)
        ld      h,TF_UHI + 1(ix)
        ret

__tf_ret_nan::
        ld      hl,#0x7fc0
        ld      de,#0x0000
        jp      __tf_leave

__tf_ret_pinf::
        ld      hl,#0x7f80
        ld      de,#0x0000
        jp      __tf_leave

__tf_leave::
        ld      sp,ix
        pop     ix
        ret

        ;; __libc_expf_core
        ;; input:  HL:DE = x
        ;; output: HL:DE = expf(x)
