;--------------------------------------------------------
; File Created by SDCC : free open source ISO C Compiler
; Version 4.5.20 #16281 (Linux)
;--------------------------------------------------------
	.module list
	
	.optsdcc -mz80 sdcccall(1)
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _list_match_eq
	.globl _list_iterate
	.globl _list_find
	.globl _list_insert
	.globl _list_append
	.globl _list_remove
	.globl _list_remove_first
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
;--------------------------------------------------------
; absolute ram data
;--------------------------------------------------------
	.area _DABS (ABS)
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;/tmp/tmp.3ViYg2OK3V/list.c:19: uint8_t list_match_eq(list_item_t *p, uint16_t arg)
;	---------------------------------
; Function list_match_eq
; ---------------------------------
_list_match_eq::
;/tmp/tmp.3ViYg2OK3V/list.c:21: return (((uint16_t)p) == arg);
	cp	a, a
	sbc	hl, de
	ld	a, #0x01
	ret	Z
	xor	a, a
;/tmp/tmp.3ViYg2OK3V/list.c:22: }
	ret
;/tmp/tmp.3ViYg2OK3V/list.c:29: void list_iterate(
;	---------------------------------
; Function list_iterate
; ---------------------------------
_list_iterate::
	push	af
	push	af
	push	af
	dec	sp
	ld	c, l
	ld	b, h
	ld	iy, #5
	add	iy, sp
	ld	0 (iy), e
	ld	1 (iy), d
;/tmp/tmp.3ViYg2OK3V/list.c:34: list_item_t *slow = first;
	pop	de
	push	bc
;/tmp/tmp.3ViYg2OK3V/list.c:35: list_item_t *fast = first;
	ld	iy, #2
	add	iy, sp
	ld	0 (iy), c
	ld	1 (iy), b
;/tmp/tmp.3ViYg2OK3V/list.c:36: uint8_t guard = 0;
	inc	iy
	inc	iy
	ld	0 (iy), #0x00
;/tmp/tmp.3ViYg2OK3V/list.c:37: while (first)
00113$:
	ld	a, b
	or	a, c
	jp	z, 00116$
;/tmp/tmp.3ViYg2OK3V/list.c:39: fn(first, the_arg);
	push	bc
	ld	hl, #11
	add	hl, sp
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	bc
	ld	hl, #9
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ex	(sp), hl
	pop	iy
	call	___sdcc_call_iy
;/tmp/tmp.3ViYg2OK3V/list.c:40: first = first->next;
	pop	hl
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
;/tmp/tmp.3ViYg2OK3V/list.c:41: if (++guard == 0) break; /* probable corruption */
	ld	iy, #4
	add	iy, sp
	inc	0 (iy)
	ld	a, 0 (iy)
	or	a, a
	jr	z, 00116$
;/tmp/tmp.3ViYg2OK3V/list.c:42: if (slow) slow = (list_item_t *)slow->next;
	ld	iy, #0
	add	iy, sp
	ld	a, 1 (iy)
	or	a, 0 (iy)
	jr	z, 00104$
	pop	hl
	push	hl
	ld	a, (hl)
	inc	hl
	ld	e, (hl)
	ld	0 (iy), a
	ld	1 (iy), e
00104$:
;/tmp/tmp.3ViYg2OK3V/list.c:43: if (fast && fast->next)
	ld	iy, #2
	add	iy, sp
	ld	a, 1 (iy)
	or	a, 0 (iy)
	jr	z, 00106$
	pop	de
	pop	hl
	push	hl
	push	de
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	or	a, h
	jr	z, 00106$
;/tmp/tmp.3ViYg2OK3V/list.c:44: fast = (list_item_t *)((list_item_t *)fast->next)->next;
	ld	a, (hl)
	inc	hl
	ld	e, (hl)
	ld	0 (iy), a
	ld	1 (iy), e
	jr	00107$
00106$:
;/tmp/tmp.3ViYg2OK3V/list.c:45: else fast = NULL;
	xor	a, a
	ld	iy, #2
	add	iy, sp
	ld	0 (iy), a
	ld	1 (iy), a
00107$:
;/tmp/tmp.3ViYg2OK3V/list.c:46: if (slow && fast && slow == fast) break; /* cycle detected */
	ld	iy, #0
	add	iy, sp
	ld	a, 1 (iy)
	or	a, 0 (iy)
	jp	z, 00113$
	ld	a, 3 (iy)
	inc	iy
	inc	iy
	or	a, 0 (iy)
	jp	z, 00113$
	ld	a, 0 (iy)
	dec	iy
	dec	iy
	sub	a, 0 (iy)
	jp	nz, 00113$
	ld	a, 3 (iy)
	sub	a, 1 (iy)
	jp	nz, 00113$
00116$:
;/tmp/tmp.3ViYg2OK3V/list.c:48: }
	pop	af
	pop	af
	pop	af
	inc	sp
	pop	hl
	pop	af
	jp	(hl)
