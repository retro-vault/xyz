        ;; Partner Xebec S1410 programmed-I/O sector driver. No CP/M calls.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.
        .module partner_sasi
        .globl partner_hd_read, partner_hd_write
        .area _BSS
.error: .ds 1
        .area _CODE
        ;; A=LUN (0/1), DE=256-byte LBA, HL=buffer. A=0 success, 1 bad
        ;; argument, 2 timeout, 4 controller/protocol error. IX/IY preserved.
        ;; Serialized by YOS. No bank transition or allocator call during I/O.
partner_hd_read::
        ld c,#8
        jr .transfer
partner_hd_write::
        ld c,#10
.transfer:
        cp #2
        jr c,.valid
        ld a,#1
        or a
        ret
.valid:
        push ix
        push iy
        push hl
        push de
        push bc
        rlca
        rlca
        rlca
        rlca
        rlca
        ld b,a                         ; LUN in command byte 1
        xor a
        ld (.error),a
        out (0x10),a                   ; release any previous session
        ld a,#1
        out (0x11),a                   ; target select mask
        out (0x10),a
        push bc
        ld bc,#0xffff
.wait_busy:
        in a,(0x10)
        cp #0xff                       ; floating bus: no controller
        jr z,.busy_next
        and #8
        jr nz,.selected
.busy_next:
        dec bc
        ld a,b
        or c
        jr nz,.wait_busy
        pop bc
        pop bc
        pop de
        pop hl
        ld a,#2
        jr .end
.selected:
        pop bc
        ld a,#2                        ; data enable, drop SEL, no DMA
        out (0x10),a
        ld a,b
        pop bc
        pop de
        pop hl
        push hl                        ; buffer
        push bc                        ; opcode
        push af                        ; LUN
        push de                        ; LBA
        ld a,c
        call .command_byte
        pop de
        pop bc                         ; B = LUN
        push af                        ; keep command stack balanced
        ld a,b
        call .command_byte
        ld a,d
        call .command_byte
        ld a,e
        call .command_byte
        ld a,#1
        call .command_byte
        xor a
        call .command_byte
        pop de                         ; D = first result
        pop bc                         ; C = opcode
        pop hl
        ld a,(.error)
        or a
        jr nz,.end
        ld b,#0                        ; exactly 256 bytes
.data:
        push bc
        ld a,c
        cp #8
        ld a,#0x88                     ; REQ+BSY, data-out phase
        jr nz,.phase
        ld a,#0xc8                     ; REQ+BSY+I/O, data-in phase
.phase:
        call .wait_phase
        pop bc
        or a
        jr nz,.end
        ld a,c
        cp #8
        jr nz,.write
        in a,(0x11)
        ld (hl),a
        jr .next
.write:
        ld a,(hl)
        out (0x11),a
.next:
        inc hl
        djnz .data
        ld a,#0xd8                     ; status
        call .wait_phase
        or a
        jr nz,.end
        in a,(0x11)
        push af
        ld a,#0xf8                     ; command-complete message
        call .wait_phase
        pop bc
        or a
        jr nz,.end
        in a,(0x11)
        or b
        jr z,.end
        ld a,#4
.end:
        push af
        xor a
        out (0x10),a
        pop af
        pop iy
        pop ix
        or a
        ret
.command_byte:
        push bc
        push af
        ld a,(.error)
        or a
        jr nz,.command_failed
        ld a,#0x98
        call .wait_phase
        or a
        jr nz,.command_failed
        pop bc
        ld a,b
        out (0x11),a
        pop bc
        xor a
        ret
.command_failed:
        ld (.error),a
        pop bc
        pop bc
        ret
        ;; A=expected status. Preserve BC/DE/HL; bounded hardware waits.
.wait_phase:
        push bc
        push de
        ld e,a
        ld bc,#0xffff
.wait:
        in a,(0x10)
        cp e
        jr z,.ready
        cp #0xff
        jr z,.wait_next
        bit 7,a
        jr nz,.unexpected
.wait_next:
        dec bc
        ld a,b
        or c
        jr nz,.wait
        ld a,#2
        jr .wait_done
.unexpected:
        ld a,#4
        jr .wait_done
.ready:
        xor a
.wait_done:
        pop de
        pop bc
        ret
