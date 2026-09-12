        ; Poll a Kempston mouse and return its current state.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module mouse_read
        .optsdcc -mz80 sdcccall(1)

        .globl  _mouse_read
        .globl  __critical_call
        .globl  __mouse_cursor
        .globl  __mouse_hardware
        .globl  __mouse_buttons
        .globl  __mouse_changes

        .equ    KMP_BTN_PORT, 0xfadf
        .equ    KMP_X_PORT, 0xfbdf
        .equ    KMP_Y_PORT, 0xffdf

        .area   _CODE

        ; input: HL = four-byte output {x, y, buttons, changed}.
_mouse_read::
        call    __critical_call
        push    hl                      ; save pointer to mouse_info_t
        call    .kmp_scan_raw           ; scan it
        pop     hl                      ; restore pointer to mi
        ld      (hl),c
        inc     hl
        ld      (hl),b
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),d
        ret

        ;; .kmp_scan_raw
        ;; return: A = mouse buttons
        ;;         B = Y delta
        ;;         C = X delta
        ;;         D = button change flags (1=change, 0=no change)
        ;; affects: FLAGS, A, BC, HL, DE
        ;; notes:   reads hardware and computes delta since last scan
.kmp_scan_raw:
        ;; first scan buttons for changes
        ld      bc,#KMP_BTN_PORT
        in      a,(c)                   ; buttons to a
        cpl                             ; complement (1=pressed)
        and     #0x07                   ; just the bottom three
        ld      b,a                     ; store current buttons
        ld      a,(__mouse_buttons)
        xor     b                       ; xor with current buttons
        ld      (__mouse_changes),a        ; store change flags
        ld      a,b                     ; a=new button state
        ld      (__mouse_buttons),a         ; store

        ;; now scan position for changes
.kmp_scanpos:
        ld      hl,(__mouse_cursor)        ; last cursor coords
        ld      de,(__mouse_hardware)         ; last hardware coords
        ld      bc,#KMP_X_PORT
        in      a,(c)                   ; read x from hw
        ld      (__mouse_hardware),a          ; immediately write
        sub     e                       ; minus prev coord
        jr      z,.kmp_x_done           ; no change to x
        jp      p,.kmp_x_right          ; move right
        ;; left or overflow
        add     a,l                     ; a=old x-new x
        jr      c,.kmp_l_norm           ; normal left
        xor     a                       ; overflow, a=0
.kmp_l_norm:
        ld      l,a                     ; to l
        jr      .kmp_x_done
.kmp_x_right:
        add     a,l                     ; a=old x + dx
        jr      c,.kmp_r_over           ; overflow?
        cp      #0xff                   ; max x?
        jr      c,.kmp_r_norm
.kmp_r_over:
        ld      a,#0xff                 ; max x
.kmp_r_norm:
        ld      l,a                     ; to l
.kmp_x_done:
        ld      b,#0xff
        in      a,(c)                   ; read y from hw
        ld      (__mouse_hardware+1),a        ; immediately write
        sub     d                       ; minus old hw y
        jr      z,.kmp_y_done
        neg                             ; reverse coord
        jp      p,.kmp_u
        add     a,h                     ; how much up?
        jr      c,.kmp_u_norm
        xor     a                       ; min y coor
.kmp_u_norm:
        ld      h,a
        jr      .kmp_y_done
.kmp_u:
        add     a,h
        jr      c,.kmp_d_over           ; y overflow
        cp      #0xbf                   ; 191 (max y)
        jr      c,.kmp_d_norm
.kmp_d_over:
        ld      a,#0xbf                 ; y=max y
.kmp_d_norm:
        ld      h,a                     ; store to h
.kmp_y_done:
        ld      (__mouse_cursor),hl        ; store new cursor pos
        push    hl
        pop     bc                      ; bc=hl
        ld      a,(__mouse_changes)
        ld      d,a
        ld      a,(__mouse_buttons)         ; button state to a
        ret
