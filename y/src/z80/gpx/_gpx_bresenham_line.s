        ;; _gpx_bresenham_line.s
        ;;
        ;; Diagonal line renderer: clip first, then draw fast.
        ;;
        ;; The segment is clipped up front with Cohen-Sutherland against the
        ;; effective rect = clip ∩ screen (screen alone when clip is NULL),
        ;; in full signed 16-bit space, so arbitrarily far off-screen
        ;; endpoints cost a handful of intersections instead of one plot
        ;; call per off-screen pixel. The dash pattern is pre-rotated by the
        ;; skipped major-axis distance so the phase matches an unclipped
        ;; draw. The surviving segment lies inside 256x192, so the raster
        ;; loops run entirely in registers: main set holds count/pattern/
        ;; masks/VRAM pointer, alternate set holds err/dx/dy, A' holds the
        ;; step directions. No per-pixel bounds or clip checks remain.
        ;;
        ;; Semantics byte-match the oracle stub (tests/zx/stub/gpx_stub.c):
        ;; same C-S edge order (T,B,R,L), same truncating-division
        ;; intersection, same skip rule, same raster loop, same return
        ;; pattern. A clip rect with swapped corners draws nothing.
        ;;
        ;; Re-entrant: all state lives in registers and the stack frame.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-25   TS

        .module _gpx_bresenham_line
        .optsdcc -mz80 sdcccall(1)

        .globl  __gpx_bresenham_line
        .globl  __rect_cmp16s_lt
        .globl  __ret_clean11
        .globl  __vid_rowaddr
        .globl  __vid_nextrow
        .globl  __vid_nextrow_carry
        .globl  __vid_prevrow_carry
        .globl  __vid_prevrow

        ;; outcode bits (stub order/priority: T, B, R, L)
        .equ    OC_LEFT,   0
        .equ    OC_RIGHT,  1
        .equ    OC_TOP,    2
        .equ    OC_BOTTOM, 3

        ;; frame locals (IX-relative)
        .equ    L_X0,     -2            ; word: clipped-in-progress x0
        .equ    L_EX0,    -3            ; effective clip, bytes (post-clamp)
        .equ    L_EY0,    -4
        .equ    L_EX1,    -5
        .equ    L_EY1,    -6
        .equ    L_SKIPB,  -8            ; word: original major-axis start
        .equ    L_FLAGS,  -9            ; bit0 = major axis is x (|odx|>=|ody|)
        .equ    L_AXIS,   -10           ; isect flavor: 0 = x (T/B), 1 = y (R/L)
        .equ    L_NB,     -11           ; byte: clip-edge coordinate for isect
        .equ    L_SIGN,   -12           ; byte: folded sign of isect offset
        .equ    L_DX,     -13           ; byte: post-clip |dx|
        .equ    L_DY,     -14           ; byte: post-clip |dy|
        .equ    L_A0,     -16           ; word: isect base coordinate
        .equ    L_SIZE,   16

        ;; args after push ix (callee cleans 11 bytes)
        ;;   HL = gpx (unused), DE = x0
        .equ    A_Y0,   4               ; word (mutated in place by C-S)
        .equ    A_X1,   6               ; word (mutated in place by C-S)
        .equ    A_Y1,   8               ; word (mutated in place by C-S)
        .equ    A_C,    10
        .equ    A_M,    11
        .equ    A_LP,   12              ; lpatt (skip-rotated in place)
        .equ    A_CLIP, 13

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; __gpx_bresenham_line
        ;; Draw a clipped, patterned line. Cohen-Sutherland runs first
        ;; against clip intersected with the screen, the pattern is
        ;; pre-rotated by the skipped major-axis distance so the phase
        ;; matches an unclipped draw, and the surviving segment is then
        ;; rastered entirely in registers with no per-pixel bounds check.
        ;;
        ;; Arguments:
        ;;   DE = x0
        ;;   stack, relative to the frame: A_Y0, A_X1, A_Y1 (all mutated
        ;;   in place by the clipper), A_C, A_M, A_LP, A_CLIP
        ;;
        ;; Return:
        ;;   A = the pattern rotated by the number of pixels drawn, so
        ;;       chained segments keep their phase
        ;;
        ;; Clobbers:
        ;;   AF, BC, DE, HL, IX, and the alternate set
        ;;
        ;; References:
        ;;   __rect_cmp16s_lt
        ;;   __vid_rowaddr
        ;;   __vid_nextrow, __vid_nextrow_carry
        ;;   __vid_prevrow_carry
        ;;   __ret_clean11
