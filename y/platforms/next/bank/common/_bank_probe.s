        ; Size the process-bank pool of the Next.
        ;
        ; Logical banks 0..15 are physical banks 16..31, logical 16.. are
        ; physical 48.. (banks 32..47 hold the embedded files, banks 0..15
        ; below that belong to YOS and NextZXOS). Every Next has 1 MB, i.e.
        ; physical banks 0..63, so 32 logical banks always exist. Banks 64..111
        ; exist only on a 2 MB Next; on a 1 MB machine they alias banks 0..47,
        ; so they are real when the first bytes of bank 64 differ from bank 0
        ; (which holds the boot code). Nothing is written to probe.
        ;
        ; outputs: A = ROM model (2), B = logical bank count (32 or 80),
        ; HL = bank mapper.
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih
        .module next_bank_probe
        .optsdcc -mz80 sdcccall(1)
        .globl __bank_probe, __bank_map_next

        .area _BSS
.probe_buf:     .ds 16
.probe_save:    .ds 2

        .area _CODE
__bank_probe::
        ; remember the pages at $C000
        ld      bc,#0x243b
        ld      a,#0x56
        out     (c),a
        inc     b
        in      a,(c)
        ld      (.probe_save),a
        dec     b
        ld      a,#0x57
        out     (c),a
        inc     b
        in      a,(c)
        ld      (.probe_save+1),a
        ; copy 16 bytes of bank 64
        ld      d,#128
        call    .probe_map
        ld      hl,#0xc000
        ld      de,#.probe_buf
        ld      bc,#16
        ldir
        ; compare with bank 0
        ld      d,#0
        call    .probe_map
        ld      hl,#0xc000
        ld      de,#.probe_buf
        ld      b,#16
        ld      c,#0                    ; C = bits that differ
.probe_cmp:
        ld      a,(de)
        xor     (hl)
        or      c
        ld      c,a
        inc     hl
        inc     de
        djnz    .probe_cmp
        ; restore $C000
        ld      a,(.probe_save)
        ld      e,a
        ld      d,#0x56
        call    .probe_nr
        ld      a,(.probe_save+1)
        ld      e,a
        ld      d,#0x57
        call    .probe_nr
        ld      b,#32
        ld      a,c
        or      a
        jr      z,.probe_done           ; identical: bank 64 aliases bank 0
        ld      b,#80
.probe_done:
        ld      a,#2
        ld      hl,#__bank_map_next
        ret
        ; D = first 8K page: map pages D, D+1 at $C000.
.probe_map:
        ld      e,d
        ld      d,#0x56
        call    .probe_nr
        ld      a,e
        inc     a
        ld      e,a
        ld      d,#0x57
.probe_nr:
        push    bc
        ld      bc,#0x243b
        out     (c),d
        inc     b
        out     (c),e
        pop     bc
        ret
