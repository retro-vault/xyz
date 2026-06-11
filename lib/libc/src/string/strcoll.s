        ; strcoll.s
        ;
        ; libc strcoll implementation for the xcc Z80 libc.
        ; The current libc is locale-agnostic, so collation falls back to the
        ; plain bytewise strcmp ordering.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module strcoll
        .optsdcc -mz80 sdcccall(1)


        .globl  _strcoll
        .globl  _strcmp

        .area   _CODE

        ; _strcoll
        ; inputs/outputs/clobbers: same as _strcmp
_strcoll::
        jp      _strcmp

;; Additional string extensions (new C23/POSIX/GNU, in existing file only).
;; All assembler, stack safe.

        .globl  _strverscmp
        .globl  _basename
        .globl  _dirname

_strverscmp::
        ; basic version compare, fall to strcmp for now (full would parse numbers)
        jp      _strcmp

_basename::
        ; simple basename: find last / , return after
        push    hl
        ld      d, h
        ld      e, l
bas_loop:
        ld      a, (hl)
        or      a
        jr      z, bas_end
        cp      #'/'
        jr      nz, bas_next
        ld      d, h
        ld      e, l
        inc     de
bas_next:
        inc     hl
        jr      bas_loop
bas_end:
        pop     hl
        ex      de, hl
        ret

_dirname::
        ; simple dirname: find last / , null it or return .
        push    hl
        ld      de, #0
        ld      b, h
        ld      c, l
dir_loop:
        ld      a, (hl)
        or      a
        jr      z, dir_end
        cp      #'/'
        jr      nz, dir_next
        ld      d, h
        ld      e, l
dir_next:
        inc     hl
        jr      dir_loop
dir_end:
        ld      a, d
        or      e
        jr      z, dir_dot
        ld      a, 0
        ld      (de), a
        pop     hl
        ret
dir_dot:
        pop     hl
        ld      hl, #__dir_dot
        ret

__dir_dot:
        .ascii ".\0"
