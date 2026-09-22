        ; Safely enumerate physical esxDOS devices into a bounded array.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module enumerate_disks
        .optsdcc -mz80 sdcccall(1)

        .globl  _enumerate_disks
        .globl  __zx_esx_buffer
        .globl  __zx_esx_errno
        .globl  __zx_esx_disk_info

        .globl  __frame_ix
        .globl  __frame_return

        .area   _CODE

        ; inputs: HL = six-byte result array, DE = capacity (0..255).
        ; output: DE = number of records, or -1 with errno set.
_enumerate_disks::
        ld      a,d
        or      a
        jr      nz,.invalid
        ld      a,e
        or      a
        jr      z,.empty
        push    hl
        ld      b,#0
        ld      c,e
        ld      h,#0
        ld      l,e
        add     hl,hl                   ; capacity * 2
        add     hl,bc                   ; capacity * 3
        add     hl,hl                   ; capacity * 6
        ld      b,h
        ld      c,l
        pop     hl
        call    __zx_esx_buffer
        jp      c,__zx_esx_errno

        call    __frame_ix
        push    hl                      ; IX-2: next output
        push    de                      ; IX-4: remaining capacity
        ld      bc,#0x0100              ; B = device, C = result count
        push    bc                      ; IX-6
.scan:
        ld      a,-5(ix)
        ld      l,-2(ix)
        ld      h,-1(ix)
        call    __zx_esx_disk_info
        jr      c,.next
        inc     -6(ix)
        dec     -4(ix)
        jr      z,.done
        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      de,#6
        add     hl,de
        ld      -2(ix),l
        ld      -1(ix),h
.next:
        inc     -5(ix)
        jr      nz,.scan
.done:
        ld      e,-6(ix)
        ld      d,#0
        jp      __frame_return
.invalid:
        ld      a,#22                   ; EINVAL
        jp      __zx_esx_errno
.empty:
        ld      de,#0
        ret