;/tmp/tmp.3ViYg2OK3V/list.c:55: list_item_t *list_find(
;	---------------------------------
; Function list_find
; ---------------------------------
_list_find::
	push	af
	dec	sp
	ld	c, l
	ld	b, h
	ld	iy, #1
	add	iy, sp
	ld	0 (iy), e
	ld	1 (iy), d
;/tmp/tmp.3ViYg2OK3V/list.c:61: uint8_t guard = 0;
	dec	iy
	ld	0 (iy), #0x00
;/tmp/tmp.3ViYg2OK3V/list.c:63: *prev = NULL;
	ld	l, 1 (iy)
	ld	h, 2 (iy)
	inc	iy
	xor	a, a
	ld	(hl), a
	inc	hl
	ld	(hl), a
;/tmp/tmp.3ViYg2OK3V/list.c:64: while (first && !match(first, the_arg))
00104$:
	ld	a, b
	or	a, c
	jr	z, 00106$
	push	bc
	ld	hl, #9
	add	hl, sp
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	bc
	ld	hl, #9
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ex	(sp), hl
	pop	iy
	call	___sdcc_call_iy
	pop	bc
	or	a, a
	jr	nz, 00106$
;/tmp/tmp.3ViYg2OK3V/list.c:66: *prev = first;
	ld	iy, #1
	add	iy, sp
	ld	l, 0 (iy)
	ld	h, 1 (iy)
	ld	(hl), c
	inc	hl
	ld	(hl), b
;/tmp/tmp.3ViYg2OK3V/list.c:67: first = first->next;
	ld	l, c
	ld	h, b
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
;/tmp/tmp.3ViYg2OK3V/list.c:68: if (++guard == 0) return NULL; /* probable cycle/corruption */
	dec	iy
	inc	0 (iy)
	ld	a, 0 (iy)
	or	a, a
	jr	nz, 00104$
	ld	de, #0x0000
	jr	00107$
00106$:
;/tmp/tmp.3ViYg2OK3V/list.c:71: return first;
	ld	e, c
	ld	d, b
00107$:
;/tmp/tmp.3ViYg2OK3V/list.c:72: }
	pop	af
	inc	sp
	pop	hl
	pop	af
	pop	af
	jp	(hl)
;/tmp/tmp.3ViYg2OK3V/list.c:77: list_item_t *list_insert(list_item_t **first, list_item_t *el)
;	---------------------------------
; Function list_insert
; ---------------------------------
_list_insert::
;/tmp/tmp.3ViYg2OK3V/list.c:79: el->next = *first;
	push	de
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	dec	hl
	push	hl
	ld	hl, #2
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	(hl), c
	inc	hl
	ld	(hl), b
	pop	hl
;/tmp/tmp.3ViYg2OK3V/list.c:80: *first = el;
	ld	(hl), e
	inc	hl
	ld	(hl), d
;/tmp/tmp.3ViYg2OK3V/list.c:81: return el;
;/tmp/tmp.3ViYg2OK3V/list.c:82: }
	pop	af
	ret
;/tmp/tmp.3ViYg2OK3V/list.c:87: list_item_t *list_append(list_item_t **first, list_item_t *el)
;	---------------------------------
; Function list_append
; ---------------------------------
_list_append::
	push	af
	dec	sp
	ld	a, l
	ld	iy, #1
	add	iy, sp
	ld	0 (iy), a
	ld	1 (iy), h
	ld	c, e
	ld	b, d
;/tmp/tmp.3ViYg2OK3V/list.c:91: uint8_t guard = 0;
	dec	iy
	ld	0 (iy), #0x00
;/tmp/tmp.3ViYg2OK3V/list.c:93: el->next = NULL; /* it's always the last element */
	ld	l, c
	ld	h, b
	xor	a, a
	ld	(hl), a
	inc	hl
	ld	(hl), a
;/tmp/tmp.3ViYg2OK3V/list.c:95: if (*first == NULL) /* empty list? */
	ld	l, 1 (iy)
	ld	h, 2 (iy)
	inc	iy
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	a, d
	or	a, e
	jr	nz, 00107$
;/tmp/tmp.3ViYg2OK3V/list.c:96: *first = el;
	ld	l, 0 (iy)
	ld	h, 1 (iy)
	ld	(hl), c
	inc	hl
	ld	(hl), b
	jr	00108$
00107$:
;/tmp/tmp.3ViYg2OK3V/list.c:99: current = *first;
;/tmp/tmp.3ViYg2OK3V/list.c:100: while (current->next)
00103$:
	ld	l, e
	ld	h, d
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	or	a, h
	jr	z, 00105$
;/tmp/tmp.3ViYg2OK3V/list.c:102: current = current->next;
	ex	de, hl
