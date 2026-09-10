        ; Load one XPRG process from esxDOS, relocate it, and schedule it.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module process_load
        .optsdcc -mz80 sdcccall(1)

        .globl  _process_load
        .globl  _process_last_error
        .globl  __yos_malloc
        .globl  __yos_free
        .globl  __crc32
        .globl  __process_read_exact
        .globl  __process_relocate
        .globl  _process_start
        .globl  _open
        .globl  _close

        .equ    YOS_VERSION, 8

        .area   _CODE

        ; input: HL = XPRG pathname.
        ; output: DE = process object or zero; preserves IX and IY.
        ; The 64-byte XPRG descriptor lives temporarily on the caller stack.
_process_load::
        push    ix
        ld      ix,#0
        add     ix,sp
        push    hl                      ; IX-2: path
        ld      hl,#-70
        add     hl,sp
        ld      sp,hl                   ; IX-4 fd, -6 image, -8 code,
                                        ; IX-72..-9 XPRG descriptor
        ld      -3(ix),#0xff
        xor     a
        ld      -6(ix),a
        ld      -5(ix),a
        ld      (_process_last_error),a

        ld      l,-2(ix)
        ld      h,-1(ix)
        ld      de,#0                   ; O_RDONLY
        call    _open
        ld      a,d
        and     e
        inc     a
        jp      z,.not_found
        ld      -4(ix),e
        ld      -3(ix),d

        push    ix
        pop     de
        ld      hl,#-72
        add     hl,de
        ex      de,hl                   ; DE = descriptor
        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      bc,#64
        call    __process_read_exact
        jp      c,.read_error

        ; XPRG v1 process descriptor with no service jump table.
        ld      a,-72(ix)
        cp      #'X'
        jp      nz,.invalid
        ld      a,-71(ix)
        cp      #'P'
        jp      nz,.invalid
        ld      a,-70(ix)
        cp      #'R'
        jp      nz,.invalid
        ld      a,-69(ix)
        cp      #'G'
        jp      nz,.invalid
        ld      a,-68(ix)               ; format version
        dec     a
        jp      nz,.invalid
        ld      a,-67(ix)               ; process, never service
        dec     a
        jp      nz,.wrong_kind
        ld      a,-65(ix)               ; HAS_ENTRY, optional FIXED_LOAD
        and     #0xfe
        cp      #2
        jp      nz,.invalid
        ld      a,-64(ix)               ; metadata size = payload offset = 64
        cp      #64
        jp      nz,.invalid
        ld      a,-63(ix)
        or      -61(ix)
        jp      nz,.invalid
        ld      a,-62(ix)
        cp      #64
        jp      nz,.invalid
        ld      a,-58(ix)               ; 16-bit address space payload
        or      -57(ix)
        jp      nz,.invalid
        ld      a,-44(ix)               ; required process stack
        or      -43(ix)
        jp      z,.invalid
        ld      a,-42(ix)               ; minimum YOS ABI
        cp      #YOS_VERSION+1
        jp      nc,.version_error
        ld      a,-41(ix)
        or      a
        jp      nz,.version_error
        ld      a,-40(ix)               ; processes have no JP table
        or      -39(ix)
        or      -38(ix)
        or      -37(ix)
        jp      nz,.invalid
        ld      l,-60(ix)
        ld      h,-59(ix)
        call    __yos_malloc
        ld      a,d
        or      e
        jp      z,.alloc_error
        ld      -6(ix),e
        ld      -5(ix),d

        ld      l,-4(ix)
        ld      h,-3(ix)
        ld      c,-60(ix)
        ld      b,-59(ix)
        call    __process_read_exact
        jp      c,.read_error

        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      c,-60(ix)
        ld      b,-59(ix)
        call    __crc32
        ld      a,l
        cp      -56(ix)
        jp      nz,.checksum_error
        ld      a,h
        cp      -55(ix)
        jp      nz,.checksum_error
        ld      a,e
        cp      -54(ix)
        jp      nz,.checksum_error
        ld      a,d
        cp      -53(ix)
        jp      nz,.checksum_error

        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      e,-60(ix)
        ld      d,-59(ix)
        call    __process_relocate
        ld      a,d
        or      e
        jp      z,.invalid
        ld      -8(ix),e
        ld      -7(ix),d

        ; The XPRG entry is relative to the XL code, and must lie inside it.
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      bc,#6
        add     hl,bc
        ld      c,(hl)
        inc     hl
        ld      b,(hl)                  ; BC = XL code size
        ld      l,-46(ix)
        ld      h,-45(ix)
        or      a
        sbc     hl,bc
        jp      nc,.invalid
        add     hl,bc
        ld      e,-8(ix)
        ld      d,-7(ix)
        add     hl,de                   ; HL = entry
        jp      c,.invalid
        ex      de,hl

        bit     0,-65(ix)
        jr      z,.start
        ld      a,-8(ix)                ; fixed load refers to XL code base
        cp      -48(ix)
        jp      nz,.invalid
        ld      a,-7(ix)
        cp      -47(ix)
        jp      nz,.invalid
.start:
        push    de                      ; entry
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    _close
        ld      -3(ix),#0xff
        pop     de
        ld      l,-44(ix)
        ld      h,-43(ix)
        ld      bc,#22                  ; scheduler context is private overhead
        add     hl,bc
        jp      c,.invalid
        push    hl
        xor     a
        ld      -25(ix),a               ; process objects retain seven name bytes
        push    ix
        pop     hl
        ld      bc,#-32
        add     hl,bc                   ; HL = process name
        call    _process_start
        ld      a,d
        or      e
        jp      z,.start_error

        ; Heap header precedes payload by seven bytes; owner is at header+2.
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      bc,#-5
        add     hl,bc
        ld      (hl),e
        inc     hl
        ld      (hl),d
        jr      .return

.read_error:
        ld      a,#3
        jr      .fail
.not_found:
        ld      a,#1
        jr      .fail
.alloc_error:
        ld      a,#2
        jr      .fail
.wrong_kind:
        ld      a,#6
        jr      .fail
.version_error:
        ld      a,#7
        jr      .fail
.checksum_error:
        ld      a,#8
        jr      .fail
.invalid:
        ld      a,#4
        jr      .fail
.start_error:
        ld      a,#5
.fail:
        ld      (_process_last_error),a
        ld      a,-3(ix)                ; valid esxDOS descriptors have high byte zero
        inc     a
        jr      z,.free
        ld      l,-4(ix)
        ld      h,-3(ix)
        call    _close
.free:
        ld      l,-6(ix)
        ld      h,-5(ix)
        ld      a,h
        or      l
        jr      z,.null
        call    __yos_free
.null:
        ld      de,#0
.return:
        ld      sp,ix
        pop     ix
        ret
