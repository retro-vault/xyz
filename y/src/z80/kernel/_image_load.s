        ; Shared XPRG disk, validation, CRC and XL relocation path.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module _image_load
        .optsdcc -mz80 sdcccall(1)
        .globl  __image_load
        .globl  __image_busy
        .globl  _process_last_error
        .globl  __yos_malloc
        .globl  __yos_free
        .globl  __crc32
        .globl  __process_read_exact
        .globl  __process_relocate
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

        .area   _CODE

        ; inputs: hl = path, a = 0 process / 1 private / 3 shared
        ; outputs: de = process/interface or zero; error cell updated
        ; clobbers: af, bc, de, hl; preserves ix and iy
        ; frame: IX+0..63 descriptor; +64..79 loader locals above.
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
        ld      ix, #-80
        add     ix, sp
        ld      sp, ix
        ld      IMAGE_MODE(ix), a
        ld      IMAGE_FD+1(ix), #0xff
        xor     a
        ld      IMAGE_DATA(ix), a
        ld      IMAGE_DATA+1(ix), a
        call    __current_process
        ld      IMAGE_OWNER(ix), c
        ld      IMAGE_OWNER+1(ix), b
        ld      a, IMAGE_MODE(ix)
        or      a
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
        cp      #YOS_VERSION+1
        ld      a, #7
        jp      nc, .fail
        ld      a, 31(ix)
        or      a
        ld      a, #7
        jp      nz, .fail
        ld      a, 33(ix)
        or      35(ix)
        jr      nz, .bad_header
        ld      a, IMAGE_MODE(ix)
        or      a
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
        call    __yos_malloc
        pop     bc
        ld      a, d
        or      e
        ld      a, #2
        jp      z, .fail
        ld      IMAGE_DATA(ix), e
        ld      IMAGE_DATA+1(ix), d
        call    .read
        jp      c, .read_error
        ld      l, IMAGE_DATA(ix)
        ld      h, IMAGE_DATA+1(ix)
        ld      e, IMAGE_JPS(ix)
        ld      d, IMAGE_JPS+1(ix)
        add     hl, de
        ld      IMAGE_XL(ix), l
        ld      IMAGE_XL+1(ix), h
        ld      c, 12(ix)
        ld      b, 13(ix)
        call    __crc32
        ld      a, l
        cp      16(ix)
        jr      nz, .checksum
        ld      a, h
        cp      17(ix)
        jr      nz, .checksum
        ld      a, e
        cp      18(ix)
        jr      nz, .checksum
        ld      a, d
        cp      19(ix)
        jr      nz, .checksum
        ld      l, IMAGE_XL(ix)
        ld      h, IMAGE_XL+1(ix)
        ld      e, 12(ix)
        ld      d, 13(ix)
        call    __process_relocate
        ld      a, d
        or      e
        jr      z, .invalid
        ld      IMAGE_CODE(ix), e
        ld      IMAGE_CODE+1(ix), d
        ld      l, IMAGE_XL(ix)
        ld      h, IMAGE_XL+1(ix)
        ld      bc, #6
        add     hl, bc
        ld      c, (hl)
        inc     hl
        ld      b, (hl)                 ; BC = code length
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
        or      a
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
        call    __yos_free
        ld      a, IMAGE_FD+1(ix)
        inc     a
        jr      z, .closed
        ld      l, IMAGE_FD(ix)
        ld      h, IMAGE_FD+1(ix)
        call    _close
.closed:
        xor     a
        ld      (__image_busy), a
        pop     af
        ld      (_process_last_error), a
        pop     de
        ld      hl, #80
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
.magic:
        .ascii  "XPRG"
        .db     1
