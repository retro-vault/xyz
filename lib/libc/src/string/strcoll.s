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
        .globl  _strverscmp
        .globl  _basename
        .globl  _dirname
        .globl  _strcmp

        .area   _CODE
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

