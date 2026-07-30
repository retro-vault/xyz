        ; ctime_r.s
        ;
        ; ctime_r() for the xcc Z80 libc, in assembly.  Equivalent to
        ; asctime_r(localtime_r(timer, &tmp), buf); local == UTC.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module ctime_r
        .optsdcc -mz80 sdcccall(1)


        .globl  _ctime_r
        .globl  _localtime_r
        .globl  _asctime_r

        .area   _CODE

        ; _ctime_r
        ; inputs:  HL = const time_t *timer, DE = char *buf
        ; outputs: DE = buf
_ctime_r::
        push    ix
        push    hl                      ; save timer
        push    de                      ; save buf
        ld      ix,#0
        add     ix,sp
        ld      hl,#-18                 ; local struct tm
        add     hl,sp
        ld      sp,hl
        push    ix
        pop     hl
        ld      bc,#-18
        add     hl,bc
        ex      de,hl                   ; DE = &tmp
        ld      l,2(ix)
        ld      h,3(ix)                 ; HL = timer
        call    _localtime_r            ; __ctime_tm = *timer broken down
        push    ix
        pop     hl
        ld      bc,#-18
        add     hl,bc                   ; HL = &tmp
        ld      e,0(ix)
        ld      d,1(ix)                 ; DE = buf
        ld      sp,ix
        pop     bc                      ; discard saved buf
        pop     bc                      ; discard saved timer
        pop     ix
        jp      _asctime_r
