        ;; Load existing XPRG/XL v2 files from CP/M. CRC and relocation are
        ;; checked before entry; CP/M record padding is outside the payload.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_loader
        .optsdcc -mz80 sdcccall(1)
        .include "native.inc"
        .globl __native_load_process_args
        .globl __native_load_process, __native_load_library, __native_load_error
        .globl __native_processes, __native_start, __native_current
        .globl __native_query, __native_load_name, __native_trim
        .globl __native_library_begin, __native_library_abort, __native_library_private
        .globl __native_adapt_exports
        .globl _disk_open, _disk_close, _disk_read, _malloc, _free
        .globl __crc32_begin, __crc32_update, __crc32_end, ___sdcc_call_bc
        .globl __alto_call_banked
        .globl __partner_image_get, __partner_image_put, __partner_image_in, __partner_image_out
        .globl __partner_image_alloc, __partner_image_free, __partner_image_bank, __partner_bank_select
        .area _BSS
.caller_bank: .ds 1
.library_flags: .ds 1
.export_copy: .ds 2

__native_load_error:: .ds 1
.busy: .ds 1
.kind: .ds 1
.header: .ds 64
__native_load_name:: .ds 17
.service_name: .ds 16
.image: .ds 2
.code: .ds 2
.entry: .ds 2
.prefix: .ds 2
.payload: .ds 2
.length: .ds 2
.slot: .ds 2
.stack: .ds 2
.exports: .ds 2
.libowner: .ds 2
.arguments: .ds 2
.raw_exports: .ds 2
.xl: .ds 12
.reloc: .ds 4
.remaining: .ds 2
.basename: .ds 2
        .area _CODE
__native_load_process::
        ld de,#0
__native_load_process_args::
        ld (.arguments),de
        ld a,#1
        jp .load
__native_load_library::
        ld a,#2
.load:
        push ix
        push iy
        ld b,a
        ld a,(.busy)
        or a
        ld a,#9
        jp nz,.failed
        ld a,b
        ld (.kind),a
        cp #2
        jr nz,.flags_ready
        ld a,d
        or a
        ld a,#4
        jp nz,.failed
        ld a,e
        cp #2
        ld a,#4
        jp nc,.failed
        ld a,e
        ld (.library_flags),a
.flags_ready:
        ld a,#1
        ld (.busy),a
        ld a,(0xc002)
        ld (.caller_bank),a
        xor a
        ld (.export_copy),a
        ld (.export_copy+1),a
        push hl
        xor a
        ld (__native_load_error),a
        ld hl,#0
        ld (.image),hl
        ld (.stack),hl
        ld (.libowner),hl
        ld (.raw_exports),hl
        pop hl
.path_ready:
        ;; Flatten YOS directories while retaining an explicit CP/M drive.
        ld de,#__native_load_name
        inc hl
        ld a,(hl)
        dec hl
        cp #58
        jr nz,.base_ready
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
.base_ready:
        ld (.basename),de
        ld b,#14
.path:
        ld a,(hl)
        inc hl
        cp #47
        jp nz,.path_char
        ld de,(.basename)
        ld b,#14
        jp .path
.path_char:
        ld (de),a
        inc de
        or a
        jp z,.named
        djnz .path
        ld a,#4
        jp .rollback
.named:
        ld a,(.kind)
        cp #2
        jp nz,.process_slot
        ld hl,#__native_load_name
        inc hl
        ld a,(hl)
        dec hl
        cp #58
        jp nz,.service_base
        inc hl
        inc hl
.service_base:
        ld de,#.service_name
        ld b,#15
.service_char:
        ld a,(hl)
        inc hl
        or a
        jp z,.service_end
        cp #46
        jp z,.service_end
        cp #65
        jp c,.lowered
        cp #91
        jp nc,.lowered
        add a,#32
.lowered:
        ld (de),a
        inc de
        djnz .service_char
.service_end:
        xor a
        ld (de),a
        ld a,(.library_flags)
        or a
        jp z,.open                      ; private instances never reuse a name
        ld hl,#.service_name
        call __native_query
        ld a,d
        or e
        jp nz,.success
        jp .open
.process_slot:
        ld ix,#__native_processes
        ld b,#P_COUNT
.find_slot:
        ld a,P_STATE(ix)
        or a
        jp z,.slot_found
        ld de,#P_BYTES
        add ix,de
        djnz .find_slot
        ld a,#2
        jp .rollback
.slot_found:
        ld (.slot),ix
.open:
        ld hl,#__native_load_name
        xor a
        call _disk_open
        or a
        ld a,#1
        jp nz,.rollback
        ld hl,#.header
        ld bc,#64
        call .read_exact
        jp nz,.read_error
        ld hl,#.header
        ld de,#.magic
        ld b,#5
