        ; Shared XPRG disk, validation, CRC and XL relocation path.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _image_load
        .optsdcc -mz80 sdcccall(1)
        .globl  __image_load
        .globl  __image_retain
        .globl  __image_busy
        .globl  _process_last_error
        .globl  __bank_allocate
        .globl  __bank_free
        .globl  __bank_map
        .globl  __bank_current
        .globl  __os_malloc
        .globl  __os_free
        .globl  __crc32
        .globl  __process_read_exact
        .globl  __process_relocate
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
        .equ    IMAGE_XL,    74
        .equ    IMAGE_ENTRY, 76
        .equ    IMAGE_OWNER, 78
        .equ    IMAGE_TEMP,  80
        .equ    IMAGE_BANK,  82
        .equ    IMAGE_OLD_BANK, 83
        .equ    IMAGE_TABLE, 84

        .area   _CODE

        ; inputs: hl = path, a = 0 user process / 1 private library /
        ;         3 shared library / 4 system process
        ; outputs: de = process/interface or zero; error cell updated
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: IX+0..63 descriptor; +64..85 loader locals above.
        ; A try-lock serializes loads; contention is BUSY. There is no
        ; load-wide critical section; native I/O gates mask separately.
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
        ld      ix, #-86
        add     ix, sp
        ld      sp, ix
        ld      IMAGE_MODE(ix), a
        ld      IMAGE_FD+1(ix), #0xff
        xor     a
        ld      IMAGE_DATA(ix), a
        ld      IMAGE_DATA+1(ix), a
        ld      IMAGE_TEMP(ix), a
        ld      IMAGE_TEMP+1(ix), a
        ld      IMAGE_TABLE(ix), a
        ld      IMAGE_TABLE+1(ix), a
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
        ld      a, #10                 ; library needs a client process
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
        ld      a, 7(ix)                ; JP table, optional initializer
        and     #0xfd
        cp      #4
        jr      nz, .bad_header
        ld      a, 28(ix)
        or      29(ix)
        or      55(ix)                 ; bounded, nonempty service name
        jr      nz, .bad_header
        ld      a, 40(ix)
        or      a
        jr      z, .bad_header
        ld      a, 32(ix)
        cp      #64
        jr      nz, .bad_header
        ld      l, 34(ix)               ; 1..255 exports
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

        ; Pin an existing shared image before permitting reaping again.
        bit     1, IMAGE_MODE(ix)
        jr      z, .allocate
        call    _enter_critical_section
        call    __library_find
        ld      a, d
        or      e
        jr      z, .missing
        ex      de, hl
        ld      e, IMAGE_OWNER(ix)
        ld      d, IMAGE_OWNER+1(ix)
        call    __library_acquire
        call    _leave_critical_section
        jp      .result
.missing:
        call    _leave_critical_section
.allocate:
        ld      l, IMAGE_JPS(ix)
        ld      h, IMAGE_JPS+1(ix)
        ld      e, 12(ix)
        ld      d, 13(ix)
        add     hl, de
        jp      c, .invalid
        push    hl
        bit     2, IMAGE_MODE(ix)
        jr      z, .bank_allocate
        call    __os_malloc
        jr      .allocated
.bank_allocate:
        call    __bank_allocate
        ld      IMAGE_BANK(ix), a
.allocated:
        pop     bc
        ld      a, d
        or      e
        ld      a, #2
        jp      z, .fail
        ld      IMAGE_TEMP(ix), e
        ld      IMAGE_TEMP+1(ix), d
        call    .read
        jp      c, .read_error
        ld      l, IMAGE_TEMP(ix)
        ld      h, IMAGE_TEMP+1(ix)
        ld      e, IMAGE_JPS(ix)
        ld      d, IMAGE_JPS+1(ix)
        add     hl, de
        ld      IMAGE_XL(ix), l
        ld      IMAGE_XL+1(ix), h
        ld      c, 12(ix)
        ld      b, 13(ix)
        call    __crc32
        ld      a, l                   ; accumulate the four byte differences
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
        ; Relocate in the existing buffer. Finish binds compact exports,
        ; then splits off and frees the trailing relocation table and the
        ; metadata prefix without allocating a second copy.
        ;
        ; Export tables live in common memory; only code/data is retained in
        ; the bank arena, so no prefix is reserved before the code.
        ld      bc, #0
        ld      l,IMAGE_XL(ix)
        ld      h,IMAGE_XL+1(ix)
        ld      e,12(ix)
        ld      d,13(ix)
        call    __process_relocate
        ld      IMAGE_DATA(ix),l
        ld      IMAGE_DATA+1(ix),h
        or      a
        jr      nz,.fail
        ld      IMAGE_CODE(ix), e
        ld      IMAGE_CODE+1(ix), d
        ld      l, e
        ld      h, d
        add     hl, bc
        ld      12(ix), l              ; consumed payload size becomes the
        ld      13(ix), h              ; relocated code end for finish/retain
        bit     1, 7(ix)
        jr      z, .no_entry
        ld      l, 26(ix)
        ld      h, 27(ix)
        or      a
        sbc     hl, bc
        jr      nc, .invalid
        add     hl, bc
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
.result:
        ld      b, a
        ld      a, d
        or      e
        ld      a, b
        jr      z, .fail
        xor     a
        jr      .finish
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
        push    de
        push    af
        ld      l, IMAGE_DATA(ix)
        ld      h, IMAGE_DATA+1(ix)
        ld      a, IMAGE_BANK(ix)
        call    .free
        ld      l,IMAGE_TEMP(ix)
        ld      h,IMAGE_TEMP+1(ix)
        ld      a, IMAGE_BANK(ix)
        call    .free                   ; whole buffer, or split-off prefix
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
        ld      hl, #86
        add     hl, sp
        ld      sp, hl
        pop     iy
        pop     ix
        ret
.busy:
        call    _leave_critical_section
        pop     af
        ld      a, #9
        ld      (_process_last_error), a
        ld      de, #0
        ret
        ; Read into DE for BC bytes using the frame's file descriptor.
.read:
        ld      l, IMAGE_FD(ix)
        ld      h, IMAGE_FD+1(ix)
        jp      __process_read_exact
        ; HL = image payload, A = logical bank or FFh for the OS heap.
.free:
        inc     a
        jp      z, __os_free
        dec     a
        jp      __bank_free
.magic:
        .ascii  "XPRG"
        .db     1
