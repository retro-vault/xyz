        ; ieee16_modf.s
        .module ieee16_modf
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_modf
        .globl  ___fh2fs
        .globl  ___fs2fh
        .globl  _modff
        .globl  ___ieee16_store_half_ptr_de

TMP0    .equ    -10
TMP1    .equ    -9
TMP2    .equ    -8
TMP3    .equ    -7
OUT0    .equ    -2
OUT1    .equ    -1

        .area   _CODE
_ieee16_modf::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-10
        add     hl,sp
        ld      sp,hl
        call    ___fh2fs
        ld      TMP0(ix),e
        ld      TMP1(ix),d
        ld      TMP2(ix),l
        ld      TMP3(ix),h
        push    ix
        pop     hl
        ld      bc,#TMP0
        add     hl,bc
        push    hl
        ld      e,TMP0(ix)
        ld      d,TMP1(ix)
        ld      l,TMP2(ix)
        ld      h,TMP3(ix)
        call    _modff
        pop     bc
        call    ___fs2fh
        ld      OUT0(ix),e
        ld      OUT1(ix),d
        ld      e,TMP0(ix)
        ld      d,TMP1(ix)
        ld      l,TMP2(ix)
        ld      h,TMP3(ix)
        call    ___fs2fh
        ld      l,4(ix)
        ld      h,5(ix)
        call    ___ieee16_store_half_ptr_de
        ld      e,OUT0(ix)
        ld      d,OUT1(ix)
        ld      sp,ix
        pop     ix
        ret
