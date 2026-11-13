        ;; mouse.s
        ;;
        ;; Alto's fixed Microsoft mouse on the Partner LPT serial channel.
        ;; This specializes the SDK transport for the configuration hid.c and
        ;; hid.s actually request: 2400 baud, seven data bits, one stop bit,
        ;; no flow control, receive-only streaming. One allocated owner block
        ;; holds the mouse, saved gateway bytes and private interrupt stack.
        ;; SDK helpers retain CP/M bank switching, IM2 install/restore, mouse
        ;; calibration, atomic snapshots and lossless button-edge latches.
        ;;
        ;; There is no replacement of the bank gateway with a polling shortcut:
        ;; mouse packets continue to accumulate while GDP drawing blocks the UI.
        ;; C HID still calls the SDK with these identical constant settings.
        ;; MIT License (see LICENSE), Copyright (C) 2026 Tomaz Stih.

        .module partner_mouse
        .optsdcc -mz80 sdcccall(1)
        .globl  _mouse_init_on, __serial_interrupt_dispatch
        .globl  _calloc, _free, _mouse_done
        .globl  __serial_interrupt_prepare, __serial_interrupt_install
        .globl  __serial_interrupt_remove, __serial_interrupt_restore_gateway
        .globl  __serial_interrupt_lock, __serial_interrupt_unlock
        .globl  __serial_interrupt_stack_top
        .globl  __serial_interrupt_port, __serial_interrupt_counter
        .globl  __serial_common_mouse_apply
        .globl  __partner_mouse_drain

        ;; SDK packed structures: only the fields used by this specialization.
        ;; mouse_t's public 52-byte layout is checked by the HID test fixture.
        .equ    M_SERIAL,                2
        .equ    M_PACKET,               13
        .equ    M_COUNT,                18
        .equ    M_SYNCED,               19
        .equ    M_MIN_X,                20
        .equ    M_MIN_Y,                22
        .equ    M_MAX_X,                24
        .equ    M_MAX_Y,                26
        .equ    S_WR5,                  24
        .equ    S_DATA,                 31
        .equ    S_CONTROL,              32
        .equ    S_MOUSE,                44
        .area   _CODE

        ;; SDK bank-gateway layout after the 31-byte public serial header.
        ;; 50..101 mouse, 102..103 saved RTC state, 104..225 saved page-zero
        ;; images (61 bytes per bank), then a private 128-byte interrupt stack.
        ;; No general-purpose TX/RX rings are needed by a streaming decoder.
        .equ    S_TX_VECTOR,             33
        .equ    S_RX_VECTOR,             34
        .equ    S_SPECIAL_VECTOR,        35
        .equ    S_MOUSE_STORAGE,         50
        .equ    S_RTC_CONTROL,          102
        .equ    S_RTC_MASKED,           103
        .equ    S_HEADER_BYTES,         226
        .equ    S_STACK_BYTES,          128

        ;; SDK ABI: HL=protocol(0), DE=port(1) -> DE=mouse or NULL.
        ;; Alto's two call sites pass these fixed settings. Keep exclusive
        ;; gateway ownership, allocation failure and enabled-IRQ preconditions;
        ;; all other configuration is immutable and written once from a table.
