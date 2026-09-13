        ; Convert one short-name esxDOS directory record to struct dirent.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _directory_convert
        .optsdcc -mz80 sdcccall(1)

        .globl  __directory_convert

        .area   _CODE

        ; input: HL = native record, DE = struct dirent.
        ; output: DE = struct dirent; preserves IX and IY.
__directory_convert::
        push    iy
        push    de
        pop     iy

        ld      a,(hl)                  ; esxDOS: attributes, name, date, size
        inc     hl
        ld      9(iy),a
        ld      8(iy),#8                ; DT_REG
        bit     4,a
        jr      z,.name
        ld      8(iy),#4                ; DT_DIR
.name:
        xor     a                       ; esxDOS has no inode number
        ld      0(iy),a
        ld      1(iy),a
        ld      2(iy),a
        ld      3(iy),a

        push    iy
        pop     de
        ld      bc,#10
        ex      de,hl
        add     hl,bc
        ex      de,hl                   ; DE = public name, HL = native name
        ld      b,#13
.copy_name:
        ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        or      a
        jr      z,.have_attributes
        djnz    .copy_name
        xor     a                       ; always terminate a malformed record
        dec     de
        ld      (de),a
.have_attributes:
        ld      bc,#4
        add     hl,bc                   ; skip packed date/time
        push    hl
        push    iy
        pop     hl
        add     hl,bc
        ex      de,hl                   ; public d_size
        pop     hl
        ldir
        push    iy
        pop     de
        pop     iy
        ret
