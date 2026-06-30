        ; ieee16_fpclassify.s
        .module ieee16_fpclassify
        .optsdcc -mz80 sdcccall(1)

        .globl  _ieee16_fpclassify
        .globl  ___ieee16_classify_hl

        .area   _CODE
_ieee16_fpclassify::
        jp      ___ieee16_classify_hl
