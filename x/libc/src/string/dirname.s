        ;; dirname.s
        ;; Split from strcoll.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module dirname
        .optsdcc -mz80 sdcccall(1)

        .globl  _dirname

        .area   _CODE
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
