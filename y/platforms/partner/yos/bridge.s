        ;; Common pointer adapters for the single-copy resident kernel.
        ;; Calls into the kernel use generated veneers; client data stays mapped
        ;; while it is copied. Packed arguments and IX/IY are preserved.
        .module partner_bridge
        .globl __partner_read, __partner_write, __kernel_read, __kernel_write
        .globl __partner_create_window, __partner_create_window_banked
        .globl __kernel_create_window_banked, _malloc, _free
        .globl __partner_invalidate, __kernel_window_invalidate
        .globl __partner_suspend_rect, __kernel__mouse_suspend_rect
        .area _FIXED_CODE
__partner_read::
        xor a
        jr .io
__partner_write::
        ld a,#1
.io:
        push ix
        push iy
        ld ix,#0
        add ix,sp
        push hl                         ; -2 fd
        push de                         ; -4 client buffer
        ld c,6(ix)
        ld b,7(ix)
        push bc                         ; -6 remaining
        ld hl,#0
        push hl                         ; -8 total
        push af                         ; -9 read/write
        ld hl,#-128
        add hl,sp
        ld sp,hl
        push hl
        pop iy                          ; common transfer buffer
.io_next:
        ld c,-6(ix)
        ld b,-5(ix)
        ld a,b
        or c
        jp z,.io_done
        ld a,b
        or a
        jr nz,.io_chunk
        ld a,c
        cp #129
        jr c,.io_count
.io_chunk:
        ld bc,#128
.io_count:
        push bc
        ld a,-9(ix)
        or a
        jr z,.io_read
        ld l,-4(ix)
        ld h,-3(ix)
        push iy
        pop de
        ldir
        pop bc
        push bc
        ld l,-2(ix)
        ld h,-1(ix)
        push iy
        pop de
        push bc                         ; libcpmfs consumes its count
        call __kernel_write
        jr .io_result
.io_read:
        ld l,-2(ix)
        ld h,-1(ix)
        push iy
        pop de
        push bc
        call __kernel_read
.io_result:
        pop bc                          ; requested chunk
        ld a,d
        and e
        inc a
        jr z,.io_error
        ld a,d
        or e
        jr z,.io_done
        push bc
        push de
        ld a,-9(ix)
        or a
        jr nz,.io_advance
        ld b,d
        ld c,e
        push iy
        pop hl
        ld e,-4(ix)
        ld d,-3(ix)
        ldir
.io_advance:
        pop de                          ; actual
        ld l,-4(ix)
        ld h,-3(ix)
        add hl,de
        ld -4(ix),l
        ld -3(ix),h
        ld l,-8(ix)
        ld h,-7(ix)
        add hl,de
        ld -8(ix),l
        ld -7(ix),h
        ld l,-6(ix)
        ld h,-5(ix)
        or a
        sbc hl,de
        ld -6(ix),l
        ld -5(ix),h
        pop hl                          ; requested
        or a
        sbc hl,de
        jp z,.io_next
.io_done:
        ld e,-8(ix)
        ld d,-7(ix)
        jr .io_return
.io_error:
        ld e,-8(ix)
        ld d,-7(ix)
        ld a,d
        or e
        jr nz,.io_return
        dec de
.io_return:
        ld sp,ix
        pop iy
        pop ix
        pop bc
        pop hl                          ; consume original count
        push bc
        ret

__partner_create_window::
        ld a,(0xc002)
        dec a
__partner_create_window_banked::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        push hl                         ; -2 borrowed title
        push de                         ; -4 parent
        push af                         ; -5 procedure bank
        ld bc,#1
        ld a,h
        or l
        jr z,.title_counted
.title_length:
        ld a,(hl)
        or a
        jr z,.title_counted
        inc hl
        inc bc
        jr .title_length
.title_counted:
        push bc
        ld h,b
        ld l,c
        call _malloc
        pop bc
        ld a,d
        or e
        jr z,.create_done
        push de                         ; -8 common title
        ld l,-2(ix)
        ld h,-1(ix)
        ld a,h
        or l
        jr z,.title_empty
        ldir
        jr .title_ready
.title_empty:
        xor a
        ld (de),a
.title_ready:
        ld b,#6                         ; six original word arguments
        push ix
        pop hl
        ld de,#16
        add hl,de
