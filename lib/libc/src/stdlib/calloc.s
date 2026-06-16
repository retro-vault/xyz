        ;; calloc.s
        ;; Split from heap_core.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module calloc
        .optsdcc -mz80 sdcccall(1)

        .globl  _calloc
        .globl  __divuint
        .globl  __mul16
        .globl  _malloc

CALLOC_COUNT_HI  .equ -5
CALLOC_COUNT_LO  .equ -6
CALLOC_SIZE_HI   .equ -3
CALLOC_SIZE_LO   .equ -4
CALLOC_TOTAL_HI  .equ -1
CALLOC_TOTAL_LO  .equ -2

        .area   _CODE
_calloc::
        ld      a,h
        or      l
        jr      z,calloc_zero_request
        ld      a,d
        or      e
        jr      z,calloc_zero_request

        ld      b,h
        ld      c,l                     ; BC = element count
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-6
        add     hl,sp
        ld      sp,hl
        ld      CALLOC_COUNT_LO(ix),c
        ld      CALLOC_COUNT_HI(ix),b
        ld      CALLOC_SIZE_LO(ix),e
        ld      CALLOC_SIZE_HI(ix),d
        ld      h,b
        ld      l,c
        call    __mul16                        ; DE = count * size (mod 65536)
        ld      CALLOC_TOTAL_LO(ix),e
        ld      CALLOC_TOTAL_HI(ix),d
        ex      de,hl                          ; HL = wrapped product
        ld      e,CALLOC_COUNT_LO(ix)
        ld      d,CALLOC_COUNT_HI(ix)
        call    __divuint                      ; DE = product / count
        ld      l,CALLOC_SIZE_LO(ix)
        ld      h,CALLOC_SIZE_HI(ix)
        ld      a,e
        cp      l
        jr      nz,calloc_fail
        ld      a,d
        cp      h
        jr      nz,calloc_fail

        ld      l,CALLOC_TOTAL_LO(ix)
        ld      h,CALLOC_TOTAL_HI(ix)
        call    _malloc
        ld      a,d
        or      e
        jr      z,calloc_return

        push    de
        ex      de,hl                          ; HL = allocated block
        ld      c,CALLOC_TOTAL_LO(ix)
        ld      b,CALLOC_TOTAL_HI(ix)
        ld      a,b
        or      c
        jr      z,calloc_zero_done
calloc_zero_loop:
        xor     a
        ld      (hl),a
        inc     hl
        dec     bc
        ld      a,b
        or      c
        jr      nz,calloc_zero_loop
calloc_zero_done:
        pop     de
calloc_return:
        ld      sp,ix
        pop     ix
        ret

calloc_zero_request:
        ld      hl,#0
        jp      _malloc

calloc_fail:
        ld      de,#0
        jr      calloc_return

;; realloc(ptr, size): shrink in place when possible, otherwise allocate/copy/free.