__gpx_bresenham_line::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      hl,#-L_SIZE
        add     hl,sp
        ld      sp,hl

        ld      L_X0(ix),e
        ld      L_X0+1(ix),d

        ;; original |dx| vs |dy| picks the skip axis (before clipping)
        ld      l,A_X1(ix)
        ld      h,A_X1+1(ix)
        ld      e,L_X0(ix)
        ld      d,L_X0+1(ix)
        call    .bl_absd                ; HL = |odx|
        push    hl
        ld      l,A_Y1(ix)
        ld      h,A_Y1+1(ix)
        ld      e,A_Y0(ix)
        ld      d,A_Y0+1(ix)
        call    .bl_absd                ; HL = |ody|
        pop     de                      ; DE = |odx|
        or      a
        sbc     hl,de                   ; |ody| - |odx| (unsigned)
        jr      c,.bl_sb_x              ; ody < odx -> x major
        jr      z,.bl_sb_x              ; equal     -> x major
        xor     a
        ld      L_FLAGS(ix),a
        ld      a,A_Y0(ix)
        ld      L_SKIPB(ix),a
        ld      a,A_Y0+1(ix)
        ld      L_SKIPB+1(ix),a
        jr      .bl_eff
.bl_sb_x:
        ld      a,#1
        ld      L_FLAGS(ix),a
        ld      a,L_X0(ix)
        ld      L_SKIPB(ix),a
        ld      a,L_X0+1(ix)
        ld      L_SKIPB+1(ix),a

        ;; ---- effective clip rect: (clip ∩ screen) as four bytes ----
.bl_eff:
        xor     a
        ld      L_EX0(ix),a
        ld      L_EY0(ix),a
        dec     a
        ld      L_EX1(ix),a             ; 255
        ld      a,#191
        ld      L_EY1(ix),a

        ld      l,A_CLIP(ix)
        ld      h,A_CLIP+1(ix)
        ld      a,h
        or      l
        jr      z,.bl_cs_loop           ; no clip: screen rect stands

        ld      e,(hl)                  ; x0c
        inc     hl
        ld      d,(hl)
        inc     hl
        push    de
        ld      e,(hl)                  ; y0c
        inc     hl
        ld      d,(hl)
        inc     hl
        push    de
        ld      e,(hl)                  ; x1c
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      c,(hl)                  ; y1c
        inc     hl
        ld      h,(hl)
        ld      l,c                     ; HL = y1c
        ld      b,d
        ld      c,e                     ; BC = x1c
        pop     de                      ; DE = y0c
        ;; swapped clip (y1c < y0c) draws nothing
        call    __rect_cmp16s_lt
        jp      c,.bl_rej1
        ;; clamp y0c (DE) -> EY0
        bit     7,d
        jr      nz,.bl_cy1              ; y0c<0 -> keep 0
        ld      a,d
        or      a
        jp      nz,.bl_rej1             ; y0c>255: below screen -> empty
        ld      a,e
        cp      #192
        jp      nc,.bl_rej1             ; y0c in 192..255 -> empty
        ld      L_EY0(ix),a
.bl_cy1:
        ;; clamp y1c (HL) -> EY1
        bit     7,h
        jp      nz,.bl_rej1             ; y1c<0: above screen -> empty
        ld      a,h
        or      a
        jr      nz,.bl_cx               ; y1c>255 -> keep 191
        ld      a,l
        cp      #192
        jr      nc,.bl_cx               ; 192..255 -> keep 191
        ld      L_EY1(ix),a