.create_args:
        ld e,(hl)
        inc hl
        ld d,(hl)
        dec hl
        dec hl
        dec hl
        push de
        djnz .create_args
        ld l,-8(ix)
        ld h,-7(ix)
        ld e,-4(ix)
        ld d,-3(ix)
        ld a,-5(ix)
        call __kernel_create_window_banked
        push de
        ld l,-8(ix)
        ld h,-7(ix)
        call _free
        pop de
.create_done:
        ld sp,ix
        pop iy
        pop ix
        pop bc
        ld hl,#12
        add hl,sp
        ld sp,hl
        push bc
        ret

        ;; Borrowed rectangles must be copied before selecting kernel RAM.
__partner_invalidate::
        push ix
        ld ix,#0
        add ix,sp
        ld de,#-8
        add ix,de
        ld sp,ix
        push ix
        pop de
        ld bc,#8
        ldir
        push ix
        pop hl
        call __kernel_window_invalidate
        ld hl,#8
        add hl,sp
        ld sp,hl
        pop ix
        ret
__partner_suspend_rect::
        push ix
        ld ix,#0
        add ix,sp
        ld de,#-8
        add ix,de
        ld sp,ix
        push ix
        pop de
        ld bc,#8
        ldir
        push ix
        pop hl
        call __kernel__mouse_suspend_rect
        ld hl,#8
        add hl,sp
        ld sp,hl
        pop ix
        ret

        ;; Copy a borrowed region and its rectangles when either lives below
        ;; C000. Return DE=usable region, HL=allocation to free (or zero).
        .globl __partner_copy_region
__partner_copy_region::
        ld a,h
        or l
        jr z,.region_direct
        ld a,h
        cp #0xc0
        jr c,.region_copy
        ld e,(hl)
        inc hl
        ld d,(hl)
        dec hl
        ld a,d
        cp #0xc0
        jr nc,.region_direct
        or e
        jr nz,.region_copy
.region_direct:
        ex de,hl
        ld hl,#0
        ret
.region_copy:
        push ix
        push iy
        push hl
        pop ix
        ld e,0(ix)
        ld d,1(ix)
        push de
        ld l,2(ix)
        ld h,#0
        add hl,hl
        add hl,hl
        add hl,hl
        push hl
        inc hl
        inc hl
        inc hl
        call _malloc
        pop bc
        pop hl
        ld a,d
        or e
        jr z,.region_failure
        push de
        push de
        pop iy
        inc de
        inc de
        inc de
        ld 0(iy),e
        ld 1(iy),d
        ld a,2(ix)
        ld 2(iy),a
        ld a,b
        or c
        jr z,.region_empty
        ldir
.region_empty:
        pop de
        ld h,d
        ld l,e
        pop iy
        pop ix
        ret
.region_failure:
        ld hl,#.empty_region
        ex de,hl
        ld hl,#0
        pop iy
        pop ix
        ret
        .area _DATA
.empty_region: .db 0,0,0
        .area _FIXED_CODE
        .globl __partner_bitmap, __kernel_gdrawbmp
__partner_bitmap::
        push ix
        push iy
        ld ix,#0
        add ix,sp
        push de                         ; -2 borrowed clip
        push hl                         ; -4 borrowed context
        ld hl,#-10
        add hl,sp
        ld sp,hl                        ; -14 common context
        ex de,hl
        ld l,-4(ix)
        ld h,-3(ix)
        ld bc,#10
        ldir
        ld l,-2(ix)
        ld h,-1(ix)
        call __partner_copy_region
        push hl                         ; -16 allocation
        push de                         ; -18 clip
        ld l,10(ix)
        ld h,11(ix)                     ; bitmap (x,y,bmp)
        push hl
        call __partner_bitmap_size
        ld h,b
        ld l,c
        push hl
        call _malloc
        pop bc
        pop hl
        ld a,d
        or e
        jr z,.bitmap_no_memory
        push de                         ; -20 copied bitmap
        ldir
        ld l,-20(ix)
        ld h,-19(ix)
        push hl
        ld l,8(ix)
        ld h,9(ix)
        push hl
        ld l,6(ix)
        ld h,7(ix)
        push hl
        push ix
        pop hl
        ld de,#-14
        add hl,de
        ld e,-18(ix)
        ld d,-17(ix)
        call __kernel_gdrawbmp
        push af
        ld l,-20(ix)
        ld h,-19(ix)
        call _free
        pop af
        jr .bitmap_cleanup
.bitmap_no_memory:
        ld a,#1
