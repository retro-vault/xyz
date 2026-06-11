; Basic crt0 for xcc Z80 C23 test cases
; Sets up stack and calls main, then halts.
; Compatible with sdcccall(1) ABI used by the project.

	.module crt0
	.globl _main

	.area _CODE

init:
	ld sp, #0xFE00		; stack in high memory (same as project's test harness)
	call _main
	halt

	.area _CABS (ABS)

; If needed, can define areas for data, but for simple cases ok.

	.area _DATA
	.area _BSS
