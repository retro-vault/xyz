        ;; nextdown.s
        ;; Split from moremathd.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module nextdown
        .optsdcc -mz80 sdcccall(1)

        .globl  _fromfp
        .globl  _fromfpl
        .globl  _fromfpx
        .globl  _fromfpxl
        .globl  _nextdown
        .globl  _nextdownl
        .globl  ___fs2db
        .globl  __db_load_arg0_fs
        .globl  _fmaximum
        .globl  _fmaximum_mag
        .globl  _fmaximum_mag_num
        .globl  _fmaximum_mag_numl
        .globl  _fmaximum_magl
        .globl  _fmaximum_num
        .globl  _fmaximum_numl
        .globl  _fmaximuml
        .globl  _fminimum
        .globl  _fminimum_mag
        .globl  _fminimum_mag_num
        .globl  _fminimum_mag_numl
        .globl  _fminimum_magl
        .globl  _fminimum_num
        .globl  _fminimum_numl
        .globl  _fminimuml
        .globl  _getpayload
        .globl  _getpayloadl
        .globl  _roundeven
        .globl  _roundevenl
        .globl  _setpayload
        .globl  _setpayloadl
        .globl  _setpayloadsig
        .globl  _setpayloadsigl
        .globl  _totalorder
        .globl  _totalorderl
        .globl  _totalordermag
        .globl  _totalordermagl
        .globl  _ufromfp
        .globl  _ufromfpl
        .globl  _ufromfpx
        .globl  _ufromfpxl

        .area   _CODE
_nextdown::
_nextdownl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _nextdownf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

;; C23 double math (new) - thin wrappers calling f versions after convert (consistent with file style, stack frame).

        .globl  _fromfp
        .globl  _fromfpl
        .globl  _ufromfp
        .globl  _ufromfpl
        .globl  _fromfpx
        .globl  _fromfpxl
        .globl  _ufromfpx
        .globl  _ufromfpxl
        .globl  _roundeven
        .globl  _roundevenl
        .globl  _fmaximum
        .globl  _fmaximuml
        .globl  _fminimum
        .globl  _fminimuml
        .globl  _fmaximum_mag
        .globl  _fmaximum_magl
        .globl  _fminimum_mag
        .globl  _fminimum_magl
        .globl  _fmaximum_num
        .globl  _fmaximum_numl
        .globl  _fminimum_num
        .globl  _fminimum_numl
        .globl  _fmaximum_mag_num
        .globl  _fmaximum_mag_numl
        .globl  _fminimum_mag_num
        .globl  _fminimum_mag_numl
        .globl  _getpayload
        .globl  _getpayloadl
        .globl  _setpayload
        .globl  _setpayloadl
        .globl  _setpayloadsig
        .globl  _setpayloadsigl
        .globl  _totalorder
        .globl  _totalorderl
        .globl  _totalordermag
        .globl  _totalordermagl

_fromfp::
_fromfpl::
_fromfpx::
_fromfpxl::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-12
        add     hl,sp
        ld      sp,hl
        call    __db_load_arg0_fs
        call    _fromfpf
        call    ___fs2db
        ld      sp,ix
        pop     ix
        ret