.bitmap_cleanup:
        push af
        ld l,-16(ix)
        ld h,-15(ix)
        call _free
        pop af
        ld sp,ix
        pop iy
        pop ix
        pop bc
        pop hl
        pop hl
        pop hl
        push bc
        ret

        .globl __partner_load_process, __partner_load_process_args, __partner_load_library
        .globl __kernel__native_load_process_args, __kernel__native_load_library
        .globl __native_load_error
__partner_load_process::
        ld de,#0
__partner_load_process_args::
        ld a,#1
        jr .load
__partner_load_library::
        ld a,#2
.load:
        push ix
        ld ix,#0
        add ix,sp
        push af                         ; -1 kind
        push de                         ; -4 flags/argument pointer
        call .string_copy
        push de                         ; -6 path allocation
        ld a,d
        or e
        jr z,.load_nomem
        ld hl,#0
        ld a,-1(ix)
        cp #2
        jr z,.load_args_ready
        ld l,-4(ix)
        ld h,-3(ix)
        ld a,h
        or l
        jr z,.load_args_ready
        call .string_copy
        ld a,d
        or e
        jr z,.load_nomem
        ex de,hl
.load_args_ready:
        push hl                         ; -8 argument allocation
        ex de,hl
        ld l,-6(ix)
        ld h,-5(ix)
        ld a,-1(ix)
        cp #2
        jr nz,.load_process
        ld e,-4(ix)
        ld d,-3(ix)
        call __kernel__native_load_library
        jr .load_cleanup
.load_process:
        call __kernel__native_load_process_args
.load_cleanup:
        push de
        ld l,-8(ix)
        ld h,-7(ix)
        call _free
        ld l,-6(ix)
        ld h,-5(ix)
        call _free
        pop de
        ld sp,ix
        pop ix
        ret
.load_nomem:
        ld l,-6(ix)
        ld h,-5(ix)
        call _free
        ld a,#2
        ld (__native_load_error),a
        ld de,#0
        ld sp,ix
        pop ix
        ret
.string_copy:
        push hl
        ld bc,#1
.string_length:
        ld a,(hl)
        or a
        jr z,.string_size
        inc hl
        inc bc
        jr .string_length
.string_size:
        push bc
        ld h,b
        ld l,c
        call _malloc
        pop bc
        pop hl
        ld a,d
        or e
        ret z
        push de
        ldir
        pop de
        ret

        .globl __partner_query, __partner_register
        .globl __kernel__native_query, __kernel__native_register
__partner_query::
        push ix
        call .string_copy
        ld a,d
        or e
        jr z,.query_none
        push de
        ex de,hl
        call __kernel__native_query
        pop hl
        push de
        call _free
        pop de
.query_none:
        pop ix
        ret
__partner_register::
        push ix
        push de
        call .string_copy
        pop hl
        ld a,d
        or e
        jr z,.register_none
        push de
        ex de,hl
        call __kernel__native_register
        pop hl
        push de
        call _free
        pop de
.register_none:
        pop ix
        ret

        .globl __partner_stat, __partner_fstat, __kernel_stat, __kernel_fstat
        .globl ___sdcc_call_bc
__partner_stat::
        ld bc,#__kernel_stat
        jr .stat
__partner_fstat::
        ld bc,#__kernel_fstat
.stat:
        push ix
        push de                         ; client output
        ld ix,#-14
        add ix,sp
        ld sp,ix
        push ix
        pop de
        call ___sdcc_call_bc
        ld a,d
        or e
        jr nz,.stat_done
        ld e,14(ix)
        ld d,15(ix)
        push ix
        pop hl
        ld bc,#14
        ldir
        ld de,#0
.stat_done:
        ld hl,#16
        add hl,sp
        ld sp,hl
        pop ix
        ret

        .globl __partner_paint_window, __kernel_paint_window
        .globl __partner_paint_frame, __partner_paint_background
        .globl __kernel__window_paint_frame, __kernel__window_paint_background
__partner_paint_frame::
        ld bc,#__kernel__window_paint_frame
        jr .paint
__partner_paint_background::
        ld bc,#__kernel__window_paint_background
        jr .paint
__partner_paint_window::
        ld bc,#__kernel_paint_window