_mouse_init_on::
        push    ix
        push    iy
        ld      hl,(__serial_interrupt_port)
        ld      a,h
        or      l
        jp      nz,.mouse_open_fail
        call    __serial_interrupt_lock
        ld      e,a
        call    __serial_interrupt_unlock
        ld      a,e
        or      a
        jp      z,.mouse_open_fail
        ld      hl,#1
        ld      de,#S_HEADER_BYTES+S_STACK_BYTES
        call    _calloc
        ld      a,d
        or      e
        jp      z,.mouse_open_done
        push    de
        pop     ix                      ; zeroed owner block, including saved banks
        ld      hl,#S_HEADER_BYTES+S_STACK_BYTES
        add     hl,de
        ld      (__serial_interrupt_stack_top),hl
        ld      hl,#0
        ld      (__serial_interrupt_counter),hl
        ld      S_WR5(ix),#0xea
        ld      25(ix),#1               ; LPT is SIO1 channel B
        ld      27(ix),#2               ; no flow control
        ld      S_DATA(ix),#0xda
        ld      S_CONTROL(ix),#0xdb
        ld      hl,#S_MOUSE_STORAGE
        add     hl,de
        ld      S_MOUSE(ix),l
        ld      S_MOUSE+1(ix),h
        push    hl
        pop     iy
        ld      M_SERIAL(iy),e
        ld      M_SERIAL+1(iy),d
        ;; calloc has already cleared every decoder, motion and edge field.
        ld      M_MIN_X+1(iy),#0x80
        ld      M_MIN_Y+1(iy),#0x80
        ld      M_MAX_X(iy),#0xff
        ld      M_MAX_X+1(iy),#0x7f
        ld      M_MAX_Y(iy),#0xff
        ld      M_MAX_Y+1(iy),#0x7f

        ;; The RTC can deliver an NMI while transient TPA code owns IM2. Keep
        ;; its counters running, save/mask the optional source, and acknowledge
        ;; any already-latched edge before installing the mirrored gateway.
        in      a,(0xb1)
        ld      S_RTC_CONTROL(ix),a
        xor     a
        out     (0xb1),a
        in      a,(0xb0)
        ld      S_RTC_MASKED(ix),#1
        ld      hl,#__mouse_config
        ld      bc,#0x0adb              ; ten bytes to the LPT control register
        otir
        ld      a,#2
        out     (0xdb),a
        in      a,(0xdb)                ; SIO1 shared vector base
        and     #0xf1
        ld      S_TX_VECTOR(ix),a
        or      #4
        ld      S_RX_VECTOR(ix),a
        or      #2
        ld      S_SPECIAL_VECTOR(ix),a
        push    ix
        pop     hl
        call    __serial_interrupt_prepare
        push    ix
        pop     hl
        call    __serial_interrupt_install
        ld      (__serial_interrupt_port),ix
        ld      a,#1
        out     (0xdb),a
        ld      a,#0x1e                 ; same RX/TX/status causes as the SDK
        out     (0xdb),a
        ;; Power the mouse only after pointers, calibration and vectors exist.
        ld      a,#5
        out     (0xdb),a
        ld      a,#0xea
        out     (0xdb),a
        push    iy
        pop     de
        jr      .mouse_open_done
.mouse_open_fail:
        ld      de,#0
.mouse_open_done:
        pop     iy
        pop     ix
        ret
__mouse_config:
        .db     0x30,0x18               ; error reset, channel reset
        .db     4,0xc4                  ; 2400 baud, one stop, no parity
        .db     5,0x68                  ; transmitter enabled, mouse power off
        .db     3,0x41                  ; seven-bit receiver enabled
        .db     1,0x04                  ; status vector, interrupts initially off

        ;; HL=mouse or NULL. Disable the owned channel before restoring its
        ;; original IM2 entries and both page-zero images. Only then restore
        ;; the saved RTC source and the caller's interrupt-enable state; free
        ;; the single owner block after no interrupt can reference it again.
_mouse_done::
        ld      a,h
        or      l
        ret     z
        push    ix
        inc     hl
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        push    de
        pop     ix
        call    __serial_interrupt_lock
        push    af
        ld      hl,#__mouse_unconfig
        ld      bc,#0x05db
        otir
        push    ix
        pop     hl
        call    __serial_interrupt_remove
        push    ix
        pop     hl
        call    __serial_interrupt_restore_gateway
        in      a,(0xb0)                ; acknowledge an edge accumulated masked
        ld      a,S_RTC_CONTROL(ix)
        out     (0xb1),a
        ld      S_RTC_MASKED(ix),#0
        pop     af
        call    __serial_interrupt_unlock
        push    ix
        pop     hl
        call    _free
        pop     ix
        ret
__mouse_unconfig:
        .db     1,0x04,0x28,5,0x68      ; IRQs off, TX pending reset, power off

        ;; The SDK's mirrored page-zero gateway has already selected the TPA
        ;; bank and its private interrupt stack. Preserve every primary and
        ;; alternate register before touching state; RET returns to that same
        ;; gateway, which restores the interrupted bank, SP, AF, IFF and RETI.
        ;; A TX-empty interrupt can occur during setup, so acknowledge it even
        ;; though this configured mouse never transmits protocol requests.