.bl_cx:
        pop     de                      ; DE = x0c
        ;; swapped clip (x1c < x0c) draws nothing
        ld      l,c
        ld      h,b                     ; HL = x1c
        call    __rect_cmp16s_lt
        jp      c,.bl_reject
        ;; clamp x0c (DE) -> EX0
        bit     7,d
        jr      nz,.bl_cx1              ; x0c<0 -> keep 0
        ld      a,d
        or      a
        jp      nz,.bl_reject           ; x0c>255 -> empty
        ld      a,e
        ld      L_EX0(ix),a
.bl_cx1:
        ;; clamp x1c (HL=BC) -> EX1
        bit     7,b
        jp      nz,.bl_reject           ; x1c<0 -> empty
        ld      a,b
        or      a
        jr      nz,.bl_cs_loop          ; x1c>255 -> keep 255
        ld      a,c
        ld      L_EX1(ix),a

        ;; ---- Cohen-Sutherland loop (16-bit, cold) ----
.bl_cs_loop:
        ld      l,L_X0(ix)
        ld      h,L_X0+1(ix)
        ld      e,A_Y0(ix)
        ld      d,A_Y0+1(ix)
        call    .bl_outc
        ld      b,a                     ; B = c0
        ld      l,A_X1(ix)
        ld      h,A_X1+1(ix)
        ld      e,A_Y1(ix)
        ld      d,A_Y1+1(ix)
        call    .bl_outc                ; A = c1 (B preserved)
        ld      c,a
        or      b
        jr      z,.bl_accept            ; both inside
        ld      a,b
        and     c
        jp      nz,.bl_reject           ; share an outside half-plane
        push    bc                      ; save which-endpoint (B = c0)
        ld      a,b
        or      a
        jr      nz,.bl_disp
        ld      a,c
.bl_disp:
        bit     OC_TOP,a
        jp      nz,.bl_edge_t
        bit     OC_BOTTOM,a
        jp      nz,.bl_edge_b
        bit     OC_RIGHT,a
        jp      nz,.bl_edge_r
        jp      .bl_edge_l

        ;; ---- accept: rotate pattern by skipped major distance ----
.bl_accept:
        ld      a,L_FLAGS(ix)
        and     #0x01
        jr      z,.bl_sk_y
        ld      l,L_X0(ix)
        ld      h,L_X0+1(ix)
        jr      .bl_sk_go
.bl_sk_y:
        ld      l,A_Y0(ix)
        ld      h,A_Y0+1(ix)
.bl_sk_go:
        ld      e,L_SKIPB(ix)
        ld      d,L_SKIPB+1(ix)
        call    .bl_absd                ; HL = skip distance
        ld      a,l
        and     #0x07
        jr      z,.bl_setup
        ld      b,a
        ld      a,A_LP(ix)
.bl_sk_rot:
        rrca
        djnz    .bl_sk_rot
        ld      A_LP(ix),a

        ;; ---- 8-bit raster setup ----
.bl_setup:
        ld      c,#0                    ; C = direction flags
        ld      a,A_X1(ix)
        sub     L_X0(ix)
        jr      nc,.bl_dxp
        neg
        set     0,c                     ; x runs left
.bl_dxp:
        ld      L_DX(ix),a
        ld      a,A_Y1(ix)
        sub     A_Y0(ix)
        jr      nc,.bl_dyp
        neg
        set     1,c                     ; y runs up
.bl_dyp:
        ld      L_DY(ix),a
        ;; Choose the raster axis from the surviving segment. The original
        ;; axis above is needed only for the pattern phase before clipping.
        cp      L_DX(ix)
        sbc     a,a
        and     #1
        ld      L_FLAGS(ix),a           ; dx > dy: x-major
        ld      a,c
        ex      af,af'                  ; A' = direction flags

        exx                             ; alt: BC=dy, DE=dx, HL=err
        ld      b,#0
        ld      c,L_DY(ix)
        ld      d,#0
        ld      e,L_DX(ix)
        ld      a,L_FLAGS(ix)
        and     #0x01
        jr      z,.bl_err_y
        ld      a,e                     ; err = dx/2
        srl     a
        ld      l,a
        ld      h,#0
        jr      .bl_err_ok
.bl_err_y:
        ld      a,c                     ; y-major stores the deficit dy/2
        srl     a
        ld      l,a
        ld      h,#0
