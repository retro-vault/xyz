; Test File 3: Complex Instructions and Directives
ORG 0x1000
START:  LD HL, 0x2000
        LD (HL), 0x42
        LD A, (HL)
        CP 0x42
        JP Z, MATCH
        JP MISMATCH

MATCH:  LD B, 0x01
        JR END

MISMATCH:
        LD B, 0x00

END:    HALT
DB 0xFF, 0x00, 0xAA, 0x55
