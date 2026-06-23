        .module numeric_bench_crt0
        .globl  _main
        .globl  __xc_test_start

        .area   _CODE

__xc_test_start::
        ld      sp,#0xf000
        call    _main
        ld      (#0xff00),de
        ld      a,#0xa5
        ld      (#0xff02),a
numeric_bench_halt:
        halt
        jr      numeric_bench_halt
