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