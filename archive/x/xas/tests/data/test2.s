; Test File 2: Labels and Jumps
BEGIN:  NOP
        JP START
        JR SKIP
        LD A, 0x01
SKIP:   LD B, 0x02
        CALL FUNCTION
        RET

FUNCTION:
        ADD A, B
        RET

START:  LD A, 0x00
        HALT
