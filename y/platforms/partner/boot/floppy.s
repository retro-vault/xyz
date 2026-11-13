        ;; floppy.s
        ;;
        ;; Interrupt-driven, DMA-backed raw-sector I/O for the Iskra Delta
        ;; Partner i8272. The public calls wait synchronously for completion.
        ;; Classic 77x2x18 geometry and one outstanding operation are supported.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-10   tstih

        .module floppy

        .include "partner.inc"
        .include "floppy.inc"

        .globl  floppy_init
        .globl  floppy_read_sector
        .globl  floppy_write_sector
        .globl  floppy_last_st0
        .globl  floppy_last_st1
        .globl  floppy_last_st2
        .globl  floppy_last_st3
        .globl  interrupt_set
        .globl  _floppy_init
        .globl  _floppy_read_sector
        .globl  _floppy_write_sector
        .globl  _floppy_last_st0
        .globl  _floppy_last_st1
        .globl  _floppy_last_st2
        .globl  _floppy_last_st3

        .equ    FD_MSR_PHASE_MASK,         0xc0
        .equ    FD_MSR_COMMAND_READY,      0x80
        .equ    FD_MSR_RESULT_READY,       0xc0

        .equ    FD_CMD_SPECIFY,            0x03
        .equ    FD_CMD_SENSE_DRIVE,        0x04
        .equ    FD_CMD_RECALIBRATE,        0x07
        .equ    FD_CMD_SENSE_INTERRUPT,    0x08
        .equ    FD_CMD_SEEK,               0x0f
        .equ    FD_CMD_READ_MFM,           0x46
        .equ    FD_CMD_WRITE_MFM,          0x45

        .equ    FD_ST0_IC_MASK,            0xc0
        .equ    FD_ST0_NO_INTERRUPT,       0x80
        .equ    FD_ST1_END_OF_CYLINDER,    0x80
        .equ    FD_ST3_READY,              0x20
        .equ    FD_SECTOR_N,               1
        .equ    FD_RW_GPL,                 0x1b
        .equ    FD_RW_DTL,                 0xff
        .equ    FD_SPECIFY_SRT_HUT,        0xde
        .equ    FD_SPECIFY_HLT_DMA,        0x08

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; floppy_init
        ;; Configure the controller for DMA, install the completion ISR, start
        ;; the motor, check readiness, and recalibrate the selected drive.
        ;;
        ;; Arguments:
        ;;   A = physical drive (0..3)
        ;;
        ;; Returns: A = FLOPPY_OK/Z or error/NZ; interrupts enabled
        ;;
        ;; Clobbers: AF, BC, DE, HL
_floppy_init::
floppy_init::
        cp      #4
        jp      nc,fd_argument_error
        di
        ld      (fd_drive),a
        ld      a,#0xff
        ld      (fd_current_cylinder),a
        xor     a
        ld      (fd_cylinder),a         ; recalibrate always targets cylinder 0
        ld      (fd_irq_done),a

        ld      a,#FLOPPY_VECTOR
        ld      de,#fd_interrupt_handler
        call    interrupt_set
        ld      a,#FLOPPY_VECTOR
        out     (FLOPPY_PORT_VECTOR),a

        ld      a,#FD_CMD_SPECIFY
        call    fd_write_data
        jp      c,fd_timeout_error
        ld      a,#FD_SPECIFY_SRT_HUT
        call    fd_write_data
        jp      c,fd_timeout_error
        ld      a,#FD_SPECIFY_HLT_DMA
        call    fd_write_data
        jp      c,fd_timeout_error

        xor     a
        out     (FLOPPY_PORT_MOTOR),a
        call    fd_motor_delay

        ld      a,#FD_CMD_SENSE_DRIVE
        call    fd_write_data
        jp      c,fd_timeout_error
        ld      a,(fd_drive)
        call    fd_write_data
        jp      c,fd_timeout_error
        call    fd_read_data
        jp      c,fd_timeout_error
        ld      (floppy_last_st3),a
        and     #FD_ST3_READY
        jp      z,fd_io_error

        ld      a,#FD_CMD_RECALIBRATE
        call    fd_write_data
        jp      c,fd_timeout_error
        ld      a,(fd_drive)
        call    fd_write_data
        jp      c,fd_timeout_error
        call    fd_wait_seek
        jp      c,fd_seek_error
        xor     a
        ld      (fd_current_cylinder),a
        ei
        ret

        ;; ------------------------------------------------------------
        ;; floppy_read_sector
        ;; Read one physical 256-byte sector through the Z80 DMA.
        ;;
        ;; Arguments:
        ;;   DE = linear physical LBA (0..2771)
        ;;   HL = destination buffer
        ;;
        ;; Returns: A = FLOPPY_OK/Z or error/NZ; interrupts enabled
        ;;
        ;; Clobbers: AF, BC, DE, HL
