        ;; YOS sequential stream adapter over baremetal libcpmfs, not BDOS.
        ;; Same register ABI as the CPC and CP/M-hosted Partner disk layer.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_yos_disk
        .globl _disk_open, _disk_close, _disk_read, _disk_write
        .globl _open, _close, _read, _write
        .globl __partner_path
        .area _BSS
.active: .ds 1
.fd: .ds 2
.mode: .ds 1
        .area _CODE
        ;; HL=path, A=0 read / 1 replace. A=0 success, FF failure.
_disk_open::
        cp #2
        jr nc,.error
        ld b,a
        ld a,(.active)
        or a
        jr nz,.error
        ld a,b
        ld (.mode),a
        call __partner_path
        or a
        jr nz,.error
        ld de,#0
        ld a,(.mode)
        or a
        jr z,.open
        ld de,#0x301
.open:
        call _open
        ld a,d
        and e
        inc a
        jr z,.error
        ld (.fd),de
        ld a,#1
        ld (.active),a
        xor a
        ret
_disk_close::
        ld a,(.active)
        or a
        ret z
        xor a
        ld (.active),a
        ld hl,(.fd)
        call _close
        ld a,d
        or e
        ret z
.error:
        ld a,#255
        ret
        ;; HL=buffer, BC=count -> BC=actual, A=0/1 EOF/FF error.
_disk_read::
        ld a,(.mode)
        or a
        jr nz,.transfer_error
        call .prepare
        ret nz
        push bc                        ; requested count, retained for EOF
        push bc                        ; callee-clean count
        call _read
        pop hl
        jr .result
_disk_write::
        ld a,(.mode)
        cp #1
        jr nz,.transfer_error
        call .prepare
        ret nz
        push bc
        push bc
        call _write
        pop hl
.result:
        ld b,d
        ld c,e
        ld a,d
        and e
        inc a
        jr z,.transfer_error
        or a
        sbc hl,de
        jr nz,.short
        xor a
        ret
.short:
        ;; Short writes are errors; short reads are EOF with a valid count.
        ld a,(.mode)
        or a
        jr nz,.error
        inc a
        ret
.prepare:
        ld a,(.active)
        or a
        jr z,.transfer_error
        ex de,hl
        ld hl,(.fd)
        xor a
        ret
.transfer_error:
        ld bc,#0
        jr .error
