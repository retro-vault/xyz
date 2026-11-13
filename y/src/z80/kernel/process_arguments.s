        ; Verbatim launch argument, stored in a sized descriptor tail.
        .module process_arguments
        .optsdcc -mz80 sdcccall(1)
        .globl _process_load_args, _process_arguments, _process_load
        .globl _process_last_error
        .globl _thread_current, _enter_critical_section, _leave_critical_section
        .globl __process_argument_length
        .globl __process_descriptor_size
        .area _BSS
        ; Only the locked argument loader changes this allocation hint.
__process_argument_length:: .ds 1
        .area _ROM_ARGS
        ; HL=executable, DE=argument (NULL means empty). Copy banked input
        ; before loading; scheduling stays locked until the child owns it.
_process_load_args::
        push ix
        push hl
        ld hl,#-128
        add hl,sp
        ld sp,hl
        push hl
        pop ix
        ld b,#128
        ld a,d
        or e
        jr z,.empty
.copy:
        ld a,(de)
        ld (hl),a
        inc de
        inc hl
        or a
        jr z,.load
        djnz .copy
        ld a,#4
        ld (_process_last_error),a
        ld de,#0
        jr .done
.empty:
        ld (hl),#0
.load:
        call _enter_critical_section
        ld a,#128
        sub b
        ld (__process_argument_length),a
        ld hl,#128
        add hl,sp
        ld a,(hl)
        inc hl
        ld h,(hl)
        ld l,a
        call _process_load
        ld a,d
        or e
        jr z,.unlock
        push de
        ld hl,#17
        add hl,de
        ex de,hl
        push ix
        pop hl
        ld b,#128
.transfer:
        ld a,(hl)
        ld (de),a
        inc hl
        inc de
        or a
        jr z,.transferred
        djnz .transfer
.transferred:
        pop de
.unlock:
        xor a
        ld (__process_argument_length),a
        call _leave_critical_section
.done:
        ld hl,#130
        add hl,sp
        ld sp,hl
        pop ix
        ret
        .area _ROM_ARGS
_process_arguments::
        ; Initializers override resource ownership, not the caller's process.
        ld de,#.no_arguments
        ld hl,(_thread_current)
        ld a,h
        or l
        ret z
        ld bc,#23
        add hl,bc
        ld c,(hl)
        inc hl
        ld b,(hl)
        ld a,b
        or c
        ret z
        ld hl,#17
        add hl,bc
        ex de,hl
        ret
.no_arguments: .db 0
        .area _ROM_AUX
        ; Isolate this small helper so the classic ROM can pack it in a trap
        ; gap without changing the common process-creation implementation.
__process_descriptor_size::
        ld a,(__process_argument_length)
        add a,#18                      ; header, argument and NUL terminator
        ld e,a
        ld d,#0
        ret
