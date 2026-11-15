        ; Loader-bindable restart gates, shared by every Z80 application.
        ; Defaults preserve the existing YOS ABI on machines with visible RSTs.
        ; The XPRG header identifies these exact slots; no opcode scanning.
        .module yos_call_gates
        .optsdcc -mz80 sdcccall(1)
        .globl __yos_gate18, __yos_gate20, __yos_gate28, __yos_gate30
        .area _CODE
__yos_gate18:: jp 0x18
__yos_gate20:: jp 0x20
__yos_gate28:: jp 0x28
__yos_gate30:: jp 0x30
