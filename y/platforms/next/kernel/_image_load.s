        ; Shared XPRG disk, validation, CRC and streaming relocation path.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _image_load
        .optsdcc -mz80 sdcccall(1)
        .globl  __image_load
        .globl  __image_busy
        .globl  _process_last_error
        .globl  __bank_allocate
        .globl  __bank_allocate_library
        .globl  __bank_allocate_current
        .globl  __bank_free
        .globl  __bank_map
        .globl  __bank_current
        .globl  __os_malloc
        .globl  __os_free
        .globl  __crc32_reset
        .globl  __crc32_update
        .globl  __crc32_finish
        .globl  __process_read_exact
        .globl  __process_relocate_chunk
        ; Root with the loader helpers to fit before fixed ROM data.
        .globl  _process_has_threads
        .globl  _mem_free_owner
        .globl  __process_load_finish
        .globl  __library_load_finish
        .globl  __library_find
        .globl  __library_acquire
        .globl  __current_process
        .globl  _open
        .globl  _close
        .globl  _enter_critical_section
        .globl  _leave_critical_section

        .equ    YOS_VERSION, 1
        .equ    IMAGE_FD,    64
        .equ    IMAGE_DATA,  66
        .equ    IMAGE_CODE,  68
        .equ    IMAGE_MODE,  70
        .equ    IMAGE_JPS,   72
        .equ    IMAGE_ENTRY, 76
        .equ    IMAGE_OWNER, 78
        .equ    IMAGE_BANK,  82
        .equ    IMAGE_OLD_BANK, 83
        .equ    IMAGE_TABLE, 84
        .equ    IMAGE_RELOC, 86
        .equ    IMAGE_XL,    88
        .equ    IMAGE_CHUNK, 98
        .equ    IMAGE_RELOC_ERROR, 99
        .equ    IMAGE_FRAME_SIZE, 100
        .equ    RELOCATION_CHUNK, 128
        .equ    RELOCATIONS_PER_CHUNK, 32

        .area   _CODE

        ; inputs: hl = path, a = 0 user process / 1 private library /
        ;         3 shared library / 4 system process
        ; outputs: de = process/interface or zero; error cell updated
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: IX+0..63 descriptor; +64..99 loader locals and XL header.
        ; The resident code is read directly into its final allocation. The
        ; trailing XL relocation table is CRC-checked and applied through one
        ; 128-byte scratch block in the same heap as the image.
__image_load::
        push    af
        call    _enter_critical_section
        ld      a, (__image_busy)
        or      a
        jp      nz, .busy
        inc     a
        ld      (__image_busy), a
        call    _leave_critical_section
        pop     af
        push    ix
        push    iy
        ld      ix, #-IMAGE_FRAME_SIZE
        add     ix, sp
        ld      sp, ix
        ld      IMAGE_MODE(ix), a
        ld      IMAGE_FD+1(ix), #0xff
        xor     a
        ld      IMAGE_DATA(ix), a
        ld      IMAGE_DATA+1(ix), a
        ld      IMAGE_TABLE(ix), a
        ld      IMAGE_TABLE+1(ix), a
        ld      IMAGE_RELOC(ix), a
        ld      IMAGE_RELOC+1(ix), a
        dec     a
        ld      IMAGE_BANK(ix), a
        ld      a, (__bank_current)
        ld      IMAGE_OLD_BANK(ix), a
        call    __current_process
        ld      IMAGE_OWNER(ix), c
        ld      IMAGE_OWNER+1(ix), b
        ld      a, IMAGE_MODE(ix)
        and     #1
        jr      z, .open
        ld      a, b
        or      c
        ld      a, #10
        jp      z, .fail
.open:
        ld      de, #0
        call    _open
        ld      a, d
        and     e
        inc     a
        ld      a, #1
        jp      z, .fail
        ld      IMAGE_FD(ix), e
        ld      IMAGE_FD+1(ix), d
        push    ix
        pop     de
        ld      bc, #64
        call    .read
        jp      c, .read_error
        push    ix
        pop     hl
        ld      de, #.magic
        ld      b, #5
