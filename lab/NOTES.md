# notes from 2021-06-16

## support for depencency .d files

great news! **sdcc** supports dependency .d files:
 * https://sourceforge.net/p/sdcc/discussion/1864/thread/a51d74f3/
 * http://nuclear.mutantstargoat.com/articles/make/

## standard file comment and guards

for some reason we don't use uppercase letters in xyz, except for
"special occasions".

for .c file
~~~cpp
/*
 * filename.c
 *
 * file description.
 *
 * NOTES:
 *  optional notes.
 *  
 * MIT License (see: LICENSE)
 * copyright (c) 2021 tomaz stih
 *
 * 2021-06-16   tstih
 *
 */
~~~

for .h file
~~~cpp
/*
 * filename.h
 *
 * file description.
 * 
 * NOTES:
 *  optional notes.
 * 
 * MIT License (see: LICENSE)
 * copyright (C) 2021 tomaz stih
 *
 * 2021-06-16   tstih
 *
 */
 #ifndef __FILENAME_H__
 #define __FILENAME_H__
 #endif /* __FILENAME_H__ */
~~~

for .s file start your assembly at column 8 (2 tabs x 4 spaces),
place comments at column 41; if not possible, place your comment
in the row above commented code.

general rule is to use double semcolon for comments that span
entire row, and single semicolon for comments at column 41.
~~~assemmbly
		;; filename.s
        ;; 
        ;; The description of your library.
        ;; 
        ;; TODO: 
        ;;  And other keywords, such as NOTES, MORE, etc.
		;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2021 Tomaz Stih
        ;;
		;; 2021-06-16   tstih
~~~





_ttyx_gets::
        ;; get ptr to string to de
        pop     hl                      ; ret address
        pop     de                      ; ptr to string
        ;; restore stack
        push    de
        push    hl
        ;; counter
        ld      b,#0                    ; 0 chars
tgs_loop:
        push    bc                      ; store counter
        push    de                      ; and current string address
        call    _tty_getc               ; read char from kbd
        pop     de                      ; restore current string addr.
        pop     bc                      ; restore counter
        ld      a,l                     ; char into l
        or      a                       ; or it
        jr      z,tgs_loop              ; no char available?
        ;; we have a key
        ld      e,l                     ; ascii to e
        call    _tty_putc               ; to screen
        jr      tgs_loop


        cp      #KEY_ENTER              ; is it enter?
        jr      z,tgs_theend
        cp      #KEY_DEL
        jr      z,tgs_del
        ;; if it's not enter and del then...
        ;; ...check max len
        ld      a,b                     ; len to a
        cp      #MAX_GETS_LEN           ; compare to max len
        jr      nc, tgs_loop            ; don't allow processing
        ;; here we are, it's a valid key
        ld      a,l                     ; ascii to a
        inc     b                       ; inc char count
        ld      (de),a                  ; to memory
        inc     de                      ; next memory location
        push    bc                      ; store regs
        push    de                      
        ld      e,a                     ; ascii to e
        call    _tty_putc               ; to screen
        pop     de                      ; restore regs
        pop     bc
        jr      tgs_loop                ; and loop
tgs_del:

tgs_theend:
        xor     a                       ; zero terminate string
        ld      (de),a
        ret