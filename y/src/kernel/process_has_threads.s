        ;; process_has_threads.s
        ;;
        ;; Return non-zero when any thread queue still contains a thread
        ;; owned by the given process.
        ;;
        ;; Signature:
        ;;   uint8_t process_has_threads(process_t *p)
        ;;
        ;; Inputs (sdcccall1):
        ;;   HL = process pointer
        ;;
        ;; Returns:
        ;;   A = 1 if a matching thread exists, else 0

        .module process_has_threads

        .globl  _process_has_threads
        .globl  _thread_first_running
        .globl  _thread_first_suspended
        .globl  _thread_first_waiting
        .globl  _thread_first_terminated

        .equ    THREAD_PROCESS_OFF, 22

        .area   _CODE

_process_has_threads:
        ex      de, hl

        ld      bc, (_thread_first_running)
        call    .scan_list
        ret     nz

        ld      bc, (_thread_first_suspended)
        call    .scan_list
        ret     nz

        ld      bc, (_thread_first_waiting)
        call    .scan_list
        ret     nz

        ld      bc, (_thread_first_terminated)
        call    .scan_list
        ret     nz

        xor     a
        ret

.scan_list:
        ld      a, b
        or      c
        ret     z

.scan_loop:
        ld      h, b
        ld      l, c
        push    bc
        ld      bc, #THREAD_PROCESS_OFF
        add     hl, bc
        ld      a, (hl)
        inc     hl
        ld      h, (hl)
        ld      l, a
        or      a, a
        sbc     hl, de
        pop     bc
        jr      z, .found

        ld      h, b
        ld      l, c
        ld      c, (hl)
        inc     hl
        ld      b, (hl)
        ld      a, b
        or      c
        jr      nz, .scan_loop
        xor     a
        ret

.found:
        ld      a, #1
        ret