.magic_loop:
        ld      a, (de)
        cp      (hl)
        jr      nz, .bad_header
        inc     hl
        inc     de
        djnz    .magic_loop
        ld      a, IMAGE_MODE(ix)
        and     #1
        inc     a
        cp      5(ix)
        ld      a, #6
        jp      nz, .fail
        ld      a, 14(ix)
        or      15(ix)
        jr      nz, .bad_header
        ld      a, 30(ix)
        cp      #YOS_VERSION
        ld      a, #7
        jp      nz, .fail
        ld      a, 31(ix)
        or      a
        ld      a, #7
        jp      nz, .fail
        ld      a, 33(ix)
        or      35(ix)
        jr      nz, .bad_header
        ld      a, IMAGE_MODE(ix)
        and     #1
        jr      nz, .service
        ld      a, 7(ix)
        and     #0xfe
        cp      #2
        jr      nz, .bad_header
        ld      a, 32(ix)
        or      34(ix)
        jr      nz, .bad_header
        ld      a, 28(ix)
        or      29(ix)
        jr      z, .bad_header
        ld      hl, #0
        jr      .metadata
.bad_header:
        jp      .invalid
.service:
        ld      a, 7(ix)
        and     #0xfd
        cp      #4
        jr      nz, .bad_header
        ld      a, 28(ix)
        or      29(ix)
        or      55(ix)
        jr      nz, .bad_header
        ld      a, 40(ix)
        or      a
        jr      z, .bad_header
        ld      a, 32(ix)
        cp      #64
        jr      nz, .bad_header
        ld      l, 34(ix)
        ld      h, #0
        ld      a, l
        or      a
        jr      z, .bad_header
        ld      e, l
        ld      d, h
        add     hl, hl
        add     hl, de
.metadata:
        ld      IMAGE_JPS(ix), l
        ld      IMAGE_JPS+1(ix), h
        ld      de, #64
        add     hl, de
        ld      a, l
        cp      8(ix)
        jr      nz, .bad_header
        cp      10(ix)
        jr      nz, .bad_header
        ld      a, h
        cp      9(ix)
        jr      nz, .bad_header
        cp      11(ix)
        jr      nz, .bad_header

        bit     1, IMAGE_MODE(ix)
        jr      z, .read_body
        call    _enter_critical_section
        call    __library_find
        ld      a, d
        or      e
        jr      z, .missing
        ex      de, hl
        ld      e, IMAGE_OWNER(ix)
        ld      d, IMAGE_OWNER+1(ix)
        call    __library_acquire
        jp      .result
.missing:
        call    _leave_critical_section

.read_body:
        ; Convert the raw JP metadata in place after relocation; this avoids
        ; a second fixed-heap export-table allocation.
        ld      l, IMAGE_JPS(ix)
        ld      h, IMAGE_JPS+1(ix)
        ld      a, h
        or      l
        jr      z, .read_xl_header
        call    __os_malloc
        ld      a, d
        or      e
        ld      a, #2
        jp      z, .fail
        ld      IMAGE_TABLE(ix), e
        ld      IMAGE_TABLE+1(ix), d
        ld      c, IMAGE_JPS(ix)
        ld      b, IMAGE_JPS+1(ix)
        call    .read
        jp      c, .read_error

.read_xl_header:
        push    ix
        pop     de
        ld      hl, #IMAGE_XL
        add     hl, de
        ex      de, hl
        ld      bc, #12
        call    .read
        jp      c, .read_error
        ld      a, IMAGE_XL(ix)
        cp      #'X'
        jp      nz, .invalid
        ld      a, IMAGE_XL+1(ix)
        cp      #'L'
        jp      nz, .invalid
        ld      a, IMAGE_XL+2(ix)
        cp      #2
        jp      nz, .invalid
        ld      a, IMAGE_XL+3(ix)
        or      IMAGE_XL+10(ix)
        or      IMAGE_XL+11(ix)
        jp      nz, .invalid
        ld      l, IMAGE_XL+8(ix)
        ld      h, IMAGE_XL+9(ix)
        add     hl, hl
        jp      c, .invalid
        add     hl, hl
        jp      c, .invalid
        ld      c, IMAGE_XL+6(ix)
        ld      b, IMAGE_XL+7(ix)
        ld      a, b
        or      c
        jp      z, .invalid
        add     hl, bc
        jp      c, .invalid
        ld      bc, #12
        add     hl, bc
        jp      c, .invalid
        ld      a, l
        cp      12(ix)
        jp      nz, .invalid
        ld      a, h
        cp      13(ix)
        jp      nz, .invalid

        ld      l, IMAGE_XL+6(ix)
        ld      h, IMAGE_XL+7(ix)
        bit     2, IMAGE_MODE(ix)
        jr      z, .bank_allocate
        call    __os_malloc
        ld      a, #0xff
        jr      .allocated
