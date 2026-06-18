        ;; _exit.s  (sys backend: CP/M 3)
        ;;
        ;; Publish the last requested process status and hand it off to BDOS so
        ;; CP/M 3 sees the return code before control returns to the CCP.




        .module _exit
        .optsdcc -mz80 sdcccall(1)

        .globl  __exit
        .globl  __exit_status

        .equ    BDOS,5
        .equ    P_CODE,108
        .equ    P_TERMCPM,0

        .area   _CODE
__exit::
        ld      (__exit_status),hl
        ex      de,hl
        push    ix
        push    iy
        ld      c,#P_CODE
        call    BDOS
        ld      c,#P_TERMCPM
        jp      BDOS
