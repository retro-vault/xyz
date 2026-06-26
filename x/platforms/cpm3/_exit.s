        ;; _exit.s  (sys backend: CP/M 3)
        ;;
        ;; Publish the last requested process status and hand it off to BDOS so
        ;; CP/M 3 sees the return code before control returns to the CCP.




        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __cpm3_entry_sp
        .globl  __exit
        .globl  __exit_status

        .equ    BDOS,5
        .equ    CPM3_VERSION_MIN,0x31
        .equ    P_CODE,108
        .equ    P_TERMCPM,0
        .equ    S_BDOSVER,12
        .area   _CODE
__exit::
        ld      (__exit_status),hl
        xor     a
        ld      (0x0080),a             ; clear command tail to avoid replay on
                                       ; warm-boot style fallbacks
        push    ix
        push    iy
        push    bc
        push    de
        ld      c,#S_BDOSVER
        call    BDOS
        pop     de
        pop     bc
        pop     iy
        pop     ix
        ld      a,l
        cp      #CPM3_VERSION_MIN
        jr      c,__cpm3_exit_return_ccp
        ld      hl,(__exit_status)
        ex      de,hl
        push    ix
        push    iy
        ld      c,#P_CODE
        call    BDOS
        pop     iy
        pop     ix
__cpm3_exit_return_ccp:
        ld      c,#P_TERMCPM
        jp      BDOS