__serial_interrupt_dispatch::
        push    af
        push    bc
        push    de
        push    hl
        ex      af,af'
        push    af
        exx
        push    bc
        push    de
        push    hl
        push    ix
        push    iy
        ld      hl,(__serial_interrupt_counter)
        inc     hl
        ld      (__serial_interrupt_counter),hl
        ld      ix,(__serial_interrupt_port)
        call    .mouse_drain
        pop     iy
        pop     ix
        pop     hl
        pop     de
        pop     bc
        exx
        pop     af
        ex      af,af'
        pop     hl
        pop     de
        pop     bc
        pop     af
        ret

        ;; Recover a pending receive byte after disk/DMA activity even when
        ;; no new SIO interrupt arrives. Use the identical packet decoder,
        ;; locked against the ISR; retain interrupt capture during drawing.
        ;; Clobbers AF/BC/DE/HL, preserves IX/IY and the caller's IFF.
__partner_mouse_drain::
        ld      hl,(__serial_interrupt_port)
        ld      a,h
        or      l
        ret     z
        push    ix
        push    iy
        push    hl
        pop     ix
        call    __serial_interrupt_lock
        push    af
        call    .mouse_drain
        pop     af
        call    __serial_interrupt_unlock
        pop     iy
        pop     ix
        ret

.mouse_drain:
.mouse_irq_poll:
        ld      c,S_CONTROL(ix)
        in      a,(c)
        rrca                            ; RR0 bit zero: received byte available
        jr      c,.mouse_irq_receive
        ld      a,#0x28                 ; reset TX-empty interrupt pending
        out     (c),a
        jr      .mouse_irq_done
.mouse_irq_receive:
        ld      c,S_DATA(ix)
        in      e,(c)
        ld      l,S_MOUSE(ix)
        ld      h,S_MOUSE+1(ix)
        ld      a,h
        or      l
        jr      z,.mouse_irq_ack         ; setup can interrupt before attachment
        push    hl
        pop     iy
        call    __partner_mouse_decode
.mouse_irq_ack:
        ld      c,S_CONTROL(ix)
        ld      a,#0x30                 ; clear framing/parity/overrun RR1 latch
        out     (c),a
        ;; Disk I/O can defer reception long enough for several bytes to
        ;; accumulate. Drain the FIFO before RETI instead of depending on a
        ;; second interrupt for a character which is already buffered.
        jr      .mouse_irq_poll
.mouse_irq_done:
        ret

        ;; IY=mouse, E=serial byte. Microsoft packets are H X Y; header bit 6
        ;; permits immediate resynchronization after a missing/noisy byte.
        ;; Header bits 1:0 and 3:2 supply the signed X/Y high bits, while the
        ;; following bytes supply six low bits. Left/right are header bits5/4.
        ;; The common apply routine retains exact SDK clipping, accumulated
        ;; deltas and pressed/released positions, including clicks between polls.
__partner_mouse_decode::
        bit     6,e
        jr      z,.mouse_data
        ld      M_PACKET(iy),e
        ld      M_COUNT(iy),#1
        ld      M_SYNCED(iy),#1
        ret
.mouse_data:
        ld      a,M_COUNT(iy)
        or      a
        ret     z                       ; ignore unsynchronized data bytes
        dec     a
        jr      nz,.mouse_packet_ready
        ld      M_PACKET+1(iy),e
        inc     M_COUNT(iy)
        ret
.mouse_packet_ready:
        ld      M_PACKET+2(iy),e
        ld      a,M_PACKET(iy)
        and     #3
        rrca
        rrca
        ld      d,a
        ld      a,M_PACKET+1(iy)
        and     #0x3f
        or      d
        ld      l,a
        add     a,a
        sbc     a,a
        ld      h,a                     ; sign-extended X
        push    hl
        ld      a,M_PACKET(iy)
        and     #12
        rlca
        rlca
        rlca
        rlca
        ld      d,a
        ld      a,e
        and     #0x3f
        or      d
        ld      c,a
        add     a,a
        sbc     a,a
        ld      b,a                     ; sign-extended Y
        xor     a
        bit     5,M_PACKET(iy)
        jr      z,.mouse_no_left
        or      #1
.mouse_no_left:
        bit     4,M_PACKET(iy)
        jr      z,.mouse_buttons_ready
        or      #4
.mouse_buttons_ready:
        ld      M_COUNT(iy),#0
        ld      M_SYNCED(iy),#0
        pop     de                      ; DE=X, BC=Y, A=normalized buttons
        jp      __serial_common_mouse_apply