.magic_loop:
        ld a,(de)
        cp (hl)
        jp nz,.invalid
        inc hl
        inc de
        djnz .magic_loop
        ld ix,#.header
        ld a,(.kind)
        cp 5(ix)
        jp nz,.invalid
        ld a,6(ix)
        cp #1
        jp nz,.invalid
        ld a,30(ix)
        cp #1
        jp nz,.invalid
        ld a,31(ix)
        or 14(ix)
        or 15(ix)
        or 33(ix)
        or 35(ix)
        jp nz,.invalid
        ld a,(.kind)
        cp #1
        jp nz,.service_header
        ld a,7(ix)
        cp #2
        jp nz,.invalid
        ld a,32(ix)
        or 34(ix)
        jp nz,.invalid
        ld a,28(ix)
        or 29(ix)
        jp z,.invalid
        ld hl,#0
        jp .metadata
.service_header:
        ld a,7(ix)
        cp #6
        jp nz,.invalid
        ld a,32(ix)
        cp #64
        jp nz,.invalid
        ld l,34(ix)
        ld h,#0
        ld a,l
        or a
        jp z,.invalid
        ld e,l
        ld d,h
        add hl,hl
        add hl,de
.metadata:
        ld (.prefix),hl
        ld de,#64
        add hl,de
        ld a,l
        cp 8(ix)
        jp nz,.invalid
        cp 10(ix)
        jp nz,.invalid
        ld a,h
        cp 9(ix)
        jp nz,.invalid
        cp 11(ix)
        jp nz,.invalid
        ld l,12(ix)
        ld h,13(ix)
        ld (.payload),hl
        ld de,#12
        or a
        sbc hl,de
        jp c,.invalid
        ;; Keep only code and exports resident. Stream relocation records:
        ;; the Paint tool service has more than 6 KiB of relocation metadata.
        ld hl,(.prefix)
        ld a,h
        or l
        jr z,.read_xl
        call _malloc
        ld a,d
        or e
        jp z,.no_memory
        ld (.raw_exports),de
        ex de,hl
        ld bc,(.prefix)
        call .read_exact
        jp nz,.read_error
.read_xl:
        ld hl,#.xl
        ld bc,#12
        call .read_exact
        jp nz,.read_error
        ld ix,#.xl
        ld a,0(ix)
        cp #88
        jp nz,.invalid
        ld a,1(ix)
        cp #76
        jp nz,.invalid
        ld a,2(ix)
        cp #2
        jp nz,.invalid
        ld l,8(ix)
        ld h,9(ix)
        ld (.remaining),hl
        add hl,hl
        jp c,.invalid
        add hl,hl
        jp c,.invalid
        ld c,6(ix)
        ld b,7(ix)
        ld (.length),bc
        ld a,b
        or c
        jp z,.invalid
        add hl,bc
        jp c,.invalid
        ld de,#12
        add hl,de
        jp c,.invalid
        ld de,(.payload)
        or a
        sbc hl,de
        jp nz,.invalid
        ld hl,(.prefix)
        add hl,bc
        jp c,.invalid
        ld de,#12
        add hl,de
        jp c,.invalid
        call __partner_image_alloc
        ld a,d
        or e
        jp z,.no_memory
        ld (.image),de
        push de
        call __crc32_begin
        ld hl,#.xl
        ld bc,#12
        call __crc32_update
        pop de
        ld bc,(.prefix)
        ld a,b
        or c
        jr z,.copy_xl
        ld hl,(.raw_exports)
        call __partner_image_in
.copy_xl:
        ld hl,#.xl
        ld bc,#12
        call __partner_image_in
        ld (.code),de
        ld hl,(.raw_exports)
        call _free
        ld hl,#0
        ld (.raw_exports),hl
        call .read_image
        jp nz,.read_error
.relocate:
        ld hl,(.remaining)
        ld a,h
        or l
        jp z,.relocated
        dec hl
        ld (.remaining),hl
        ld hl,#.reloc
        ld bc,#4
        call .read_exact
        jp nz,.read_error
        ld hl,#.reloc
        ld bc,#4
        call __crc32_update
        ld hl,(.reloc)
        ld a,(.reloc+2)
        cp #1
        jr z,.span_ready
        cp #2
        jp nz,.invalid
        inc hl
        ld a,h
        or l
        jp z,.invalid
