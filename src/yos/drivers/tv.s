        ;; tv.s
        ;; 
        ;; vram calculations
		;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-06-16   tstih
        .module tv

        .area   _CODE

        .equ    VMEMBEG, 0x4000         ; video ram
        .equ    VMEMSZE, 0x1800         ; raster size
        .equ    ATTRSZE, 0x02ff         ; attr size

        .equ    BDRPORT, 0xfe           ; border port

        ;; given y (0...191) calc. row addr. in vmem
        ;; input:   b=y
        ;; output:  hl=vmem address, a=l
        ;; affects: flags, a, hl
tv_rowaddr::
        ld      a,b                     ; get y0-y2 to acc.
        and     #0x07                   ; mask out 00000111
        or      #0x40                   ; vmem addr
        ld      h,a                     ; to h
        ld      a,b                     ; y to acc. again
        rrca
        rrca
        rrca
        and     #0x18                   ; y6,y7 to correct pos.
        or      h                       ; h or a
        ld      h,a                     ; store to h
        ld      a,b                     ; y back to a
        rla                             ; move y3-y5 to position
        rla
        and     #0xe0                   ; mask out 11100000
        ld      l,a                     ; to l
        ret


        ;; given hl get next row address in vmem
        ;; input:   hl=address
        ;; output:  hl=next row address
        ;; affects: flags, a, hl
tv_nextrow::
        inc     h
        ld      a,h
        and     #7
        jr      nz,nextrow_done
        ld      a,l
        add     a,#32
        ld      l,a
        jr      c,nextrow_done
        ld      a,h
        sub     #8
        ld      h,a
nextrow_done:
        ret


        ;; clears the screen using background
        ;; and foreground color 
        ;; input:   a=background color
        ;;          b=foreground color
        ;; affects: af, hl, de, bc
tv_cls::
        out     (#BDRPORT),a            ; set border
        ;; prepare attr
        rlca                            ; bits 3-5
        rlca
        rlca
        or      b                       ; ink color to bits 0-2
        ;; first vmem
        ld      hl,#VMEMBEG             ; vmem
        ld      bc,#VMEMSZE             ; size
        ld      (hl),l                  ; l=0
        ld      d,h
        ld      e,#1
        ldir                            ; cls
        ld      (hl),a                  ; attr
        ld      bc,#ATTRSZE             ; size of attr
        ldir
        ret