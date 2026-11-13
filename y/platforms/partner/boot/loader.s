        ;; loader.s
        ;;
        ;; Standard Partner-ROM floppy hand-off at 0xf600. It disables the ROM
        ;; overlay, validates the sector-zero bare-metal manifest, establishes
        ;; an IM2 table, loads one or two named CP/M 3 files into the selected
        ;; RAM banks, and jumps to the requested startup image.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        .module loader

        .include "partner.inc"
        .include "floppy.inc"
        .include "manifest.inc"

        .globl  loader_entry
        .globl  loader_status
        .globl  loader_far_template
        .globl  interrupt_init
        .globl  floppy_init
        .globl  loader_cpm_load

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; loader_entry
        ;; Entry selected by both standard Partner ROM variants after their
        ;; fixed cylinder-zero read. The linker must place this first at f600.
        ;;
        ;; Returns: never; jumps to the manifest entry or halts on error
        ;;
        ;; Clobbers: all registers
loader_entry::
        di
        ld      sp,#PARTNER_LOADER_STACK_TOP

        ;; An I/O access in 0x80..0x87 removes the low-memory ROM overlay.
        in      a,(PARTNER_PORT_ROM_OFF)
        xor     a
        out     (PARTNER_PORT_BANK_1),a

        call    interrupt_init
        call    loader_read_manifest
        jr      nz,.loader_halt

        xor     a                       ; boot from physical drive 0
        call    floppy_init
        jr      nz,.loader_floppy_error

        ld      ix,#loader_bank1_descriptor
        call    loader_load_descriptor
        jr      nz,.loader_bank1_cpm_error

        ld      a,(loader_flags)
        bit     0,a
        jr      z,.loader_install_runtime

        xor     a
        out     (PARTNER_PORT_BANK_2),a
        ld      ix,#loader_bank2_descriptor
        call    loader_load_descriptor
        jr      nz,.loader_bank2_cpm_error

.loader_install_runtime:
        call    loader_install_memory_runtime

        ld      a,(loader_flags)
        bit     1,a
        jr      z,.loader_start_bank1

.loader_start_bank2:
        xor     a
        out     (PARTNER_PORT_BANK_2),a
        ld      hl,(loader_bank2_descriptor + MANIFEST_DESC_ENTRY)
        jr      .loader_start_program

.loader_start_bank1:
        xor     a
        out     (PARTNER_PORT_BANK_1),a
        ld      hl,(loader_bank1_descriptor + MANIFEST_DESC_ENTRY)

.loader_start_program:
        xor     a
        ld      (loader_status),a
        ei
        jp      (hl)

.loader_floppy_error:
        add     a,#0x10                 ; distinguish loader from FDC errors
        ld      (loader_status),a
        jr      .loader_halt

.loader_bank1_cpm_error:
        add     a,#0x20                 ; distinguish bank-1 CP/M failures
        ld      (loader_status),a
        jr      .loader_halt

.loader_bank2_cpm_error:
        add     a,#0x40                 ; distinguish bank-2 CP/M failures
        ld      (loader_status),a

.loader_halt:
        di
.loader_halt_loop:
        halt
        jr      .loader_halt_loop

        ;; ------------------------------------------------------------
        ;; loader_load_descriptor
        ;; Load one cached manifest descriptor into the currently selected
        ;; bank. The worker is a tail call and may clobber IX.
        ;;
        ;; Arguments: IX = packed manifest descriptor
        ;; Returns: loader_cpm_load status in A and flags
        ;; Clobbers: AF, BC, DE, HL, IX
loader_load_descriptor:
        ld      a,MANIFEST_DESC_USER(ix)
        ld      b,MANIFEST_DESC_SECTORS(ix)
        ld      e,MANIFEST_DESC_LOAD(ix)
        ld      d,MANIFEST_DESC_LOAD_HIGH(ix)
        push    ix
        pop     hl
        inc     hl                      ; canonical name follows user byte
        jp      loader_cpm_load

        ;; ------------------------------------------------------------
        ;; loader_read_manifest
        ;; Validate and cache the manifest before the destination image can
        ;; overwrite its ROM-preloaded copy at 0xe000.
        ;;
        ;; Returns: A = 0/Z on success, loader error/NZ otherwise
        ;;
        ;; Clobbers: A, B, DE, HL
