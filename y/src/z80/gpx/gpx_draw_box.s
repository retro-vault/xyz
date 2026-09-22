        ;; gpx_draw_box.s
        ;;
        ;; Compact selectable-edge box renderer. Each target assembles its own
        ;; copy so the public primitive stays with the backend line engine.
        ;;
        ;; GPL2 License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih

        .module gpx_draw_box
        .optsdcc -mz80 sdcccall(1)

        .globl  _gpx_draw_box
        .globl  _gpx_draw_line
        .globl  __rect_unpack_norm

        .equ    EDGE_LEFT,   0x01
        .equ    EDGE_TOP,    0x02
        .equ    EDGE_RIGHT,  0x04
        .equ    EDGE_BOTTOM, 0x08
        .equ    EDGE_ALL,    0x0f

        .equ    A_EDGES, 4
        .equ    A_C,     5
        .equ    A_M,     6
        .equ    A_LP,    7
        .equ    A_CLIP,  8

        ;; __rect_unpack_norm layout, followed by our state.
        .equ    L_X0,    -8
        .equ    L_X1,    -4
        .equ    L_Y0,    -6
        .equ    L_Y1,    -2
        .equ    L_EDGES, -9
        .equ    L_SIZE,  9

        .globl  __frame_ix

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; uint8_t gpx_draw_box(gpx_t *gpx, const rect_t *r, uint8_t edges,
        ;;     color c, bmode m, uint8_t lpatt, const rect_t *clip)
        ;; Clobbers: AF, BC, DE, HL and the alternate set. Preserves IX/IY.
_gpx_draw_box::
        call    __frame_ix
        ld      hl,#-L_SIZE
        add     hl,sp
        ld      sp,hl
        ld      a,A_EDGES(ix)
        and     #EDGE_ALL
        ld      L_EDGES(ix),a
        jr      z,.db_done
        ld      a,d
        or      e
        jr      z,.db_done

        call    __rect_unpack_norm

        ;; Collapse coincident geometry to one owner. The ordinary edge path
        ;; below can then handle rows, columns and points without a case tree.
        ld      a,L_X0(ix)
        cp      L_X1(ix)
        jr      nz,.db_check_row
        ld      a,L_X0+1(ix)
        cp      L_X1+1(ix)
        jr      nz,.db_check_row
        ld      a,L_Y0(ix)
        cp      L_Y1(ix)
        jr      nz,.db_one_col
        ld      a,L_Y0+1(ix)
        cp      L_Y1+1(ix)
        jr      nz,.db_one_col
        ld      L_EDGES(ix),#EDGE_TOP
        jr      .db_edges

.db_one_col:
        ld      a,L_EDGES(ix)
        and     #(EDGE_LEFT | EDGE_RIGHT)
        jr      z,.db_edges
        ld      L_EDGES(ix),#EDGE_RIGHT
        jr      .db_edges

.db_check_row:
        ld      a,L_Y0(ix)
        cp      L_Y1(ix)
        jr      nz,.db_edges
        ld      a,L_Y0+1(ix)
        cp      L_Y1+1(ix)
        jr      nz,.db_edges
        ld      a,L_EDGES(ix)
        and     #(EDGE_TOP | EDGE_BOTTOM)
        jr      z,.db_edges
        ld      L_EDGES(ix),#EDGE_TOP

.db_edges:
        bit     1,L_EDGES(ix)
        call    nz,.db_top
        bit     2,L_EDGES(ix)
        call    nz,.db_right
        bit     3,L_EDGES(ix)
        call    nz,.db_bottom
        bit     0,L_EDGES(ix)
        call    nz,.db_left

.db_done:
        ld      a,A_LP(ix)
        ld      sp,ix
        pop     ix
        pop     hl
        pop     bc
        pop     bc
        pop     bc
        jp      (hl)

        ;; DE=x0, HL=y0, DE'=x1, BC'=y1, A=next pattern.
        ;; The ZX line engine does not use the context pointer. Keep y0 in
        ;; BC while the alternate bank pushes the remaining arguments.
