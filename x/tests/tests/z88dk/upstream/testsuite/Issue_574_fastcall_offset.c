void func(long x) [[z88dk::fastcall]] {
}

void WaitForBOF(void) [[z88dk::fastcall]]
{
#asm
WaitForBOF:
	ld	bc,0x243B
	ld	a,0x1f
	out	(c),a
	ld	bc,0x253B
	in	a,(c)
	cp	192
	jr	nz,WaitForBOF
#endasm

}