.bl_err_ok:
        exx

        ;; mask = 0x80 >> (x0 & 7)
        ld      a,L_X0(ix)
        and     #0x07
        ld      b,a
        ld      a,#0x80
        jr      z,.bl_mask_ok
.bl_mask_sh:
        srl     a
        djnz    .bl_mask_sh
.bl_mask_ok:
        ;; plot = (byte | D) ^ E:  set D=M,E=0; clear D=M,E=M; xor D=0,E=M
        ld      d,a
        ld      e,a
        ld      a,A_M(ix)
        and     #0x01
        jr      z,.bl_m_cpy
        ld      d,#0                    ; xor
        jr      .bl_m_ok
.bl_m_cpy:
        ld      a,A_C(ix)
        and     #0x01
        jr      z,.bl_m_ok              ; clear: D=E=M
        ld      e,#0                    ; set
.bl_m_ok:
        ;; HL = VRAM addr of (x0,y0)
        ld      b,A_Y0(ix)
        call    __vid_rowaddr           ; preserves DE
        ld      a,L_X0(ix)
        rrca
        rrca
        rrca
        and     #0x1F
        add     a,l
        ld      l,a                     ; row base has bits 0..4 clear

        ld      c,A_LP(ix)              ; C = pattern
        ld      a,L_FLAGS(ix)
        and     #0x01
        jr      z,.bl_go_y
        ld      b,L_DX(ix)              ; steps = dx
        ld      a,b
        or      a
        jp      z,.bl_last              ; clipped to a single pixel
        ;; The x direction is fixed for the whole line, so it selects the
        ;; raster once here instead of being re-read from A' every pixel.
        ex      af,af'
        bit     0,a
        jr      nz,.bx_go_left          ; branch before swapping the flags back
        ex      af,af'
        jr      .bxr_loop
.bx_go_left:
        ex      af,af'
        jr      .bxl_loop
.bl_go_y:
        ld      b,L_DY(ix)              ; steps = dy
        ld      a,b
        or      a
        jp      z,.bl_last              ; clipping collapsed it to one pixel
        ld      a,L_DX(ix)
        or      a
        jp      nz,.by_loop             ; genuine diagonal
        ex      af,af'
        bit     1,a                     ; vertical: upward?
        jr      nz,.bl_go_vup
        ex      af,af'
        jp      .bv_loop                ; downward vertical: fast path
.bl_go_vup:
        ex      af,af'
        jr      .by_loop                ; upward vertical: generic loop

        ;; ---- x-major raster: B=steps C=patt D/E=masks HL=vram ----
        ;; alt: L'=err DE'=dx BC'=dy; H' is ignored, A' holds directions.
        ;; ---- x-major rasters: B=steps C=patt D/E=masks HL=vram ----
        ;; alt: L'=err DE'=dx BC'=dy; A' bit1=up. Two variants, one per x
        ;; direction, so the only per-pixel branch left is the y step.
.bxr_loop:
        rrc     c                       ; carry = pattern bit, C rotated for
        jr      nc,.bxr_loop_np         ; the next step in the same op
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
.bxr_loop_np:
        exx
        ld      a,l
        sub     c                       ; 8-bit err -= dy; carry means negative
        ld      l,a
        exx                             ; flags survive exx
        jp      nc,.bxr_loop_xadv
        exx
        add     hl,de                   ; err += dx
        exx
        ex      af,af'
        bit     1,a
        jr      nz,.bxr_loop_yup
        ex      af,af'
        call    __vid_nextrow
        jr      .bxr_loop_xadv
.bxr_loop_yup:
        ex      af,af'
        call    __vid_prevrow
.bxr_loop_xadv:
        ld      a,d
        or      e                       ; A = live mask
        rrc     d
        rrc     e
        rrca                            ; carry = old bit0 (wrap right)
        jr      nc,.bxr_loop_rot
        inc     hl
.bxr_loop_rot:
        djnz    .bxr_loop
        jp      .bl_last                ; do NOT fall through into the left variant

