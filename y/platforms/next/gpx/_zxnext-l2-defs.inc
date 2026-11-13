;; ZX Spectrum Next Layer 2 (320x256x8) constants.
.equ NXT_WIDTH,       320
.equ NXT_HEIGHT,      256
.equ NXT_PAGES,       5
.equ NXT_REG_SEL,     0x243b
.equ NXT_REG_DATA,    0x253b
.equ NXT_L2_PORT,     0x123b
.equ NXT_L2_BASE,     8          ; active Layer 2 bank (16 KiB units)
.equ NXT_L2_MAP,      0x07       ; read + write + display enabled
.equ NXT_L2_OFFSET,   0x10       ; 80 KiB offset mode
.equ NXT_L2_IDLE,     0x02       ; display enabled, CPU mapping disabled
.equ NXT_MODE_320,    0x10       ; NextReg 0x70: 320x256x8
.equ NXT_CO_FORE,     0x00       ; API foreground: black palette index
.equ NXT_CO_BACK,     0xff       ; API background: white palette index