.span_ready:
        ld bc,(.length)
        or a
        sbc hl,bc
        jp nc,.invalid
        ld hl,(.reloc)
        ld bc,(.code)
        add hl,bc
        ld a,(.reloc+2)
        cp #2
        jr z,.word
        ld a,(.reloc+3)
        srl a
        jp nz,.invalid
        ld a,c
        jr nc,.byte
        ld a,b
.byte:
        push bc
        ld b,a
        call __partner_image_get
        add a,b
        pop bc
        call __partner_image_put
        jp .relocate
.word:
        ld a,(.reloc+3)
        or a
        jp nz,.invalid
        call __partner_image_get
        add a,c
        call __partner_image_put
        inc hl
        call __partner_image_get
        adc a,b
        call __partner_image_put
        jp .relocate
.relocated:
        call _disk_close
        or a
        jp nz,.read_error_closed
        call __crc32_end
        ld ix,#.header
        ld a,l
        cp 16(ix)
        jp nz,.crc_error
        ld a,h
        cp 17(ix)
        jp nz,.crc_error
        ld a,e
        cp 18(ix)
        jp nz,.crc_error
        ld a,d
        cp 19(ix)
        jp nz,.crc_error
        ld bc,(.length)
        ld de,(.code)
        ld l,26(ix)
        ld h,27(ix)
        or a
        sbc hl,bc
        jp nc,.invalid_closed
        add hl,bc
        add hl,de
        jp c,.invalid_closed
        ld (.entry),hl
        ld a,(.kind)
        cp #2
        jp z,.library
        ld hl,(.header+28)
        ;; Honor the PRC's stack request, plus room for Partner's pointer
        ;; adapters and bank gates. Keep stacks common: deferred callbacks
        ;; may retain near pointers into a suspended process's stack.
        ld de,#256
        add hl,de
        jp c,.invalid_closed
        ld de,#512
        or a
        sbc hl,de
        jp nc,.stack_size
        ld hl,#0
.stack_size:
        add hl,de
        push hl
        call _malloc
        pop bc
        ld a,d
        or e
        jp z,.no_memory_closed
        ld (.stack),de
        ;; Watermark unused stack space so emulator tests can measure the
        ;; deepest calls, including asynchronous interrupts.
        push bc
        push de
        ex de,hl
.stack_watermark:
        ld (hl),#0xa5
        inc hl
        dec bc
        ld a,b
        or c
        jr nz,.stack_watermark
        pop de
        pop bc
        ld ix,(.slot)
        ld P_STACK(ix),e
        ld P_STACK+1(ix),d
        ex de,hl
        add hl,bc
        dec hl
        ld de,#__native_start
        ld (hl),d
        dec hl
        ld (hl),e
        ld b,#12
.zero_frame:
        dec hl
        ld (hl),#0
        djnz .zero_frame
        ld P_SP(ix),l
        ld P_SP+1(ix),h
        ld hl,(.image)
        ld P_IMAGE(ix),l
        ld P_IMAGE+1(ix),h
        ld hl,(.code)
        ld P_CODE(ix),l
        ld P_CODE+1(ix),h
        ld hl,(.entry)
        ld P_ENTRY(ix),l
        ld P_ENTRY+1(ix),h
        ld hl,(.length)
        ld P_LENGTH(ix),l
        ld P_LENGTH+1(ix),h
        ld a,(__partner_image_bank)
        ld P_BANK(ix),a
        inc a                          ; scheduler resumes the physical bank
        ld P_RESUME(ix),a
        ;; Keep only a pointer in the process record. Empty argument tails
        ;; need no allocation; nonempty tails retain the ABI's 127-byte limit.
        ld P_ARGS(ix),#0
        ld P_ARGS+1(ix),#0
        ld hl,(.arguments)
        ld a,h
        or l
        jr z,.arguments_done
        ld a,(hl)
        or a
        jr z,.arguments_done
        push hl
        ld bc,#0
.argument_length:
        inc bc
        inc hl
        ld a,c
        cp #127
        jr z,.argument_sized
        ld a,(hl)
        or a
        jr nz,.argument_length
.argument_sized:
        push bc
        ld h,b
        ld l,c
        inc hl
        call _malloc
        pop bc
        pop hl
        ld a,d
        or e
        jp z,.no_memory_closed
        ld P_ARGS(ix),e
        ld P_ARGS+1(ix),d
        ldir
        xor a
        ld (de),a
.arguments_done:
        push ix
        pop hl
        ld de,#P_NAME
        add hl,de
        ex de,hl
        ld hl,#.header+40
        ld bc,#8
        ldir
        ld P_STATE(ix),#1
        push ix
        pop de
        jp .success
.crc_error:
        ld a,#8
        jp .rollback
.no_memory:
        call _disk_close