.bxl_loop:
        rrc     c                       ; carry = pattern bit, C rotated for
        jr      nc,.bxl_loop_np         ; the next step in the same op
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
.bxl_loop_np:
        exx
        ld      a,l
        sub     c                       ; 8-bit err -= dy; carry means negative
        ld      l,a
        exx                             ; flags survive exx
        jp      nc,.bxl_loop_xadv
        exx
        add     hl,de                   ; err += dx
        exx
        ex      af,af'
        bit     1,a
        jr      nz,.bxl_loop_yup
        ex      af,af'
        call    __vid_nextrow
        jr      .bxl_loop_xadv
.bxl_loop_yup:
        ex      af,af'
        call    __vid_prevrow
.bxl_loop_xadv:
        ld      a,d
        or      e
        rlc     d
        rlc     e
        rlca                            ; carry = old bit7 (wrap left)
        jr      nc,.bxl_loop_rot
        dec     hl
.bxl_loop_rot:
        djnz    .bxl_loop

        ;; ---- shared tail: final pixel + return pattern ----
.bl_last:
        bit     0,c
        jr      z,.bl_retp
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
.bl_retp:
        ld      a,c
.bl_done:
        ld      sp,ix
        pop     ix
        jp      __ret_clean11

.bl_rej1:                               ; reject with one word still stacked
        pop     de
.bl_reject:
        ld      a,A_LP(ix)              ; untouched original pattern
        jr      .bl_done

        ;; ---- y-major raster ----
.by_loop:
        rrc     c                       ; carry = pattern bit, C rotated for
        jr      nc,.by_np               ; the next step in the same op
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
.by_np:
        exx
        ld      a,l
        sub     e                       ; deficit -= dx; equality does not step
        ld      l,a
        exx
        jp      nc,.by_yadv
        exx
        add     hl,bc                   ; deficit += dy, only the low byte is live
        exx
        ex      af,af'
        bit     0,a
        jr      nz,.by_xleft
        ex      af,af'
        ld      a,d
        or      e
        rrc     d
        rrc     e
        rrca
        jr      nc,.by_yadv
        inc     hl
        jr      .by_yadv
.by_xleft:
        ex      af,af'
        ld      a,d
        or      e
        rlc     d
        rlc     e
        rlca
        jr      nc,.by_yadv
        dec     hl
.by_yadv:
        ex      af,af'
        bit     1,a
        jr      nz,.by_yup
        ex      af,af'
        inc     h                       ; inlined __vid_nextrow fast path
        ld      a,h
        and     #0x07
        call    z,__vid_nextrow_carry
        jr      .by_rot
.by_yup:
        ex      af,af'
        dec     h                       ; inlined __vid_prevrow fast path
        ld      a,h
        and     #0x07
        cp      #0x07
        call    z,__vid_prevrow_carry
.by_rot:
        djnz    .by_loop
        jr      .bl_last

        ;; ---- vertical fast path: dx == 0, y increasing ----
        ;; With dx = 0 the y-major loop never steps x, so no err
        ;; bookkeeping is needed: plot, next row, rotate pattern.
        ;; B=steps C=patt D/E=masks HL=vram. ~105 T/pixel.
.bv_loop:
        rrc     c                       ; carry = pattern bit, C rotated for
        jr      nc,.bv_np               ; the next step in the same op
        ld      a,(hl)
        or      d
        xor     e
        ld      (hl),a
.bv_np:
        inc     h                       ; inlined __vid_nextrow fast path
        ld      a,h
        and     #0x07
        call    z,__vid_nextrow_carry
        djnz    .bv_loop
        jr      .bl_last

        ;; ---- C-S edge handlers ----
        ;; stack holds pushed BC (B = c0 = which endpoint moves)
.bl_edge_t:
        call    .bl_eqy
        jr      z,.bl_rejpop
        ld      a,L_EY0(ix)
        ld      L_NB(ix),a
        call    .bl_isect_x             ; HL = nx
        jr      .bl_put_xy
.bl_edge_b:
        call    .bl_eqy
        jr      z,.bl_rejpop
        ld      a,L_EY1(ix)
        ld      L_NB(ix),a
        call    .bl_isect_x
        jr      .bl_put_xy
.bl_edge_r:
        call    .bl_eqx
        jr      z,.bl_rejpop
        ld      a,L_EX1(ix)
        ld      L_NB(ix),a
        call    .bl_isect_y             ; HL = ny
        jr      .bl_put_yx
