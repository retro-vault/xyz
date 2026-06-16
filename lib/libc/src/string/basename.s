        ;; basename.s
        ;; Split from strcoll.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module basename
        .optsdcc -mz80 sdcccall(1)

        .globl  _basename

        .area   _CODE
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

