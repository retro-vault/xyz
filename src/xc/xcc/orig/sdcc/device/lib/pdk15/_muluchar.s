	.module _muluchar
	.optsdcc -mpdk15

	.area DATA

	.area	OSEG (OVR,DATA)
__muluchar_PARM_1::
	.ds 1
__muluchar_PARM_2::
	.ds 1

	.area CODE

; unsigned int _muluchar (unsigned char x, unsigned char y)
__muluchar::

	mov	a, __muluchar_PARM_1
	mov	p, a
	mov	a, #0x08
	mov	__muluchar_PARM_1, a
	mov	a, #0x00

0$:
	sl	a
	slc	p
	t0sn.io f, c
	add	a, __muluchar_PARM_2
3$:
	addc	p

	dzsn	__muluchar_PARM_1
	goto	0$
4$:

	ret

