        ;; mdr_dbg.s
        ;;
        ;; ZX Spectrum Microdrive driver: debug counters entrypoint.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module mdr_dbg

        .equ    MDR_DBG_SZ, 10

        .area   _CODE
        ;; ------------------------------------------------------------
        ;; _mdr_dbg
        ;; Dispatch strategy:
        ;;   copy the last debug snapshot to caller buffer.
        ;;
        ;; Signature:
        ;;   uint8_t mdr_dbg(uint8_t drive, mdr_debug_t *out)
        ;;
        ;; Arguments:
        ;;   A  = drive number (reserved)
        ;;   DE = output buffer pointer
_mdr_dbg::
        push	af
        call	_ir_disable
        pop	af
        ld	hl,#dbg_op
        ld	b,#MDR_DBG_SZ
.dbg_copy:
        ld	a,(hl)
        ld	(de),a
        inc	hl
        inc	de
        djnz	.dbg_copy
        call	_ir_enable
        xor	a
        ld	l,a
        ret