_floppy_read_sector::
        ex      de,hl                   ; sdcccall(1): HL=LBA, DE=buffer
floppy_read_sector::
        xor     a                       ; direction 0 = FDC to memory
        jr      fd_transfer

        ;; ------------------------------------------------------------
        ;; floppy_write_sector
        ;; Write one physical 256-byte sector through the Z80 DMA.
        ;;
        ;; Arguments:
        ;;   DE = linear physical LBA (0..2771)
        ;;   HL = source buffer
        ;;
        ;; Returns: A = FLOPPY_OK/Z or error/NZ; interrupts enabled
        ;;
        ;; Clobbers: AF, BC, DE, HL
_floppy_write_sector::
        ex      de,hl                   ; sdcccall(1): HL=LBA, DE=buffer
floppy_write_sector::
        ld      a,#1                    ; direction 1 = memory to FDC

fd_transfer:
        di
        ld      (fd_direction),a
        ld      a,h
        or      l
        jp      z,fd_argument_error
        ld      (fd_buffer),hl
        call    fd_lba_to_chs
        jp      c,fd_argument_error
        ld      (fd_sector),a
        ld      a,b
        ld      (fd_head),a
        ld      a,c
        ld      (fd_cylinder),a

        ld      hl,#fd_current_cylinder
        cp      (hl)
        jr      z,.fd_transfer_positioned
        call    fd_seek
        jp      c,fd_seek_error
        ld      a,(fd_cylinder)
        ld      (fd_current_cylinder),a

.fd_transfer_positioned:
        ld      hl,(fd_buffer)
        call    fd_dma_setup
        xor     a
        ld      (fd_irq_done),a
        ld      (fd_irq_status),a
        call    fd_issue_rw
        jp      c,fd_timeout_error

        ;; Completion can already be pending after a polled seek. Check both
        ;; the ISR flag and result phase; never HALT without a running clock.
        ld      bc,#0xffff
.fd_wait_completion:
        ei
        nop
        di
        ld      a,(fd_irq_done)
        or      a
        jr      nz,.fd_complete
        in      a,(FLOPPY_PORT_DMA)
        bit     5,a
        jr      nz,.fd_wait_next
        in      a,(FLOPPY_PORT_MSR)
        and     #FD_MSR_PHASE_MASK
        cp      #FD_MSR_RESULT_READY
        jr      z,.fd_poll_complete
.fd_wait_next:
        dec     bc
        ld      a,b
        or      c
        jr      nz,.fd_wait_completion
        ld      a,#0x83                ; disable outstanding DMA on timeout
        out     (FLOPPY_PORT_DMA),a
        jp      fd_timeout_error
.fd_poll_complete:
        call    fd_finish_rw
        ld      (fd_irq_status),a
.fd_complete:
        ld      a,(fd_irq_status)
        or      a
        ei
        ret

        ;; ------------------------------------------------------------
        ;; fd_lba_to_chs
        ;; Convert classic Partner physical LBA to i8272 C/H/R.
        ;;
        ;; Arguments:
        ;;   DE = LBA
        ;;
        ;; Returns: C = cylinder, B = head, A = 1-based sector, CF on range
        ;;
        ;; Clobbers: AF, DE