.bl_edge_l:
        call    .bl_eqx
        jr      z,.bl_rejpop
        ld      a,L_EX0(ix)
        ld      L_NB(ix),a
        call    .bl_isect_y

.bl_put_yx:                             ; HL = ny computed, nx = NB
        ex      de,hl                   ; DE = ny
        ld      l,L_NB(ix)
        ld      h,#0                    ; HL = nx
        jr      .bl_put
.bl_put_xy:                             ; HL = nx computed, ny = NB
        ld      e,L_NB(ix)
        ld      d,#0                    ; DE = ny
.bl_put:
        pop     bc                      ; B = c0
        ld      a,b
        or      a
        jr      z,.bl_put1
        ld      L_X0(ix),l              ; move endpoint 0
        ld      L_X0+1(ix),h
        ld      A_Y0(ix),e
        ld      A_Y0+1(ix),d
        jp      .bl_cs_loop
.bl_put1:
        ld      A_X1(ix),l              ; move endpoint 1
        ld      A_X1+1(ix),h
        ld      A_Y1(ix),e
        ld      A_Y1+1(ix),d
        jp      .bl_cs_loop

.bl_rejpop:
        pop     bc
        jp      .bl_reject

        ;; ---- helpers ----

        ;; outcode of (HL=x, DE=y) vs effective rect. A = code, clobbers C.
.bl_outc:
        ld      c,#0
        bit     7,h
        jr      z,.bo_x1
        set     OC_LEFT,c               ; x < 0
        jr      .bo_y
.bo_x1:
        ld      a,h
        or      a
        jr      z,.bo_x2
        set     OC_RIGHT,c              ; x > 255
        jr      .bo_y
.bo_x2:
        ld      a,l
        cp      L_EX0(ix)
        jr      nc,.bo_x3
        set     OC_LEFT,c
        jr      .bo_y
.bo_x3:
        ld      a,L_EX1(ix)
        cp      l
        jr      nc,.bo_y
        set     OC_RIGHT,c
.bo_y:
        bit     7,d
        jr      z,.bo_y1
        set     OC_TOP,c                ; y < 0
        jr      .bo_ret
.bo_y1:
        ld      a,d
        or      a
        jr      z,.bo_y2
        set     OC_BOTTOM,c             ; y > 255 (> 191)
        jr      .bo_ret
.bo_y2:
        ld      a,e
        cp      L_EY0(ix)
        jr      nc,.bo_y3
        set     OC_TOP,c
        jr      .bo_ret
.bo_y3:
        ld      a,L_EY1(ix)
        cp      e
        jr      nc,.bo_ret
        set     OC_BOTTOM,c
.bo_ret:
        ld      a,c
        ret

        ;; endpoint equality tests (Z set when equal / parallel to edge)
.bl_eqy:
        ld      a,A_Y0(ix)
        cp      A_Y1(ix)
        ret     nz
        ld      a,A_Y0+1(ix)
        cp      A_Y1+1(ix)
        ret
.bl_eqx:
        ld      a,L_X0(ix)
        cp      A_X1(ix)
        ret     nz
        ld      a,L_X0+1(ix)
        cp      A_X1+1(ix)
        ret

        ;; HL = |HL - DE| (signed operands), A = 1 iff HL < DE.
        ;; Callers fold that A into a product sign, so the 0/1 value is part
        ;; of the contract and is synthesized from the compare's carry here.
.bl_absd:
        call    __rect_cmp16s_lt        ; carry set iff HL < DE
        sbc     a,a                     ; 0xFF on carry, else 0x00
        and     #0x01                   ; A = 1 iff HL < DE (carry cleared)
        jr      z,.ba_sub
        ex      de,hl
.ba_sub:
        or      a                       ; clear carry, keep A
        sbc     hl,de
        ret

        ;; intersection: n = a0 +/- m1*m2/m3 (magnitudes u16, trunc division)
        ;;   x-flavor (T/B edges): a = x, b = y, NB = edge y
        ;;   y-flavor (R/L edges): a = y, b = x, NB = edge x
        ;; Both flavors need P = |x1-x0| and Q = |y1-y0|; they only swap
        ;; the m1 (numerator) / m3 (divisor) roles, the m2 base coordinate
        ;; and the a0 base, so P/Q are computed once and arranged after.