loader_read_manifest:
        ld      hl,#MANIFEST_MAGIC
        ld      de,#loader_manifest_magic
        ld      b,#4
.loader_magic_loop:
        ld      a,(de)
        cp      (hl)
        jp      nz,.loader_bad_manifest
        inc     de
        inc     hl
        djnz    .loader_magic_loop

        ld      a,(MANIFEST_VERSION)
        cp      #MANIFEST_CURRENT_VERSION
        jp      nz,.loader_bad_version

        ;; The sum over bytes 3..43, including the stored checksum, is zero.
        ld      hl,#MANIFEST_MAGIC
        ld      b,#(MANIFEST_CHECKSUM_LAST - MANIFEST_CHECKSUM_FIRST + 1)
        xor     a
.loader_checksum_loop:
        add     a,(hl)
        inc     hl
        djnz    .loader_checksum_loop
        or      a
        jp      nz,.loader_bad_checksum

        ld      a,(MANIFEST_FLAGS)
        and     #(0xff ^ MANIFEST_VALID_FLAGS)
        jp      nz,.loader_bad_range
        ld      a,(MANIFEST_FLAGS)
        ld      (loader_flags),a
        bit     1,a
        jr      z,.loader_copy_descriptors
        bit     0,a                     ; cannot start an absent bank 2
        jp      z,.loader_bad_range

.loader_copy_descriptors:
        ld      hl,#MANIFEST_BANK1_DESCRIPTOR
        ld      de,#loader_bank1_descriptor
        ld      bc,#(MANIFEST_DESC_SIZE * 2)
        ldir

        ;; Payloads remain wholly within banked 0x0000..0xbfff. Common RAM is
        ;; reserved for runtime state, heaps, stack, and interrupt vectors.
        ld      b,#(PARTNER_BANK_LIMIT >> 8)
.loader_validate_bank1:
        ld      ix,#loader_bank1_descriptor
        call    loader_validate_descriptor
        jp      nz,.loader_bad_range

        ld      a,(loader_flags)
        bit     0,a
        jr      z,.loader_manifest_ok
        ld      b,#(PARTNER_BANK_LIMIT >> 8)
        ld      ix,#loader_bank2_descriptor
        call    loader_validate_descriptor
        jp      nz,.loader_bad_range

.loader_manifest_ok:
        xor     a
        ld      (loader_status),a
        ret

        ;; ------------------------------------------------------------
        ;; loader_validate_descriptor
        ;; Check one cached descriptor. Page alignment lets the high byte act
        ;; as a 256-byte sector index.
        ;;
        ;; Arguments: IX = descriptor, B = exclusive maximum page
        ;; Returns: A = 0/Z when valid, A != 0/NZ otherwise
        ;; Clobbers: A, C, H
loader_validate_descriptor:
        ld      a,MANIFEST_DESC_USER(ix)
        cp      #16
        jp      nc,loader_descriptor_bad
        ld      a,MANIFEST_DESC_SECTORS(ix)
        or      a
        jp      z,loader_descriptor_bad
        cp      #(MANIFEST_MAX_SECTORS + 1)
        jp      nc,loader_descriptor_bad
        ld      a,MANIFEST_DESC_LOAD(ix)
        or      a
        jp      nz,loader_descriptor_bad
        ld      h,MANIFEST_DESC_LOAD_HIGH(ix)
        ld      a,MANIFEST_DESC_SECTORS(ix)
        add     a,h
        jp      c,loader_descriptor_bad
        cp      b
        jr      c,loader_descriptor_end_ok
        jp      nz,loader_descriptor_bad
loader_descriptor_end_ok:
        ld      c,a                     ; exclusive ending page
        ld      a,MANIFEST_DESC_ENTRY_HIGH(ix)
        cp      h
        jr      nc,descriptor_entry_ok
        ld      a,#1
        or      a
        ret
descriptor_entry_ok:
        cp      c
        jr      c,descriptor_valid
        ld      a,#1
        or      a
        ret
descriptor_valid:
        xor     a
        ret

