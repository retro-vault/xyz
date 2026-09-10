        ;; mdr_detect_drives.s
        ;;
        ;; ZX Spectrum Microdrive driver: drive detection entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_detect_drives

        .globl  __mdr_select_drive
        .globl  __mdr_start_motor
        .globl  __mdr_wait_sync
        .globl  __mdr_wait_gap_sync
        .globl  __mdr_delay_1ms
        .globl  __mdr_stop_motor

        .equ    MD_CTRL, 0xef
        .equ    MD_R_BUSY, 0x08
        .equ    MD_R_WP, 0x01
        .equ    MD_R_SYNC_ANY, 0x12
        .equ    MD_R_GAP_ANY, 0x24
        .equ    MD_R_ACT_ANY, 0x36

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_detect_drives
        ;; Dispatch strategy:
        ;;   pass 1: formatted-media check (SYNC then GAP+SYNC).
        ;;   pass 2: presence probe via status activity, busy, and WP sampling.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_detect_drives(void)
        ;;
        ;; Arguments:
        ;;   (none)
_mdr_detect_drives::
        call	_enter_critical_section
        ld	e,#0                    ; e = detected drive count
        ld	d,#1                    ; d = current drive under test
.det_loop:
        ld	a,d
        call	__mdr_select_drive      ; select drive d
        call	__mdr_start_motor

        ;; allow tape to reach speed before probing status
        ld	b,#220                  ; ~220 ms
.det_spinup:
        call	__mdr_delay_1ms
        djnz	.det_spinup

        ;; phase 1: formatted media probe
        call	__mdr_wait_sync         ; formatted media: SYNC edge
        jr	c,.det_probe
        call	__mdr_wait_gap_sync     ; formatted media: GAP+SYNC boundary
        jr	c,.det_probe
        inc	e                       ; count this drive
        jr	.det_next

.det_probe:
        ;; phase 2: unformatted/blank probe
        ;; sample port activity for a short window:
        ;;   - SYNC/GAP transitions suggest moving media
        ;;   - BUSY high suggests drive mechanism responding
        ;;   - repeated WP-low samples suggest inserted writable cartridge
        ;; this avoids MD_DATA reads (safe on missing/invalid media)
        push	de
        ld	c,#MD_CTRL
        ld	b,#180                  ; ~180 ms sample window
        in	a,(c)
        and	#MD_R_ACT_ANY
        ld	h,a                     ; previous activity mask
        ld	l,#0                    ; transition count
        ld	e,#0                    ; wp-low sample count
.det_sync_poll:
        in	a,(c)
        ld	d,a                     ; keep full status byte
        and	#MD_R_BUSY
        jr	nz,.det_probe_hit       ; busy active => drive response seen
        ld	a,d
        and	#MD_R_ACT_ANY
        cp	h
        jr	z,.det_probe_wp
        ld	h,a
        inc	l
        ld	a,l
        cp	#3                      ; 3 transitions => activity present
        jr	nc,.det_probe_hit
.det_probe_wp:
        ld	a,d
        and	#MD_R_WP
        jr	nz,.det_probe_next
        inc	e
        ld	a,e
        cp	#24                     ; enough WP-low samples => inserted media
        jr	nc,.det_probe_hit
.det_probe_next:
        call	__mdr_delay_1ms
        djnz	.det_sync_poll
        pop	de
        jr	.det_next
.det_probe_hit:
        pop	de
        jr	.det_present

.det_present:
        inc	e
.det_next:
        call	__mdr_stop_motor
        inc	d
        ld	a,d
        cp	#9                      ; past drive 8?
        jr	nz,.det_loop
        ld	a,e                     ; return count in A
.det_ret:
        ld	l,a                     ; also in L for C return value
        call	_leave_critical_section
        ret