fd_lba_to_chs:
        ld      a,d
        cp      #0x0a
        jr      c,.fd_lba_valid
        jr      nz,.fd_lba_invalid
        ld      a,e
        cp      #0xd4                   ; 2772 = 0x0ad4, exclusive
        jr      nc,.fd_lba_invalid
.fd_lba_valid:
        ld      c,#0
.fd_lba_cylinder:
        ld      a,d
        or      a
        jr      nz,.fd_lba_subtract
        ld      a,e
        cp      #(PARTNER_FD_HEADS * PARTNER_FD_SPT)
        jr      c,.fd_lba_head
.fd_lba_subtract:
        ld      a,e
        sub     #(PARTNER_FD_HEADS * PARTNER_FD_SPT)
        ld      e,a
        jr      nc,.fd_lba_no_borrow
        dec     d
.fd_lba_no_borrow:
        inc     c
        jr      .fd_lba_cylinder
.fd_lba_head:
        ld      b,#0
        ld      a,e
        cp      #PARTNER_FD_SPT
        jr      c,.fd_lba_sector
        sub     #PARTNER_FD_SPT
        inc     b
.fd_lba_sector:
        inc     a
        or      a                       ; clear carry
        ret
.fd_lba_invalid:
        scf
        ret

        ;; ------------------------------------------------------------
        ;; fd_seek
        ;; Seek the selected drive to fd_cylinder and verify completion.
        ;;
        ;; Returns: CF set on controller or seek failure
        ;;
        ;; Clobbers: AF, BC, DE, HL
fd_seek:
        ld      a,#FD_CMD_SEEK
        call    fd_write_data
        ret     c
        call    fd_head_unit
        call    fd_write_data
        ret     c
        ld      a,(fd_cylinder)
        call    fd_write_data
        ret     c
        jp      fd_wait_seek

        ;; ------------------------------------------------------------
        ;; fd_wait_seek
        ;; Poll SENSE INTERRUPT STATUS until seek/recalibrate completion.
        ;;
        ;; Returns: CF clear for normal completion, set on failure/timeout
        ;;
        ;; Clobbers: AF, BC, DE, HL
fd_wait_seek:
        ld      b,#0                    ; up to 256 sense attempts
.fd_wait_seek_loop:
        push    bc
        ld      a,#FD_CMD_SENSE_INTERRUPT
        call    fd_write_data
        jr      c,.fd_wait_seek_pop_fail
        call    fd_read_data
        jr      c,.fd_wait_seek_pop_fail
        ld      d,a                     ; ST0
        call    fd_read_data            ; PCN
        jr      c,.fd_wait_seek_pop_fail
        ld      e,a
        pop     bc
        ld      a,d
        cp      #FD_ST0_NO_INTERRUPT
        jr      z,.fd_wait_seek_again
        and     #FD_ST0_IC_MASK
        jr      nz,.fd_wait_seek_fail
        ld      a,(fd_cylinder)
        cp      e
        jr      nz,.fd_wait_seek_fail
        or      a
        ret
.fd_wait_seek_again:
        djnz    .fd_wait_seek_loop
.fd_wait_seek_fail:
        scf
        ret
.fd_wait_seek_pop_fail:
        pop     bc
        scf
        ret

        ;; ------------------------------------------------------------
        ;; fd_dma_setup
        ;; Program the exact Partner-ROM Z80-DMA byte stream for 256 bytes.
        ;;
        ;; Arguments:
        ;;   HL = memory buffer
        ;;
        ;; Returns: nothing
        ;;
        ;; Clobbers: AF, BC, HL
fd_dma_setup:
        ld      a,#0x05                 ; reset DMA
        out     (FLOPPY_PORT_DMA),a
        ld      a,#0xcf                 ; continuous mode setup
        out     (FLOPPY_PORT_DMA),a
        ld      a,(fd_direction)
        or      a
        ld      a,#0x79                 ; port B (FDC) -> port A (RAM)
        jr      z,.fd_dma_direction
        ld      a,#0x7d                 ; port A (RAM) -> port B (FDC)
