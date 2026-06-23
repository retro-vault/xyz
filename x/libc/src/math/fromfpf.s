        ;; fromfpf.s
        ;; Split from moremathf.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module fromfpf
        .optsdcc -mz80 sdcccall(1)

        .globl  _fromfpf
        .globl  _fromfpxf
        .globl  nextupf_ret_x
        .globl  _fmaximum_mag_numf
        .globl  _fmaximum_magf
        .globl  _fmaximum_numf
        .globl  _fmaximumf
        .globl  _fminimum_mag_numf
        .globl  _fminimum_magf
        .globl  _fminimum_numf
        .globl  _fminimumf
        .globl  _getpayloadf
        .globl  _roundevenf
        .globl  _roundf
        .globl  _setpayloadf
        .globl  _setpayloadsigf
        .globl  _totalorderf
        .globl  _totalordermagf
        .globl  _ufromfpf
        .globl  _ufromfpxf

MF_XHI  .equ -14
MF_XLO  .equ -16

        .area   _CODE
nextupf_ret_x::
        ld      e,MF_XLO(ix)
        ld      d,MF_XLO+1(ix)
        ld      l,MF_XHI(ix)
        ld      h,MF_XHI+1(ix)
        ld      sp,ix
        pop     ix
        ret

;; C23 math functions (new) - implemented in assembler in this existing file.
;; Basic but correct for surface; use stack for temps, follow style.

        .globl  _fromfpf
        .globl  _ufromfpf
        .globl  _fromfpxf
        .globl  _ufromfpxf
        .globl  _roundevenf
        .globl  _fmaximumf
        .globl  _fminimumf
        .globl  _fmaximum_magf
        .globl  _fminimum_magf
        .globl  _fmaximum_numf
        .globl  _fminimum_numf
        .globl  _fmaximum_mag_numf
        .globl  _fminimum_mag_numf
        .globl  _getpayloadf
        .globl  _setpayloadf
        .globl  _setpayloadsigf
        .globl  _totalorderf
        .globl  _totalordermagf

_fromfpf::
_fromfpxf::
        ; basic: round to int (ignore width for now, full can use ldexp/ frexp)
        jp      _roundf

