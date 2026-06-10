        ;; db1argf_common.s
        ;;
        ;; Shared loader for one-argument double/long double wrappers that
        ;; convert the stacked 64-bit argument into the single-precision float
        ;; register ABI before calling a float kernel.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module db1argf_common
        .optsdcc -mz80 sdcccall(1)

        .globl  __db1arg_load_arg0_fs
        .globl  ___db2fs

        .area   _CODE

__db1arg_load_arg0_fs:
        ld      a,4(ix)
        ld      e,a
        ld      a,5(ix)
        ld      d,a
        ld      a,6(ix)
        ld      l,a
        ld      a,7(ix)
        ld      h,a
        exx
        ld      a,8(ix)
        ld      e,a
        ld      a,9(ix)
        ld      d,a
        ld      a,10(ix)
        ld      l,a
        ld      a,11(ix)
        ld      h,a
        exx
        jp      ___db2fs