.no_memory_closed:
        ld a,#2
        jp .rollback
.invalid:
        call _disk_close
.invalid_closed:
        ld a,#4
        jp .rollback
.read_error:
        call _disk_close
.read_error_closed:
        ld a,#3
        jp .rollback
.library:
        ;; Replace each raw JP/offset with the existing packed far table.
        ld hl,(.image)
        ld (.exports),hl
        ld a,(.header+34)
        ld b,a
.export:
        call __partner_image_get
        cp #0xc3
        jp nz,.invalid_closed
        ld a,(__partner_image_bank)
        call __partner_image_put
        inc hl
        call __partner_image_get
        ld e,a
        inc hl
        call __partner_image_get
        ld d,a
        push hl
        push bc
        ex de,hl
        ld bc,(.length)
        or a
        sbc hl,bc
        jp nc,.bad_export
        add hl,bc
        ld bc,(.code)
        add hl,bc
        ex de,hl
        pop bc
        pop hl
        ld a,d
        call __partner_image_put
        dec hl
        ld a,e
        call __partner_image_put
        inc hl
        inc hl
        djnz .export
        ld hl,(.prefix)
        call _malloc
        ld a,d
        or e
        jp z,.no_memory_closed
        ld (.export_copy),de
        ld hl,(.exports)
        ld (.exports),de
        ld bc,(.prefix)
        call __partner_image_out
        ld hl,#.service_name
        ld de,(.exports)
        ld a,(.header+34)
        call __native_adapt_exports
        ld (.exports),de
        ld hl,(.image)
        call __native_library_begin
        ld a,d
        or e
        jp z,.no_memory_closed
        ld (.libowner),de
        push de
        pop ix
        ld hl,(.export_copy)
        ld 2(ix),l
        ld 3(ix),h
        ld hl,#0
        ld 4(ix),l
        ld 5(ix),h
        ld a,(.library_flags)
        xor #1
        ld (__native_library_private),a
        ld hl,(__native_current)
        or a
        jr z,.library_owner_ready
        ld 4(ix),l                     ; private library's acquiring process
        ld 5(ix),h
.library_owner_ready:
        push hl
        ld (__native_current),de
        ld hl,(.exports)
        ld bc,(.entry)
        ld a,(__partner_image_bank)
        call __alto_call_banked
        pop hl
        ld (__native_current),hl
        xor a
        ld (__native_library_private),a
        ld a,d
        or e
        jp nz,.invalid_closed
        ld de,(.exports)
        jp .success
.bad_export:
        pop bc
        pop hl
        jp .invalid_closed
.rollback:
        push af
        ld hl,(.libowner)
        ld a,h
        or l
        jr z,.no_library
        push hl
        pop ix
        call __native_library_abort
.no_library:
        ld hl,(.raw_exports)
        call _free
        ld hl,(.stack)
        call _free
        ld hl,(.export_copy)
        call _free
        ld hl,(.image)
        ld a,(__partner_image_bank)
        call __partner_image_free
        pop af
        ld (__native_load_error),a
        ld de,#0
.success:
        ld a,(.caller_bank)
        call __partner_bank_select
        xor a
        ld (.busy),a
        pop iy
        pop ix
        ret
.failed:
        ld (__native_load_error),a
        ld de,#0
        pop iy
        pop ix
        ret
.read_exact:
        push bc
        call _disk_read
        pop hl
        or a
        ret nz
        or a
        sbc hl,bc
        ld a,h
        or l
        ret
.magic: .ascii "XPRG"
        .db 1
        ;; HL=live native allocation, DE=new payload size (no growth).
__native_trim::
        ; Keeping the original allocation is a valid no-growth shrink.
        ret

        .area _KERNEL_BSS
.image_left: .ds 2
.image_cursor: .ds 2
.image_buffer: .ds 128
        .area _CODE
.read_image:
        ld hl,(.length)
        ld (.image_left),hl
        ld hl,(.code)
        ld (.image_cursor),hl
.image_chunk:
        ld bc,(.image_left)
        ld a,b
        or c
        ret z
        ld a,b
        or a
        jr nz,.image_128
        ld a,c
        cp #129
        jr c,.image_count
.image_128:
        ld bc,#128
.image_count:
        push bc
        ld hl,#.image_buffer
        call .read_exact
        pop bc
        ret nz
        push bc
        ld hl,#.image_buffer
        call __crc32_update
        pop bc
        ld hl,(.image_left)
        or a
        sbc hl,bc
        ld (.image_left),hl
        ld hl,#.image_buffer
        ld de,(.image_cursor)
        call __partner_image_in
        ld (.image_cursor),de
        jr .image_chunk
