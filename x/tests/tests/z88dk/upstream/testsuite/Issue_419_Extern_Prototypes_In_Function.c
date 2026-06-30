

void func(int a)
{
	[[z88dk::fastcall]] extern void ZXN_NEXTREG_0x50(unsigned char) __preserves_regs(a,b,c,d,e,h,l,iyl,iyh); 
	ZXN_NEXTREG_0x50(a); 
}