.bl_isect_x:
        xor     a
        jr      .bl_isect
.bl_isect_y:
        ld      a,#1
.bl_isect:
        ld      L_AXIS(ix),a
        ld      l,A_X1(ix)              ; P = |x1 - x0|, sign in B
        ld      h,A_X1+1(ix)
        ld      e,L_X0(ix)
        ld      d,L_X0+1(ix)
        call    .bl_absd
        ld      b,a                     ; B survives .bl_absd
        push    hl                      ; park P
        ld      l,A_Y1(ix)              ; Q = |y1 - y0|, sign in A
        ld      h,A_Y1+1(ix)
        ld      e,A_Y0(ix)
        ld      d,A_Y0+1(ix)
        call    .bl_absd
        xor     b
        ld      L_SIGN(ix),a            ; sP ^ sQ (m2 sign folded below)
        pop     de                      ; DE = P, HL = Q
        ld      a,L_AXIS(ix)
        or      a
        jr      nz,.bi_yflav
        ;; x-flavor: m1 = P, m3 = Q, b0 = y0, a0 = x0
        push    de                      ; [m1 = P]
        push    hl                      ; park m3 = Q
        ld      a,L_X0(ix)
        ld      L_A0(ix),a
        ld      a,L_X0+1(ix)
        ld      L_A0+1(ix),a
        ld      e,A_Y0(ix)              ; b0 = y0
        ld      d,A_Y0+1(ix)
        jr      .bi_m2
.bi_yflav:
        ;; y-flavor: m1 = Q, m3 = P, b0 = x0, a0 = y0
        push    hl                      ; [m1 = Q]
        push    de                      ; park m3 = P
        ld      a,A_Y0(ix)
        ld      L_A0(ix),a
        ld      a,A_Y0+1(ix)
        ld      L_A0+1(ix),a
        ld      e,L_X0(ix)              ; b0 = x0
        ld      d,L_X0+1(ix)
.bi_m2:
        ld      l,L_NB(ix)              ; m2 = |nb - b0|
        ld      h,#0
        call    .bl_absd
        ex      de,hl                   ; DE = m2
        ld      l,a                     ; fold m2 sign
        ld      a,L_SIGN(ix)
        xor     l
        ld      L_SIGN(ix),a
        pop     hl                      ; m3
        pop     bc                      ; m1
        push    hl                      ; park m3
        call    .bl_mul16               ; DE:HL = m1 * m2
        ex      de,hl                   ; HL = hi, DE = lo
        pop     bc                      ; m3
        call    .bl_div                 ; DE = quotient (hi < m3 guaranteed)
        ld      l,L_A0(ix)
        ld      h,L_A0+1(ix)
        ld      a,L_SIGN(ix)
        or      a
        jr      z,.bi_add
        sbc     hl,de                   ; carry cleared by or above
        ret
.bi_add:
        add     hl,de
        ret

        ;; DE:HL = DE * BC (unsigned 16x16 -> 32)
.bl_mul16:
        ld      hl,#0
        ld      a,#16
.bm_loop:
        add     hl,hl
        rl      e
        rl      d
        jr      nc,.bm_next
        add     hl,bc
        jr      nc,.bm_next
        inc     de
.bm_next:
        dec     a
        jr      nz,.bm_loop
        ret

        ;; DE = HL:DE / BC (requires HL < BC; that holds: quotient < 2^16),
        ;; HL = remainder
.bl_div:
        ld      a,#16
.bd_loop:
        sla     e
        rl      d
        adc     hl,hl
        jr      c,.bd_s17               ; 17-bit remainder: subtract always
        sbc     hl,bc                   ; trial (carry clear from adc)
        jr      nc,.bd_one
        add     hl,bc
        jr      .bd_next
.bd_s17:
        or      a
        sbc     hl,bc
.bd_one:
        inc     e                       ; quotient bit (bit0 vacated by sla)
.bd_next:
        dec     a
        jr      nz,.bd_loop
        ret