.paint:
        push bc
        push hl                         ; window
        ex de,hl
        call __partner_copy_region
        ex (sp),hl                      ; allocation; HL=window, DE=clip
        pop bc                          ; clip allocation
        ex (sp),hl                      ; window on stack, target in HL
        push bc                         ; allocation
        ld b,h
        ld c,l                          ; target
        pop hl                          ; allocation
        ex (sp),hl                      ; window, allocation on stack
        call ___sdcc_call_bc
        pop hl
        push af
        call _free
        pop af
        ret

        .globl __partner_popup_open, __kernel_popup_open
__partner_popup_open::
        push ix
        ld ix,#0
        add ix,sp
        push hl                         ; -2 owner
        push de                         ; -4 procedure
        push af                         ; -5 content bank
        ld hl,#-8
        add hl,sp
        ld sp,hl                        ; -14 anchor copy
        ex de,hl
        ld l,6(ix)
        ld h,7(ix)
        ld bc,#8
        ldir
        ld hl,#-9
        add hl,sp
        ld sp,hl
        ex de,hl
        push ix
        pop hl
        ld bc,#4
        add hl,bc
        ld bc,#9
        ldir                            ; duplicate original packed arguments
        push ix
        pop hl
        ld de,#-14
        add hl,de
        ld -21(ix),l                    ; replace borrowed anchor pointer
        ld -20(ix),h
        ld l,-2(ix)
        ld h,-1(ix)
        ld e,-4(ix)
        ld d,-3(ix)
        ld a,-5(ix)
        call __kernel_popup_open
        ld sp,ix
        pop ix
        pop bc
        ld hl,#9
        add hl,sp
        ld sp,hl
        push bc
        ret

        ;; Raster/compact headers carry a word size; legacy Tiny glyphs use
        ;; a byte count and start their stream at offset four.
        .globl __partner_bitmap_size
__partner_bitmap_size::
        push hl
        ld a,(hl)
        and #0xe0
        jr z,.size_legacy
        bit 0,(hl)
        jr nz,.size_word
.size_legacy:
        inc hl
        inc hl
        inc hl
        ld c,(hl)
        inc hl
        ld b,(hl)
        or a
        jr z,.bitmap_compact
        ld a,b
        or a
        jr z,.bitmap_compact
        ld b,#0
        inc bc
        inc bc
        inc bc
        inc bc
        jr .bitmap_sized
.size_word:
        ; New Tiny records explicitly carry a 16-bit stream length.
        inc hl
        inc hl
        inc hl
        ld c,(hl)
        inc hl
        ld b,(hl)
.bitmap_compact:
        inc bc
        inc bc
        inc bc
        inc bc
        inc bc
.bitmap_sized:
        pop hl
        ret

        .globl __partner_frame_area, __kernel__window_frame_area
__partner_frame_area::
        push ix
        ld ix,#0
        add ix,sp
        push de                         ; original client rectangle
        push hl                         ; window
        ld hl,#-8
        add hl,sp
        ld sp,hl                        ; -12 rectangle
        ex de,hl
        ld l,-2(ix)
        ld h,-1(ix)
        ld bc,#8
        ldir
        ld l,-4(ix)
        ld h,-3(ix)
        push ix
        pop de
        ld bc,#-12
        ex de,hl
        add hl,bc
        ex de,hl
        call __kernel__window_frame_area
        push af
        push ix
        pop hl
        ld bc,#-12
        add hl,bc
        ld e,-2(ix)
        ld d,-1(ix)
        ld bc,#8
        ldir
        pop af
        ld sp,ix
        pop ix
        ret

        .globl __partner_visible_region, __kernel_control_visible_region
__partner_visible_region::
        push ix
        ld ix,#0
        add ix,sp
        push hl                         ; -2 window
        push de                         ; -4 rectangle
        ld hl,#-11
        add hl,sp
        ld sp,hl                        ; -15 rectangle, -7 region header
        ex de,hl
        ld l,-4(ix)
        ld h,-3(ix)
        ld bc,#8
        ldir
        ld l,4(ix)
        ld h,5(ix)
        ld bc,#3
        ldir
        push ix
        pop hl
        ld bc,#-7
        add hl,bc
        push hl                         ; out
        ld bc,#-8
        add hl,bc
        ex de,hl                        ; rectangle
        ld l,-2(ix)
        ld h,-1(ix)
        call __kernel_control_visible_region
        push af
        push ix
        pop hl
        ld bc,#-7
        add hl,bc
        ld e,4(ix)
        ld d,5(ix)
        ld bc,#3
        ldir
        pop af
        ld sp,ix
        pop ix
        pop bc
        pop hl                          ; consume out
        push bc
        ret