.fd_dma_direction:
        out     (FLOPPY_PORT_DMA),a
        ld      a,l
        out     (FLOPPY_PORT_DMA),a
        ld      a,h
        out     (FLOPPY_PORT_DMA),a
        ld      a,#255
        out     (FLOPPY_PORT_DMA),a     ; Z80 DMA length = byte count minus one
        xor     a
        out     (FLOPPY_PORT_DMA),a     ; 00FF transfers exactly 256 bytes
        ld      hl,#fd_dma_tail
        ld      b,#7
        ld      c,#FLOPPY_PORT_DMA
        otir
        ret

fd_dma_tail:
        ;; FDC completion owns vector 0xe8, so the DMA must not request its own
        ;; end-of-block interrupt. This is the PartOS/Partner byte stream.
        .db     0x14,0x28,0x85,FLOPPY_PORT_DATA,0x8a
        .db     0xcf,0x87               ; LOAD, ENABLE; retain WR0 direction

        ;; ------------------------------------------------------------
        ;; fd_issue_rw
        ;; Send a one-sector MFM READ DATA or WRITE DATA command.
        ;;
        ;; Returns: CF set if an FDC command phase times out
        ;;
        ;; Clobbers: AF, BC, DE, HL
fd_issue_rw:
        ld      a,(fd_direction)
        or      a
        ld      a,#FD_CMD_READ_MFM
        jr      z,.fd_issue_opcode
        ld      a,#FD_CMD_WRITE_MFM
.fd_issue_opcode:
        call    fd_write_data
        ret     c
        call    fd_head_unit
        call    fd_write_data
        ret     c
        ld      a,(fd_cylinder)
        call    fd_write_data
        ret     c
        ld      a,(fd_head)
        call    fd_write_data
        ret     c
        ld      a,(fd_sector)
        call    fd_write_data
        ret     c
        ld      a,#FD_SECTOR_N
        call    fd_write_data
        ret     c
        ld      a,(fd_sector)            ; EOT=R limits transfer to one sector
        call    fd_write_data
        ret     c
        ld      a,#FD_RW_GPL
        call    fd_write_data
        ret     c
        ld      a,#FD_RW_DTL
        jp      fd_write_data

        ;; ------------------------------------------------------------
        ;; fd_finish_rw
        ;; Drain all seven i8272 result bytes and validate ST0/ST1/ST2. This is
        ;; called from the FDC ISR, as in PARTOS, so acknowledging completion
        ;; and consuming the result phase are one interrupt operation.
        ;;
        ;; Returns: A = FLOPPY_OK/Z or error/NZ
        ;;
        ;; Clobbers: AF, BC, DE, HL
fd_finish_rw:
        call    fd_read_data
        jr      c,.fd_finish_timeout
        ld      (floppy_last_st0),a
        call    fd_read_data
        jr      c,.fd_finish_timeout
        ld      (floppy_last_st1),a
        call    fd_read_data
        jr      c,.fd_finish_timeout
        ld      (floppy_last_st2),a
        ld      b,#4                    ; drain C, H, R, N
.fd_finish_drain:
        push    bc
        call    fd_read_data
        pop     bc
        jr      c,.fd_finish_timeout
        djnz    .fd_finish_drain
        ld      a,(floppy_last_st0)
        and     #FD_ST0_IC_MASK
        jr      nz,.fd_finish_error
        ld      a,(floppy_last_st1)
        and     #(0xff ^ FD_ST1_END_OF_CYLINDER)
        ld      hl,#floppy_last_st2
        or      (hl)
        jr      nz,.fd_finish_error
        xor     a
        ret
.fd_finish_timeout:
        ld      a,#FLOPPY_ERR_TIMEOUT
        or      a
        ret
.fd_finish_error:
        ld      a,#FLOPPY_ERR_IO
        or      a
        ret

        ;; ------------------------------------------------------------
        ;; fd_head_unit
        ;; Build the i8272 head/unit byte for the current operation.
        ;;
        ;; Returns: A = (head << 2) | drive
        ;;
        ;; Clobbers: AF