.bank_allocate:
        bit     0, IMAGE_MODE(ix)
        jr      z, .process_bank
        call    __bank_allocate_library
        jr      .allocated
.process_bank:
        ; A near-pointer program needs room for its runtime allocations too.
        ; Filling a service's tail with TERM left no space for even opendir.
        ; Give each process an empty bank, as we already do for libraries.
        call    __bank_allocate_library
.allocated:
        ld      IMAGE_BANK(ix), a
        ld      a, d
        or      e
        ld      a, #2
        jp      z, .fail
        ld      IMAGE_DATA(ix), e
        ld      IMAGE_DATA+1(ix), d
        ld      IMAGE_CODE(ix), e
        ld      IMAGE_CODE+1(ix), d
        ld      l, IMAGE_XL+6(ix)
        ld      h, IMAGE_XL+7(ix)
        add     hl, de
        jp      c, .invalid
        ld      12(ix), l
        ld      13(ix), h
        ld      c, IMAGE_XL+6(ix)
        ld      b, IMAGE_XL+7(ix)
        call    .read
        jp      c, .read_error

        call    __crc32_reset
        push    ix
        pop     hl
        ld      de, #IMAGE_XL
        add     hl, de
        ld      bc, #12
        call    __crc32_update
        ld      l, IMAGE_CODE(ix)
        ld      h, IMAGE_CODE+1(ix)
        ld      c, IMAGE_XL+6(ix)
        ld      b, IMAGE_XL+7(ix)
        call    __crc32_update

        xor     a
        ld      IMAGE_RELOC_ERROR(ix), a
        ld      a, IMAGE_XL+8(ix)
        or      IMAGE_XL+9(ix)
        jp      z, .relocations_done
        ld      hl, #RELOCATION_CHUNK
        bit     2, IMAGE_MODE(ix)
        jr      nz, .os_relocation_buffer
        set     7, h
        call    __bank_allocate_current
        ld      a, d
        or      e
        jr      nz, .relocation_buffer_ready
        ld      hl, #RELOCATION_CHUNK   ; the image bank has no room left for
        call    __os_malloc             ; the buffer: use the common heap
        jr      .relocation_buffer_ready
.os_relocation_buffer:
        call    __os_malloc
.relocation_buffer_ready:
        ld      a, d
        or      e
        ld      a, #2
        jp      z, .fail
        ld      IMAGE_RELOC(ix), e
        ld      IMAGE_RELOC+1(ix), d

.relocation_loop:
        ld      a, IMAGE_XL+9(ix)
        or      a
        ld      a, #RELOCATIONS_PER_CHUNK
        jr      nz, .chunk_ready
        ld      a, IMAGE_XL+8(ix)
        cp      #RELOCATIONS_PER_CHUNK+1
        jr      c, .chunk_ready
        ld      a, #RELOCATIONS_PER_CHUNK
.chunk_ready:
        ld      IMAGE_CHUNK(ix), a
        ld      l, a
        ld      h, #0
        add     hl, hl
        add     hl, hl
        ld      c, l
        ld      b, h
        ld      e, IMAGE_RELOC(ix)
        ld      d, IMAGE_RELOC+1(ix)
        call    .read
        jp      c, .read_error
        ld      a, IMAGE_CHUNK(ix)
        ld      l, a
        ld      h, #0
        add     hl, hl
        add     hl, hl
        ld      c, l
        ld      b, h
        ld      l, IMAGE_RELOC(ix)
        ld      h, IMAGE_RELOC+1(ix)
        call    __crc32_update
        ld      a, IMAGE_RELOC_ERROR(ix)
        or      a
        jr      nz, .chunk_relocated
        ld      l, IMAGE_RELOC(ix)
        ld      h, IMAGE_RELOC+1(ix)
        ld      b, IMAGE_CHUNK(ix)
        call    __process_relocate_chunk
        or      a
        jr      z, .chunk_relocated
        ld      IMAGE_RELOC_ERROR(ix), a
