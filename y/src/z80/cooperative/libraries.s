        ;; Shared near services remain resident while applications are live.
        ;; Clear hook pointers before reclaiming idle libraries.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_libraries
        .optsdcc -mz80 sdcccall(1)
        .include "native.inc"
        .globl __native_library_begin, __native_library_abort, __native_library_gc
        .globl __native_processes, __native_memory_reap, __native_service_reap
        .globl __app_wnd_install_menu_hooks, __route_install_modal_hooks, _free
        .globl __partner_image_bank, __partner_image_free
        .area _BSS
.libraries: .ds 288                   ; 16 records: image + ABI owner bank at 16
        .area _CODE
        ;; HL=image -> DE=library owner descriptor, or NULL.
__native_library_begin::
        push ix
        ld ix,#.libraries
        ld b,#16
.find:
        ld a,0(ix)
        or 1(ix)
        jr z,.found
        ld de,#18
        add ix,de
        djnz .find
        pop ix
        ld de,#0
        ret
.found:
        ld 0(ix),l
        ld 1(ix),h
        ld a,(__partner_image_bank)
        ld 16(ix),a
        push ix
        pop de
        pop ix
        ret
        ;; IX=owner; caller retains/frees image on failed initialization.
__native_library_abort::
        call __native_service_reap
        call __native_memory_reap
        ld 0(ix),#0
        ld 1(ix),#0
        ret
__native_library_gc::
        push ix
        ld ix,#__native_processes
        ld b,#P_COUNT
.active:
        ld a,P_STATE(ix)
        or a
        jr nz,.done
        ld de,#P_BYTES
        add ix,de
        djnz .active
        ;; No client window/callback can now reach a service.
        ld hl,#0
        push hl
        ld de,#0
        call __app_wnd_install_menu_hooks
        ld hl,#0
        push hl
        push hl
        push hl
        ld de,#0
        call __route_install_modal_hooks
        ld ix,#.libraries
        ld b,#16
.release:
        push bc
        ld a,0(ix)
        or 1(ix)
        jr z,.next
        ld l,0(ix)
        ld h,1(ix)
        push hl
        call __native_library_abort
        pop hl
        ld a,16(ix)
        call __partner_image_free
        ld l,2(ix)
        ld h,3(ix)
        call _free
.next:
        ld de,#18
        add ix,de
        pop bc
        djnz .release
.done:
        pop ix
        ret