fd_head_unit:
        ld      a,(fd_head)
        add     a,a
        add     a,a
        ld      hl,#fd_drive
        or      (hl)
        ret

        ;; ------------------------------------------------------------
        ;; fd_write_data / fd_read_data
        ;; Timed i8272 command/result phase accessors.
        ;;
        ;; Returns: byte in A for read; CF set on timeout
        ;;
        ;; Clobbers: AF, BC
fd_write_data:
        push    af
        ld      a,#FD_MSR_COMMAND_READY
        call    fd_wait_phase
        jr      c,.fd_write_timeout
        pop     af
        out     (FLOPPY_PORT_DATA),a
        or      a
        ret
.fd_write_timeout:
        pop     af
        scf
        ret

fd_read_data:
        ld      a,#FD_MSR_RESULT_READY
        call    fd_wait_phase
        ret     c
        in      a,(FLOPPY_PORT_DATA)
        or      a
        ret

fd_wait_phase:
        ld      e,a
        ld      bc,#0xffff
.fd_wait_phase_loop:
        in      a,(FLOPPY_PORT_MSR)
        and     #FD_MSR_PHASE_MASK
        cp      e
        ret     z
        dec     bc
        ld      a,b
        or      c
        jr      nz,.fd_wait_phase_loop
        scf
        ret

        ;; ------------------------------------------------------------
        ;; fd_motor_delay
        ;; Conservative spin-up delay without relying on an initialized timer.
        ;;
        ;; Returns: nothing
        ;;
        ;; Clobbers: B, C
fd_motor_delay:
        ld      c,#0
.fd_motor_outer:
        ld      b,#0
.fd_motor_inner:
        djnz    .fd_motor_inner
        dec     c
        jr      nz,.fd_motor_outer
        ret

        ;; ------------------------------------------------------------
        ;; fd_interrupt_handler
        ;; Complete the DMA-backed request in interrupt context. Drain the FDC
        ;; result phase here, publish its status, and then wake the synchronous
        ;; public call by setting fd_irq_done. Saving every primary register
        ;; keeps unrelated foreground code safe if another client later uses
        ;; the same ISR with a non-HALT wait.
        ;;
        ;; Returns: through RETI with interrupts enabled
        ;;
        ;; Clobbers: none
fd_interrupt_handler:
        push    af
        push    bc
        push    de
        push    hl
        in      a,(FLOPPY_PORT_MSR)
        and     #FD_MSR_PHASE_MASK
        cp      #FD_MSR_RESULT_READY
        jr      nz,.fd_irq_done
        call    fd_finish_rw
        ld      (fd_irq_status),a
        ld      a,#1                    ; status is visible before completion
        ld      (fd_irq_done),a
.fd_irq_done:
        pop     hl
        pop     de
        pop     bc
        pop     af
        ei
        reti

fd_argument_error:
        ld      a,#FLOPPY_ERR_ARGUMENT
        jr      fd_return_error
fd_timeout_error:
        ld      a,#FLOPPY_ERR_TIMEOUT
        jr      fd_return_error
fd_seek_error:
        ld      a,#FLOPPY_ERR_SEEK
        jr      fd_return_error
fd_io_error:
        ld      a,#FLOPPY_ERR_IO
fd_return_error:
        ei
        or      a
        ret

        ;; Single-operation state. This library is deliberately synchronous
        ;; and non-reentrant; an OS can wrap it in its own lock if needed.
fd_drive:
        .db     0x00
fd_direction:
        .db     0x00
fd_buffer:
        .dw     0x0000
fd_cylinder:
        .db     0x00
fd_current_cylinder:
        .db     0xff
fd_head:
        .db     0x00
fd_sector:
        .db     0x00
fd_irq_done:
        .db     0x00
fd_irq_status:
        .db     FLOPPY_OK
_floppy_last_st0::
floppy_last_st0::
        .db     0x00
_floppy_last_st1::
floppy_last_st1::
        .db     0x00
_floppy_last_st2::
floppy_last_st2::
        .db     0x00
_floppy_last_st3::
floppy_last_st3::
        .db     0x00