.chunk_relocated:
        ld      a, IMAGE_XL+8(ix)
        sub     IMAGE_CHUNK(ix)
        ld      IMAGE_XL+8(ix), a
        ld      a, IMAGE_XL+9(ix)
        sbc     a, #0
        ld      IMAGE_XL+9(ix), a
        or      IMAGE_XL+8(ix)
        jr      nz, .relocation_loop

        ld      l, IMAGE_RELOC(ix)
        ld      h, IMAGE_RELOC+1(ix)
        call    .free_reloc
        xor     a
        ld      IMAGE_RELOC(ix), a
        ld      IMAGE_RELOC+1(ix), a

.relocations_done:
        call    __crc32_finish
        ld      a, l
        xor     16(ix)
        ld      c, a
        ld      a, h
        xor     17(ix)
        or      c
        ld      c, a
        ld      a, e
        xor     18(ix)
        or      c
        ld      c, a
        ld      a, d
        xor     19(ix)
        or      c
        jr      nz, .checksum
        ld      a, IMAGE_RELOC_ERROR(ix)
        or      a
        jp      nz, .fail


        bit     1, 7(ix)
        jr      z, .no_entry
        ld      l, 26(ix)
        ld      h, 27(ix)
        ld      c, IMAGE_XL+6(ix)
        ld      b, IMAGE_XL+7(ix)
        or      a
        sbc     hl, bc
        jr      nc, .invalid
        add     hl, bc
        ld      e, IMAGE_CODE(ix)
        ld      d, IMAGE_CODE+1(ix)
        add     hl, de
        jr      c, .invalid
        ld      IMAGE_ENTRY(ix), l
        ld      IMAGE_ENTRY+1(ix), h
.no_entry:
        ld      a, IMAGE_MODE(ix)
        and     #1
        jr      nz, .library
        call    __process_load_finish
        jr      .result
.library:
        call    __library_load_finish
        call    _enter_critical_section
.result:
        ld      b, a
        ld      a, d
        or      e
        ld      a, b
        jr      z, .finish_locked
        xor     a
        jr      .finish_locked
.read_error:
        ld      a, #3
        jr      .fail
.checksum:
        ld      a, #8
        jr      .fail
.invalid:
        ld      a, #4
.fail:
        ld      de, #0
.finish:
        call    _enter_critical_section
.finish_locked:
        push    de
        push    af
        ld      l, IMAGE_RELOC(ix)
        ld      h, IMAGE_RELOC+1(ix)
        call    .free_reloc
        ld      l, IMAGE_DATA(ix)
        ld      h, IMAGE_DATA+1(ix)
        ld      a, IMAGE_BANK(ix)
        call    .free
        ld      l, IMAGE_TABLE(ix)
        ld      h, IMAGE_TABLE+1(ix)
        call    __os_free
        ld      a, IMAGE_FD+1(ix)
        inc     a
        jr      z, .closed
        ld      l, IMAGE_FD(ix)
        ld      h, IMAGE_FD+1(ix)
        call    _close
.closed:
        ld      a, IMAGE_OLD_BANK(ix)
        call    __bank_map
        xor     a
        ld      (__image_busy), a
        pop     af
        ld      (_process_last_error), a
        pop     de
        ld      hl, #IMAGE_FRAME_SIZE
        add     hl, sp
        ld      sp, hl
        pop     iy
        pop     ix
        jp      _leave_critical_section
.busy:
        call    _leave_critical_section
        pop     af
        ld      a, #9
        ld      (_process_last_error), a
        ld      de, #0
        ret

.read:
        ld      l, IMAGE_FD(ix)
        ld      h, IMAGE_FD+1(ix)
        jp      __process_read_exact

.free_reloc:
        ld      a, h
        cp      #0xc0                   ; below the banked window: common heap
        jp      c, __os_free
        ld      a, IMAGE_BANK(ix)
.free:
        inc     a
        jp      z, __os_free
        dec     a
        jp      __bank_free
.magic:
        .ascii  "XPRG"
        .db     1
