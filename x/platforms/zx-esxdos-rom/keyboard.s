        ;; keyboard.s -- ZX Spectrum keyboard polling and libc input
        ;;
        ;; Uses the YOS keyboard row order and ASCII maps. No interrupt or
        ;; ROM system-variable dependency is required.

        .module zx_keyboard
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar
        .globl  _trygetchar

        .area   _CODE

_getchar::
.zx_key_wait_press:
        call    _trygetchar
        or      a
        jr      z,.zx_key_wait_press
        ld      (_zx_key_ascii),a
.zx_key_wait_release:
        call    _trygetchar
        or      a
        jr      nz,.zx_key_wait_release
        ld      a,(_zx_key_ascii)
        ld      e,a
        ld      d,#0
        ret

;; Poll the matrix once without blocking.
;; Return the current ASCII key in DE, or zero when no key is down.
_trygetchar::
        call    .zx_key_scan
        ld      e,a
        ld      d,#0
        ret

;; Return one ASCII character in A, or zero when no non-shift key is down.
.zx_key_scan:
        xor     a
        ld      (_zx_key_control),a
        xor     a
        ld      (_zx_key_row),a
        ld      bc,#0xf7fe
.zx_key_row_loop:
        in      a,(c)
        cpl
        and     #0x1f
        ld      e,a
        ld      a,(_zx_key_row)
        cp      #4
        jr      nz,.zx_key_not_symbol_row
        ld      a,e
        and     #0x1d
        ld      e,a
.zx_key_not_symbol_row:
        ld      a,(_zx_key_row)
        cp      #5
        jr      nz,.zx_key_not_caps_row
        ld      a,e
        and     #0x1e
        ld      e,a
.zx_key_not_caps_row:
        ld      a,e
        or      a
        jr      nz,.zx_key_find_bit
        rlc     b
        ld      a,(_zx_key_row)
        inc     a
        ld      (_zx_key_row),a
        cp      #8
        jr      nz,.zx_key_row_loop
        xor     a
        ret

.zx_key_find_bit:
        ld      d,#0
.zx_key_bit_loop:
        srl     a
        jr      c,.zx_key_bit_found
        inc     d
        jr      .zx_key_bit_loop
.zx_key_bit_found:
        ld      a,(_zx_key_row)
        ld      e,a
        add     a,a
        add     a,a
        add     a,e
        add     a,d
        ld      e,a
        ld      d,#0

        ;; A chord may go down halfway through the row scan. Sample the
        ;; modifiers only after the ordinary key has been selected so that
        ;; CAPS/SYMBOL and the key belong to the same observation.
        xor     a
        ld      (_zx_key_control),a
        ld      bc,#0xfefe
        in      a,(c)
        cpl
        and     #1
        jr      z,.zx_key_no_caps
        ld      a,#2
        ld      (_zx_key_control),a
.zx_key_no_caps:
        ld      bc,#0x7ffe
        in      a,(c)
        cpl
        and     #2
        jr      z,.zx_key_no_symbol
        ld      a,(_zx_key_control)
        or      #1
        ld      (_zx_key_control),a
.zx_key_no_symbol:
        ld      a,(_zx_key_control)
        and     #1
        ld      hl,#.zx_key_map
        jr      z,.zx_key_not_symbol_map
        ld      hl,#.zx_key_map_symbol
        jr      .zx_key_map_ready
.zx_key_not_symbol_map:
        ld      a,(_zx_key_control)
        and     #2
        jr      z,.zx_key_map_ready
        ld      hl,#.zx_key_map_caps
.zx_key_map_ready:
        add     hl,de
        ld      a,(hl)
        ret

        .area   _CONST
.zx_key_map:
        .db     '1','2','3','4','5'
        .db     '0','9','8','7','6'
        .db     'p','o','i','u','y'
        .db     0x0d,'l','k','j','h'
        .db     0x20,0x00,'m','n','b'
        .db     0x00,'z','x','c','v'
        .db     'a','s','d','f','g'
        .db     'q','w','e','r','t'
.zx_key_map_symbol:
        .db     '!','@','#','$','%'
        .db     '_',')','(',0x27,'&'
        .db     '"',';','?',']','['
        .db     0x0d,'=','+','-','^'
        .db     0x20,0x00,'.',',','*'
        .db     0x00,':','?','?','/'
        .db     '~','|',']','{','}'
        .db     0x00,0x00,'`','<','>'
.zx_key_map_caps:
        .db     '1','2','3','4',0x08
        .db     0x08,'9',0x0c,0x0b,0x0a
        .db     'P','O','I','U','Y'
        .db     0x0d,'L','K','J','H'
        .db     0x20,0x00,'M','N','B'
        .db     0x00,'Z','X','C','V'
        .db     'A','S','D','F','G'
        .db     'Q','W','E','R','T'

        .area   _BSS
_zx_key_control:
        .ds     1
_zx_key_row:
        .ds     1
_zx_key_ascii:
        .ds     1