loader_descriptor_bad:
        ld      a,#1
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; loader_install_memory_runtime
        ;; Publish payload heap bounds for both private banks. The far-access
        ;; dispatcher is already resident at a fixed address in this image.
        ;;
        ;; Returns: nothing; selected bank unchanged
        ;;
        ;; Clobbers: AF, BC, DE, HL
loader_install_memory_runtime:
        ld      hl,#loader_memory_magic
        ld      de,#PARTNER_MEMORY_INFO_MAGIC
        ld      bc,#4
        ldir
        ld      a,(loader_bank1_descriptor + MANIFEST_DESC_LOAD_HIGH)
        ld      b,a
        ld      a,(loader_bank1_descriptor + MANIFEST_DESC_SECTORS)
        add     a,b
        ld      (PARTNER_MEMORY_INFO_BANK1_END + 1),a
        xor     a
        ld      (PARTNER_MEMORY_INFO_BANK1_END),a

        ld      hl,#0x0100
        ld      a,(loader_flags)
        bit     0,a
        jr      z,.loader_memory_bank2_end_ready
        ld      a,(loader_bank2_descriptor + MANIFEST_DESC_LOAD_HIGH)
        ld      b,a
        ld      a,(loader_bank2_descriptor + MANIFEST_DESC_SECTORS)
        add     a,b
        ld      h,a
        ld      l,#0
.loader_memory_bank2_end_ready:
        ld      (PARTNER_MEMORY_INFO_BANK2_END),hl
        ld      a,(loader_flags)
        ld      (PARTNER_MEMORY_INFO_FLAGS),a
        bit     1,a
        ld      a,#1
        jr      z,.loader_memory_start_ready
        inc     a
.loader_memory_start_ready:
        ld      (PARTNER_MEMORY_INFO_BANK),a
        ret

loader_memory_magic:
        .ascii  "BMEM"

.loader_bad_manifest:
        ld      a,#1
        jr      .loader_store_error
.loader_bad_version:
        ld      a,#2
        jr      .loader_store_error
.loader_bad_checksum:
        ld      a,#3
        jr      .loader_store_error
.loader_bad_range:
        ld      a,#4
.loader_store_error:
        ld      (loader_status),a
        or      a
        ret

loader_manifest_magic:
        .ascii  "BMOS"

        ;; Loader state remains below the stack reserve and is discarded when
        ;; the loaded program takes control.
loader_bank1_descriptor:
        .ds     MANIFEST_DESC_SIZE
loader_bank2_descriptor:
        .ds     MANIFEST_DESC_SIZE

        ;; Fixed far-pointer runtime retained with the loader. Entry +0 reads
        ;; a byte; entry +6 writes one. Both preserve BC, DE, and HL.
        .area   _FAR_RUNTIME
loader_far_template::
        push    bc
        push    de
        ld      b,#0
        jr      .loader_far_dispatch
        push    bc
        push    de
        ld      d,a
        ld      b,#1
.loader_far_dispatch:
        ld      a,c
        or      a
        jr      z,.loader_far_direct
        ld      a,(0xc002)
        cp      c
        jr      z,.loader_far_direct
        ld      e,a
        ld      a,c
        cp      #1
        jr      z,.loader_far_select_one
        xor     a
        out     (PARTNER_PORT_BANK_2),a
        jr      .loader_far_selected
.loader_far_select_one:
        xor     a
        out     (PARTNER_PORT_BANK_1),a
.loader_far_selected:
        ld      a,b
        or      a
        jr      nz,.loader_far_put_switched
        ld      d,(hl)
        jr      .loader_far_restore
.loader_far_put_switched:
        ld      a,d
        ld      (hl),a
.loader_far_restore:
        ld      a,e
        cp      #1
        jr      z,.loader_far_restore_one
        xor     a
        out     (PARTNER_PORT_BANK_2),a
        jr      .loader_far_return
.loader_far_restore_one:
        xor     a
        out     (PARTNER_PORT_BANK_1),a
.loader_far_return:
        ld      a,d
        pop     de
        pop     bc
        ret
.loader_far_direct:
        ld      a,b
        or      a
        jr      nz,.loader_far_put_direct
        ld      d,(hl)
        jr      .loader_far_return
.loader_far_put_direct:
        ld      a,d
        ld      (hl),a
        jr      .loader_far_return
loader_flags:
        .db     0x00
loader_status::
        .db     0xff