.db_emit:
        push    af                      ; continuation survives the line call
        ld      b,h
        ld      c,l
        exx
        ld      l,A_CLIP(ix)
        ld      h,A_CLIP+1(ix)
        push    hl
        ld      h,A_LP(ix)
        ld      l,A_M(ix)
        push    hl
        ld      a,A_C(ix)
        push    af
        inc     sp
        push    bc
        push    de
        exx
        push    bc
        call    _gpx_draw_line
        pop     af
        ld      A_LP(ix),a
        ret

        ;; A = inclusive edge length modulo 256. Precompute clip-independent
        ;; continuation; zero low bits mean a whole number of periods.
.db_advance:
        and     #7
        ld      b,a
        ld      a,A_LP(ix)
        ret     z
.db_advance_loop:
        rrca
        djnz    .db_advance_loop
        ret

.db_top:
        ld      a,L_X1(ix)
        sub     L_X0(ix)
        inc     a
        call    .db_advance
        ld      e,L_X0(ix)
        ld      d,L_X0+1(ix)
        ld      l,L_Y0(ix)
        ld      h,L_Y0+1(ix)
        push    hl
        exx
        pop     bc
        ld      e,L_X1(ix)
        ld      d,L_X1+1(ix)
        exx
        jr      .db_emit

.db_right:
        ld      l,L_Y0(ix)
        ld      h,L_Y0+1(ix)
        bit     1,L_EDGES(ix)
        jr      z,.db_right_start
        inc     hl
.db_right_start:
        ld      a,L_Y1(ix)
        sub     l
        inc     a
        call    .db_advance
        ld      e,L_X1(ix)
        ld      d,L_X1+1(ix)
        push    de
        exx
        pop     de
        ld      c,L_Y1(ix)
        ld      b,L_Y1+1(ix)
        exx
        jr      .db_emit

.db_bottom:
        ld      l,L_X1(ix)
        ld      h,L_X1+1(ix)
        bit     2,L_EDGES(ix)
        jr      z,.db_bottom_start
        dec     hl
.db_bottom_start:
        ld      a,l
        sub     L_X0(ix)
        inc     a
        call    .db_advance
        ex      de,hl
        ld      l,L_Y1(ix)
        ld      h,L_Y1+1(ix)
        push    hl
        exx
        pop     bc
        ld      e,L_X0(ix)
        ld      d,L_X0+1(ix)
        exx
        jp      .db_emit

.db_left:
        ;; With normalized non-coincident rows, shortening crosses only when
        ;; both horizontal edges own a two-row box.
        ld      a,L_EDGES(ix)
        and     #(EDGE_TOP | EDGE_BOTTOM)
        cp      #(EDGE_TOP | EDGE_BOTTOM)
        jr      nz,.db_left_build
        ld      l,L_Y0(ix)
        ld      h,L_Y0+1(ix)
        inc     hl
        ld      a,l
        cp      L_Y1(ix)
        jr      nz,.db_left_build
        ld      a,h
        cp      L_Y1+1(ix)
        ret     z

.db_left_build:
        ld      l,L_Y1(ix)
        ld      h,L_Y1+1(ix)
        bit     3,L_EDGES(ix)
        jr      z,.db_left_start
        dec     hl
.db_left_start:
        ld      a,l
        bit     1,L_EDGES(ix)
        jr      z,.db_left_length
        dec     a
.db_left_length:
        sub     L_Y0(ix)
        inc     a
        call    .db_advance
        ld      e,L_X0(ix)
        ld      d,L_X0+1(ix)
        push    de
        exx
        pop     de
        ld      c,L_Y0(ix)
        ld      b,L_Y0+1(ix)
        bit     1,L_EDGES(ix)
        jr      z,.db_left_end
        inc     bc
.db_left_end:
        exx
        jp      .db_emit
