        ;; CPC data disks: 40 tracks, nine 512-byte sectors, IDs C1..C9.
        ;; libcpmfs borrows 256-byte sectors; cache/RMW preserves the other half.
        .module cpc_fdc
        .globl floppy_read_sector, floppy_write_sector, floppy_init
        .globl partner_hd_read, partner_hd_write
        .area _KERNEL_BSS
.cache: .ds 512
.cached: .ds 2
.valid: .ds 1
.started: .ds 1
.track: .ds 1
.target: .ds 1
.half: .ds 1
.buffer: .ds 2
.lba: .ds 2
.write: .ds 1
.command: .ds 9
.globl __cpc_fdc_result
__cpc_fdc_result::
.result: .ds 7
        .area _CODE
floppy_init::
        or a
        ret z
partner_hd_read::
partner_hd_write::
        ld a,#1
        ret
floppy_read_sector::
        xor a
        jr .transfer
floppy_write_sector::
        ld a,#1
.transfer:
        ld (.write),a
        ld (.buffer),hl
        ld a,e
        and #1
        ld (.half),a
        srl d
        rr e
        ld (.lba),de
        ld a,i
        push af
        di
        ld a,(.started)
        or a
        call z,.start
        jp c,.failed
        ld hl,(.cached)
        ld de,(.lba)
        or a
        sbc hl,de
        jr nz,.fetch
        ld a,(.valid)
        or a
        jr nz,.copy
.fetch:
        xor a
        ld (.valid),a
        call .position
        jp c,.failed
        ld a,#0x46
        call .sector
        jp c,.failed
        ld hl,(.lba)
        ld (.cached),hl
        ld a,#1
        ld (.valid),a
.copy:
        ld hl,#.cache
        ld a,(.half)
        add a,h
        ld h,a
        ld de,(.buffer)
        ld bc,#256
        ld a,(.write)
        or a
        jr nz,.put
        ldir
        jr .ok
.put:
        ex de,hl
        ldir
        call .position
        jr c,.failed
        ld a,#0x45
        call .sector
        jr c,.failed
.ok:
        pop af
        jp po,.ok_di
        ei
.ok_di:
        xor a
        ret
.failed:
        xor a
        ld (.valid),a
        pop af
        jp po,.error
        ei
.error:
        ld a,#1
        ret
.start:
        ld bc,#0xfa7e
        ld a,#1
        out (c),a
        ;; Motor settling delay, approximately half a second at 4 MHz.
        ld hl,#0
.spin:
        dec hl
        ld a,h
        or l
        jr nz,.spin
        ld a,#3
        call .out
        ret c
        ld a,#0xdf
        call .out
        ret c
        ld a,#3                       ; non-DMA mode
        call .out
        ret c
        ld a,#7                       ; recalibrate
        call .out
        ret c
        xor a
        call .out
        ret c
        xor a
        ld (.target),a
        call .seek_done
        ret c
        ld a,#1
        ld (.started),a
        xor a
        ld (.track),a
        ret
.position:
        ld hl,(.lba)
        ld de,#9
        ld a,#0
.divide:
        or a
        sbc hl,de
        jr c,.divided
        inc a
        jr .divide
.divided:
        add hl,de
        ld (.target),a
        ld (.command+2),a
        ld a,l
        add a,#0xc1
        ld (.command+4),a
        ld (.command+6),a
        ld a,(.track)
        ld hl,#.target
        cp (hl)
        ret z
        ld a,#15                      ; seek drive 0 to cylinder
        call .out
        ret c
        xor a
        call .out
        ret c
        ld a,(.target)
        call .out
        ret c
        call .seek_done
        ret c
        ld a,(.target)
        ld (.track),a
        or a
        ret
.seek_done:
        ld de,#0xffff
.sense:
        ld a,#8                       ; sense interrupt status
        call .out
        ret c
        call .in
        ret c
        bit 5,a                       ; seek end
        jr nz,.sense_end
        cp #0x80                      ; no pending interrupt => one-byte result
        jr z,.sense_next
        call .in
        ret c
.sense_next:
        dec de
        ld a,d
        or e
        jr nz,.sense
        scf
        ret
.sense_end:
        and #0xc0
        jp nz,.sense_error
        call .in
        ret c
        ld hl,#.target
        cp (hl)
        ret z
.sense_error:
        scf
        ret
.sector:
        ld (.command),a
        xor a
        ld (.command+1),a
        ld (.command+3),a
        ld a,#2
        ld (.command+5),a
        ld a,#0x2a
        ld (.command+7),a
        ld a,#0xff
        ld (.command+8),a
        ld hl,#.command
        ld d,#9
.send:
        ld a,(hl)
        call .out
        ret c
        inc hl
        dec d
        jr nz,.send
        ;; Keep the data-register loop below the 128-T byte interval.
        ;; INI/OUTI change B, so restore it before polling FB7E again.
        ;; The not-ready path must also poll within that interval: testing
        ;; IX halves inline avoids a 125-T wait loop plus data-read latency.
        push ix
        ld ix,#0xffff
        ld hl,#.cache
        ld de,#0x0200
        ld bc,#0xfb7e
        ld a,(.command)
        cp #0x45
        jr z,.write_bytes
.read_bytes:
        in a,(c)
        jp p,.read_wait
        bit 6,a
        jr z,.data_error
        inc c
        ini
        inc b
        dec c
        dec e
        jr nz,.read_bytes
        dec d
        jr nz,.read_bytes
        jr .data_done
.write_bytes:
        in a,(c)
        jp p,.write_wait
        bit 6,a
        jr nz,.data_error
        inc c
        inc b                         ; OUTI decrements B before the port write
        outi
        dec c
        dec e
        jr nz,.write_bytes
        dec d
        jr nz,.write_bytes
.data_done:
        pop ix
        jr .results
.read_wait:
        dec ix
        .db 0xdd,0x7c                  ; LD A,IXH
        .db 0xdd,0xb5                  ; OR IXL
        jr nz,.read_bytes
        jr .data_error
.write_wait:
        dec ix
        .db 0xdd,0x7c                  ; LD A,IXH
        .db 0xdd,0xb5                  ; OR IXL
        jr nz,.write_bytes
.data_error:
        pop ix
        scf
        ret
.results:
        ld hl,#.result
        ld d,#7
.result_next:
        call .in
        ret c
        ld (hl),a
        inc hl
        dec d
        jr nz,.result_next
        ld a,(.result)
        and #0xc0
        jp nz,.sense_error
        ld hl,#.result+1
        ld a,(hl)
        inc hl
        or (hl)
        jp nz,.sense_error
        ret
        ;; Bounded controller handshake; preserve the transfer pointer/count.
.in:
        push de
        ld de,#0xffff
        ld bc,#0xfb7e
.in_wait:
        in a,(c)
        and #0xc0
        cp #0xc0
        jr z,.in_ready
        dec de
        ld a,d
        or e
        jr nz,.in_wait
.timeout:
        pop de
        scf
        ret
.in_ready:
        inc c
        in a,(c)
        pop de
        or a
        ret
.out:
        push de
        push af
        ld de,#0xffff
        ld bc,#0xfb7e
.out_wait:
        in a,(c)
        and #0xc0
        cp #0x80
        jr z,.out_ready
        dec de
        ld a,d
        or e
        jr nz,.out_wait
        pop af
        jr .timeout
.out_ready:
        pop af
        inc c
        out (c),a
        pop de
        or a
        ret
