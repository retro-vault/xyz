        ;; Disk-backed pages use the same 256-byte transfer API as the CPC
        ;; expansion banks. Executable banks are managed separately by YOS.
        ;; AP00.SWP..APBF.SWP are reserved scratch names on the current drive.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_page
        .optsdcc -mz80 sdcccall(1)
        .globl _page_read, _page_write, _page_count
        .globl _disk_open, _disk_close, _disk_read, _disk_write
        .area _DATA
.name: .asciz "AP00.SWP"
        .area _CODE
_page_count::
        ld de,#192
        ret
        ;; A=page, HL=256-byte buffer; A=0 success, FF failure.
_page_read::
        cp #192
        jr nc,.error
        push hl
        call .filename
        xor a
        call _disk_open
        pop hl
        or a
        ret nz
        ld bc,#256
        call _disk_read
        jr .finish
_page_write::
        cp #192
        jr nc,.error
        push hl
        call .filename
        ld a,#1
        call _disk_open
        pop hl
        or a
        ret nz
        ld bc,#256
        call _disk_write
.finish:
        or a
        jr nz,.failed_transfer
        ld a,b
        cp #1
        jr nz,.failed_transfer
        ld a,c
        or a
        jr nz,.failed_transfer
        jp _disk_close
.failed_transfer:
        call _disk_close
.error:
        ld a,#0xff
        ret
.filename:
        push af
        rrca
        rrca
        rrca
        rrca
        call .hex
        ld (.name+2),a
        pop af
        call .hex
        ld (.name+3),a
        ld hl,#.name
        ret
.hex:
        and #15
        add a,#48
        cp #58
        ret c
        add a,#7
        ret