;/tmp/tmp.3ViYg2OK3V/list.c:103: if (++guard == 0) return NULL; /* probable cycle/corruption */
	ld	iy, #0
	add	iy, sp
	inc	0 (iy)
	ld	a, 0 (iy)
	or	a, a
	jr	nz, 00103$
	ld	de, #0x0000
	jr	00109$
00105$:
;/tmp/tmp.3ViYg2OK3V/list.c:105: current->next = el;
	ld	l, c
	ld	h, b
	ld	a, l
	ld	(de), a
	inc	de
	ld	a, h
	ld	(de), a
00108$:
;/tmp/tmp.3ViYg2OK3V/list.c:107: return el;
	ld	e, c
	ld	d, b
00109$:
;/tmp/tmp.3ViYg2OK3V/list.c:108: }
	pop	af
	inc	sp
	ret
;/tmp/tmp.3ViYg2OK3V/list.c:113: list_item_t *list_remove(list_item_t **first, list_item_t *el)
;	---------------------------------
; Function list_remove
; ---------------------------------
_list_remove::
	ld	iy, #-8
	add	iy, sp
	ld	sp, iy
	ld	c, l
	ld	b, h
	ld	iy, #6
	add	iy, sp
	ld	0 (iy), e
	ld	1 (iy), d
;/tmp/tmp.3ViYg2OK3V/list.c:116: if (el == *first)
	ld	l, c
	ld	h, b
	ld	a, (hl)
	ld	iy, #2
	add	iy, sp
	ld	0 (iy), a
	inc	hl
	ld	a, (hl)
	ld	1 (iy), a
;/tmp/tmp.3ViYg2OK3V/list.c:118: *first = el->next;
	ld	iy, #6
	add	iy, sp
	ld	a, 0 (iy)
	dec	iy
	dec	iy
	ld	0 (iy), a
	ld	a, 3 (iy)
	ld	1 (iy), a
;/tmp/tmp.3ViYg2OK3V/list.c:116: if (el == *first)
	ld	a, 2 (iy)
	ld	hl, #2
	add	hl, sp
	sub	a, (hl)
	jr	nz, 00105$
	ld	hl, #0x7
	add	hl, sp
	ld	a, (hl)
	ld	hl, #0x3
	add	hl, sp
	sub	a, (hl)
	jr	nz, 00105$
;/tmp/tmp.3ViYg2OK3V/list.c:118: *first = el->next;
	ld	hl, #4
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	a, e
	ld	(bc), a
	inc	bc
	ld	a, d
	ld	(bc), a
	jr	00106$
00105$:
;/tmp/tmp.3ViYg2OK3V/list.c:122: if (!list_find(*first, &prev, list_match_eq, (uint16_t)el))
	ld	hl, #6
	add	hl, sp
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	push	bc
	ld	hl, #_list_match_eq
	push	hl
	ld	hl, #4
	add	hl, sp
	ex	de, hl
	ld	hl, #6
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	call	_list_find
	ld	a, d
	or	a, e
	jr	nz, 00102$
;/tmp/tmp.3ViYg2OK3V/list.c:123: return NULL;
	ld	de, #0x0000
	jr	00107$
00102$:
;/tmp/tmp.3ViYg2OK3V/list.c:125: prev->next = el->next;
	pop	bc
	push	bc
	ld	hl, #4
	add	hl, sp
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	ld	a, e
	ld	(bc), a
	inc	bc
	ld	a, d
	ld	(bc), a
00106$:
;/tmp/tmp.3ViYg2OK3V/list.c:127: return el;
	ld	hl, #6
	add	hl, sp
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
00107$:
;/tmp/tmp.3ViYg2OK3V/list.c:128: }
	pop	af
	pop	af
	pop	af
	pop	af
	ret
;/tmp/tmp.3ViYg2OK3V/list.c:133: list_item_t *list_remove_first(list_item_t **first)
;	---------------------------------
; Function list_remove_first
; ---------------------------------
_list_remove_first::
;/tmp/tmp.3ViYg2OK3V/list.c:136: if (*first == NULL)
	ld	c,l
	ld	b,h
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	or	a, h
	jr	nz, 00102$
;/tmp/tmp.3ViYg2OK3V/list.c:137: return NULL; /* empty list */
	ld	de, #0x0000
	ret
00102$:
;/tmp/tmp.3ViYg2OK3V/list.c:138: result = *first;
	ld	e, l
	ld	d, h
;/tmp/tmp.3ViYg2OK3V/list.c:139: *first = (list_item_t *)((*first)->next);
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	(bc), a
	inc	bc
	ld	a, h
	ld	(bc), a
;/tmp/tmp.3ViYg2OK3V/list.c:140: return result;
;/tmp/tmp.3ViYg2OK3V/list.c:141: }
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
