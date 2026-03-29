; Test File 1: Basic Instructions
START:  LD A, 0xFF
        LD B, A
        INC B
        DEC B
        LD (0x1234), A
        HALT