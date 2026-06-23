        ;; stdio_emit_bytes.s
        ;;
        ;; __stdio_emit_bytes — drop-in for write() used by the stdio output
        ;; paths that work on a raw fd (fwrite, perror).  Console descriptors
        ;; (0/1/2) are sent to the platform putchar hook; file descriptors
        ;; (>= 3) tail-call the platform disk write().  Same ABI as write():
        ;;   HL = fd, DE = buf, length pushed by the caller (read at 4(ix)).
        ;;   returns DE = bytes emitted.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 tomaz stih

        .module stdio_emit_bytes
        .optsdcc -mz80 sdcccall(1)

        .globl  __stdio_emit_bytes
        .globl  _putchar
        .globl  _write

        .area   _CODE
__stdio_emit_bytes::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,h
        or      a
        jr      nz,__seb_disk
        ld      a,l
        cp      #3
        jr      nc,__seb_disk
        ;; console: putchar each byte
        ld      c,4(ix)
        ld      b,5(ix)
        push    bc                      ; save count for the return value
__seb_loop:
        ld      a,b
        or      c
        jr      z,__seb_done
        ld      a,(de)
        push    bc
        push    de
        ld      l,a
        ld      h,#0x00
        call    _putchar
        pop     de
        pop     bc
        inc     de
        dec     bc
        jr      __seb_loop
__seb_done:
        pop     de                      ; DE = bytes written
        pop     ix
        ret
__seb_disk:
        pop     ix
        jp      _write                  ; tail-call: HL=fd, DE=buf, len on stack
