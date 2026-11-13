        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        .ifndef YOS_EXTERNAL_FAR_GATE
        .equ YOS_EXTERNAL_FAR_GATE,0
        .endif
        ;; memory.s
        ;;
        ;; First-fit allocator for the Partner's two private RAM banks and its
        ;; common-memory window. Banked blocks are represented by xcc 24-bit
        ;; far pointers. All allocator state is kept at fixed common addresses,
        ;; and byte access is routed through the loader-installed trampoline.
        ;;
        ;; Block header (6 bytes): liblist link, payload size, free flag, magic.
        ;; Adjacent free blocks are eagerly coalesced.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module memory
        .optsdcc -mz80 sdcccall(1)

        .include "partner.inc"
        .include "list.inc"

        .globl  _memory_init
        .globl  _malloc
        .globl  _balloc
        .globl  _smalloc
        .globl  _free
        .globl  _sfree
        .globl  _setbank
        .globl  __far_getb
        .globl  __far_putb
        .globl  __list_far_next
        .globl  __list_far_set_next

        .equ    MEMORY_MAGIC,              0x4d42
        .equ    MEMORY_STATE_MAGIC,        0xc000
        .equ    MEMORY_CURRENT_BANK,       0xc002
        .equ    MEMORY_NEXT_BANK,          0xc003
        .equ    MEMORY_HEAP_BANK1,         0xc008
        .equ    MEMORY_HEAP_BANK2,         0xc010
        .equ    MEMORY_HEAP_SHARED,        0xc018
        .equ    MEMORY_TMP_BLOCK,          0xc020
        .equ    MEMORY_TMP_NEXT,           0xc022
        .equ    MEMORY_TMP_SIZE,           0xc024
        .equ    MEMORY_TMP_REQUEST,        0xc026
        .equ    MEMORY_TMP_BANK,           0xc028
        .equ    MEMORY_TMP_DESC,           0xc029
        .equ    MEMORY_TMP_OLD_SIZE,       0xc02b
        .equ    MEMORY_TMP_OLD_NEXT,       0xc02d
        .equ    MEMORY_SHARED_HEAP_BASE,   0xc800
        .equ    MEMORY_SHARED_HEAP_LIMIT,  PARTNER_LOADER_BASE

        .equ    HEAP_HEAD,                 0
        .equ    HEAP_BASE,                 2
        .equ    HEAP_LIMIT,                4
        .equ    HEAP_BANK,                 6
        .equ    HEAP_FLAGS,                7

        .equ    BLOCK_NEXT,                LIST_NODE_NEXT
        .equ    BLOCK_SIZE,                2
        .equ    BLOCK_FREE,                4
        .equ    BLOCK_MAGIC,               5
        .equ    BLOCK_HEADER_SIZE,         6
        .equ    BLOCK_MAGIC_VALUE,         0xa5

        .area   _CODE

        ;; ------------------------------------------------------------
        ;; memory_init
        ;; Initialize the bank-1, bank-2, and common arenas from the fixed
        ;; loader memory record.
        ;;
        ;; Signature: int memory_init(void)
        ;;
        ;; Returns: DE = 0 on success or -1 when the loader record is invalid
_memory_init::
        ld      hl,(MEMORY_STATE_MAGIC)
        ld      de,#MEMORY_MAGIC
        or      a
        sbc     hl,de
        jr      nz,.memory_init_cold
        ld      de,#0
        ret
.memory_init_cold:
        ld      hl,#PARTNER_MEMORY_INFO_MAGIC
        ld      de,#.memory_info_magic
        ld      b,#4
.memory_init_magic_loop:
        ld      a,(de)
        cp      (hl)
        jr      nz,.memory_init_fail
        inc     de
        inc     hl
        djnz    .memory_init_magic_loop
        ld      a,(PARTNER_MEMORY_INFO_BANK)
        cp      #1
        jr      z,.memory_init_bank_valid
        cp      #2
        jr      nz,.memory_init_fail
.memory_init_bank_valid:
        ld      (MEMORY_CURRENT_BANK),a
        ld      (MEMORY_NEXT_BANK),a

        ld      de,(PARTNER_MEMORY_INFO_BANK1_END)
        ld      a,#1
        ld      hl,#MEMORY_HEAP_BANK1
        call    .memory_init_heap
        jr      c,.memory_init_fail
        ld      de,(PARTNER_MEMORY_INFO_BANK2_END)
        ld      a,#2
        ld      hl,#MEMORY_HEAP_BANK2
        call    .memory_init_heap
        jr      c,.memory_init_fail
        ld      de,#MEMORY_SHARED_HEAP_BASE
        xor     a
        ld      hl,#MEMORY_HEAP_SHARED
        call    .memory_init_heap
        jr      c,.memory_init_fail
        ld      hl,#MEMORY_MAGIC
        ld      (MEMORY_STATE_MAGIC),hl
        ld      de,#0
        ret
.memory_init_fail:
        ld      hl,#0
        ld      (MEMORY_STATE_MAGIC),hl
        ld      de,#0xffff
        ret

.memory_info_magic:
        .ascii  "BMEM"

        ;; Initialize one descriptor and its first free block.
        ;; A=bank, DE=base, HL=descriptor. Carry indicates invalid bounds.
.memory_init_heap:
        ld      (MEMORY_TMP_BANK),a
        bit     0,e
        jr      z,.memory_init_heap_aligned
        inc     de
.memory_init_heap_aligned:
        ld      (MEMORY_TMP_BLOCK),de
        ld      (MEMORY_TMP_DESC),hl
        ld      bc,#PARTNER_BANK_LIMIT
        ld      a,(MEMORY_TMP_BANK)
        or      a
        jr      nz,.memory_init_heap_limit
        ld      bc,#MEMORY_SHARED_HEAP_LIMIT
.memory_init_heap_limit:
        push    hl
        ld      h,b
        ld      l,c
        or      a
        sbc     hl,de
        ld      bc,#(BLOCK_HEADER_SIZE + 2)
        or      a
        sbc     hl,bc
        pop     hl
        jr      c,.memory_init_heap_fail

        ld      bc,#PARTNER_BANK_LIMIT
        ld      a,(MEMORY_TMP_BANK)
        or      a
        jr      nz,.memory_init_heap_store
        ld      bc,#MEMORY_SHARED_HEAP_LIMIT
.memory_init_heap_store:
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (hl),c
        inc     hl
        ld      (hl),b
        inc     hl
        ld      a,(MEMORY_TMP_BANK)
        ld      (hl),a
        inc     hl
        ld      (hl),#1

        ld      h,b
        ld      l,c
        or      a
        sbc     hl,de
        ld      bc,#BLOCK_HEADER_SIZE
        or      a
        sbc     hl,bc
        ld      (MEMORY_TMP_SIZE),hl
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,(MEMORY_TMP_BANK)
        ld      de,#0
        call    __list_far_set_next
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_SIZE
        add     hl,de
        ld      de,(MEMORY_TMP_SIZE)
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    .memory_write_word
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        ld      a,#1
        call    PARTNER_FAR_PUT
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_MAGIC
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        ld      a,#BLOCK_MAGIC_VALUE
        call    PARTNER_FAR_PUT
        or      a
        ret
.memory_init_heap_fail:
        scf
        ret

        ;; ------------------------------------------------------------
        ;; malloc
        ;; Allocate from either private bank using round-robin preference.
        ;;
        ;; Signature: void * [[xcc::far]] malloc(size_t size)
        ;;
        ;; Arguments: HL = size
        ;; Returns: HL:E = far pointer, or zero
_malloc::
        ld      a,(MEMORY_NEXT_BANK)
        push    af
        push    hl
        call    .memory_allocate_bank
        pop     bc
        pop     af
        ld      d,a
        ld      a,h
        or      l
        jp      nz,.malloc_success
        ld      a,d
        xor     #3
        push    af
        ld      h,b
        ld      l,c
        call    .memory_allocate_bank
        pop     af
        ld      d,a
        ld      a,h
        or      l
        jr      z,.malloc_fail
.malloc_success:
        ld      a,d
        xor     #3
        ld      (MEMORY_NEXT_BANK),a
        ld      e,d
        ret
.malloc_fail:
        ld      hl,#0
        ld      e,#0
        ret

        ;; ------------------------------------------------------------
        ;; balloc
        ;; Allocate from one explicitly selected private bank.
        ;;
        ;; Signature: void * [[xcc::far]] balloc(size_t size, uint8_t bank)
        ;;
        ;; Arguments: HL = size, bank at 4(ix)
        ;; Returns: HL:E = far pointer, or zero
_balloc::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      a,4(ix)
        cp      #1
        jr      z,.balloc_valid
        cp      #2
        jr      nz,.balloc_fail
.balloc_valid:
        ld      d,a
        call    .memory_allocate_bank
        ld      a,(MEMORY_TMP_BANK)
        ld      e,a
        ld      a,h
        or      l
        jr      nz,.balloc_done
        ld      e,#0
.balloc_done:
        pop     ix
        ret
.balloc_fail:
        ld      hl,#0
        ld      e,#0
        pop     ix
        ret

        ;; ------------------------------------------------------------
        ;; smalloc
        ;; Allocate from common memory and return a normal pointer.
        ;;
        ;; Signature: void *smalloc(size_t size)
        ;;
        ;; Arguments: HL = size
        ;; Returns: DE = pointer, or zero
_smalloc::
        xor     a
        call    .memory_allocate_bank
        ex      de,hl
        ret

        ;; Internal first-fit allocator. A=bank and HL=request; returns HL.
.memory_allocate_bank:
        ld      (MEMORY_TMP_BANK),a
        ld      a,h
        or      l
        jp      z,.memory_allocate_fail
        inc     hl
        res     0,l
        ld      a,h
        or      l
        jp      z,.memory_allocate_fail
        ld      (MEMORY_TMP_REQUEST),hl
        ld      hl,(MEMORY_STATE_MAGIC)
        ld      de,#MEMORY_MAGIC
        or      a
        sbc     hl,de
        jp      nz,.memory_allocate_fail
        ld      a,(MEMORY_TMP_BANK)
        call    .memory_descriptor
        jp      c,.memory_allocate_fail
        ld      (MEMORY_TMP_DESC),hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (MEMORY_TMP_BLOCK),de
.memory_allocate_loop:
        .if YOS_CPC_AMSDOS
 ld a,(MEMORY_TMP_BANK)
 or a
 jp z,.cpc_common_find
        .else
        .endif
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,h
        or      l
        jp      z,.memory_allocate_fail
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        push    hl
        ld      de,#BLOCK_FREE
        add     hl,de
        call    PARTNER_FAR_GET
        pop     hl
        or      a
        jp      z,.memory_allocate_next
        push    hl
        ld      de,#BLOCK_SIZE
        add     hl,de
        call    .memory_read_word
        ld      (MEMORY_TMP_OLD_SIZE),de
        pop     hl
        ld      de,(MEMORY_TMP_REQUEST)
        ld      bc,(MEMORY_TMP_OLD_SIZE)
        ld      a,c
        sub     e
        ld      a,b
        sbc     a,d
        jp      c,.memory_allocate_next
        .if YOS_CPC_AMSDOS
.memory_allocate_selected:
        .else
        .endif
        call    .memory_split_block
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        xor     a
        call    PARTNER_FAR_PUT
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_HEADER_SIZE
        add     hl,de
        ret
.memory_allocate_next:
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_next
        ld      (MEMORY_TMP_BLOCK),de
        jp      .memory_allocate_loop
.memory_allocate_fail:
        ld      hl,#0
        ret

        ;; Split current block when a useful tail remains.
.memory_split_block:
        ld      hl,(MEMORY_TMP_OLD_SIZE)
        ld      de,(MEMORY_TMP_REQUEST)
        or      a
        sbc     hl,de
        ld      de,#(BLOCK_HEADER_SIZE + 2)
        or      a
        sbc     hl,de
        ret     c
        ;; The two-byte minimum is a fit test, not lost payload space.
        inc     hl
        inc     hl
        push    hl
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_next
        ld      (MEMORY_TMP_OLD_NEXT),de
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_HEADER_SIZE
        add     hl,de
        ld      de,(MEMORY_TMP_REQUEST)
        add     hl,de
        ld      (MEMORY_TMP_NEXT),hl
        pop     de
        ld      (MEMORY_TMP_SIZE),de

        ;; Initialize the split tail as a free bank-local list node.
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,(MEMORY_TMP_OLD_NEXT)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_set_next
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,#BLOCK_SIZE
        add     hl,de
        ld      de,(MEMORY_TMP_SIZE)
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    .memory_write_word
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        ld      a,#1
        call    PARTNER_FAR_PUT
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,#BLOCK_MAGIC
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        ld      a,#BLOCK_MAGIC_VALUE
        call    PARTNER_FAR_PUT

        ;; Shrink the current block and link it to the new tail.
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      bc,#BLOCK_SIZE
        add     hl,bc
        ld      de,(MEMORY_TMP_REQUEST)
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    .memory_write_word
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,(MEMORY_TMP_NEXT)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_set_next
        ret

        ;; ------------------------------------------------------------
        ;; free
        ;; Release a far allocation. The pointer's bank selects its heap.
        ;;
        ;; Signature: void free(void * [[xcc::far]] pointer)
_free::
        push    ix
        ld      ix,#0
        add     ix,sp
        ld      l,4(ix)
        ld      h,5(ix)
        ld      a,6(ix)
        call    .free_core
        pop     ix
        ;; xcc makes a function with a far-pointer parameter callee-clean.
        ;; Remove the three-byte address+bank argument before returning.
        pop     hl                      ; return address
        pop     bc                      ; far address
        inc     sp                      ; far bank byte
        jp      (hl)

        ;; ------------------------------------------------------------
        ;; sfree
        ;; Release a common-memory pointer returned by smalloc().
        ;;
        ;; Signature: void sfree(void *pointer)
        ;;
        ;; Arguments: HL = pointer
_sfree::
        xor     a
        jp      .free_core

.free_core:
        ld      (MEMORY_TMP_BANK),a
        ld      a,h
        or      l
        ret     z
        ld      de,#BLOCK_HEADER_SIZE
        or      a
        sbc     hl,de
        ld      (MEMORY_TMP_BLOCK),hl
        ld      a,(MEMORY_TMP_BANK)
        call    .memory_descriptor
        ret     c
        ld      (MEMORY_TMP_DESC),hl
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_MAGIC
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    PARTNER_FAR_GET
        cp      #BLOCK_MAGIC_VALUE
        ret     nz
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        ld      a,#1
        call    PARTNER_FAR_PUT
        jp      .memory_coalesce

        ;; Walk and merge adjacent free blocks in the selected heap.
.memory_coalesce:
        .if YOS_CPC_AMSDOS
 ld a,(MEMORY_TMP_BANK)
 or a
 jp z,.cpc_common_coalesce
        .else
        .endif
        ld      hl,(MEMORY_TMP_DESC)
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (MEMORY_TMP_BLOCK),de
.memory_coalesce_loop:
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,h
        or      l
        ret     z
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_next
        ld      (MEMORY_TMP_NEXT),de
        ld      a,d
        or      e
        ret     z
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    PARTNER_FAR_GET
        or      a
        jr      z,.memory_coalesce_advance
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,#BLOCK_FREE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    PARTNER_FAR_GET
        or      a
        jr      z,.memory_coalesce_advance
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      de,#BLOCK_SIZE
        add     hl,de
        call    .memory_read_word
        ld      (MEMORY_TMP_SIZE),de
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      bc,#BLOCK_HEADER_SIZE
        add     hl,bc
        add     hl,de
        ld      de,(MEMORY_TMP_NEXT)
        or      a
        sbc     hl,de
        jr      nz,.memory_coalesce_advance
        ld      hl,(MEMORY_TMP_NEXT)
        ld      de,#BLOCK_SIZE
        add     hl,de
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    .memory_read_word
        ld      hl,(MEMORY_TMP_SIZE)
        ld      bc,#BLOCK_HEADER_SIZE
        add     hl,bc
        add     hl,de
        ex      de,hl
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      bc,#BLOCK_SIZE
        add     hl,bc
        ld      a,(MEMORY_TMP_BANK)
        ld      c,a
        call    .memory_write_word
        ld      hl,(MEMORY_TMP_NEXT)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_next
        ld      hl,(MEMORY_TMP_BLOCK)
        ld      a,(MEMORY_TMP_BANK)
        call    __list_far_set_next
        jp      .memory_coalesce_loop
.memory_coalesce_advance:
        ld      hl,(MEMORY_TMP_NEXT)
        ld      (MEMORY_TMP_BLOCK),hl
        jp      .memory_coalesce_loop

        ;; Return descriptor in HL for A=0,1,2. Carry on invalid bank.
.memory_descriptor:
        or      a
        jr      z,.memory_descriptor_shared
        cp      #1
        jr      z,.memory_descriptor_one
        cp      #2
        jr      nz,.memory_descriptor_fail
        ld      hl,#MEMORY_HEAP_BANK2
        or      a
        ret
.memory_descriptor_one:
        ld      hl,#MEMORY_HEAP_BANK1
        or      a
        ret
.memory_descriptor_shared:
        ld      hl,#MEMORY_HEAP_SHARED
        or      a
        ret
.memory_descriptor_fail:
        scf
        ret

        ;; Read/write little-endian words through the far byte dispatcher.
        ;; C=bank, HL=address. Read returns DE; both leave HL at address+1.
.memory_read_word:
        .if YOS_CPC_AMSDOS
 ld a,c
 or a
 jr nz,.memory_read_word_far
 ld e,(hl)
 inc hl
 ld d,(hl)
 ret
.memory_read_word_far:
        .else
        .endif
        call    PARTNER_FAR_GET
        ld      e,a
        inc     hl
        call    PARTNER_FAR_GET
        ld      d,a
        ret
.memory_write_word:
        .if YOS_CPC_AMSDOS
 ld a,c
 or a
 jr nz,.memory_write_word_far
 ld (hl),e
 inc hl
 ld (hl),d
 ret
.memory_write_word_far:
        .else
        .endif
        ld      a,e
        call    PARTNER_FAR_PUT
        inc     hl
        ld      a,d
        call    PARTNER_FAR_PUT
        ret

        ;; ------------------------------------------------------------
        ;; setbank
        ;; Update the bank restored by far-pointer accesses after application
        ;; code has performed its own persistent bank transition.
        ;;
        ;; Signature: int setbank(uint8_t bank)
        ;;
        ;; Arguments: A = bank
        ;; Returns: DE = 0 or -1
_setbank::
        cp      #1
        jr      z,.memory_set_bank_ok
        cp      #2
        jr      nz,.memory_set_bank_fail
.memory_set_bank_ok:
        ld      (MEMORY_CURRENT_BANK),a
        ld      de,#0
        ret
.memory_set_bank_fail:
        ld      de,#0xffff
        ret

        ;; xcc far-pointer hooks. The loader-installed routines preserve BC,
        ;; DE, and HL and return after restoring MEMORY_CURRENT_BANK.
        .if YOS_CPC_AMSDOS

        ;; Freeing UI objects used to traverse the common heap through two
        ;; far-byte calls per link. Walk its near headers directly instead.
        ;; Keep the far allocator's six-byte header and adjacency checks.
.cpc_common_coalesce:
        push ix
        push iy
        ld hl,(MEMORY_TMP_DESC)
        ld e,(hl)
        inc hl
        ld d,(hl)
.cpc_coalesce_next:
        ld a,d
        or e
        jr z,.cpc_coalesce_done
        push de
        pop ix
.cpc_coalesce_current:
        ld e,0(ix)
        ld d,1(ix)
        ld a,d
        or e
        jr z,.cpc_coalesce_done
        ld a,4(ix)
        or a
        jr z,.cpc_coalesce_next
        push de
        pop iy
        ld a,4(iy)
        or a
        jr z,.cpc_coalesce_next
        ld l,2(ix)
        ld h,3(ix)
        ld bc,#6
        add hl,bc
        push de
        push ix
        pop de
        add hl,de
        pop de
        or a
        sbc hl,de
        jr nz,.cpc_coalesce_next
        ld l,2(ix)
        ld h,3(ix)
        ld e,2(iy)
        ld d,3(iy)
        add hl,de
        add hl,bc
        ld 2(ix),l
        ld 3(ix),h
        ld a,0(iy)
        ld 0(ix),a
        ld a,1(iy)
        ld 1(ix),a
        jr .cpc_coalesce_current
.cpc_coalesce_done:
        pop iy
        pop ix
        ret

        ;; First-fit on common headers needs no far calls or per-node scratch
        ;; stores. Keep the original split routine and allocation bookkeeping.
.cpc_common_find:
        push ix
        ld ix,(MEMORY_TMP_BLOCK)
        ld bc,(MEMORY_TMP_REQUEST)
.cpc_find_next:
        push ix
        pop hl
        ld a,h
        or l
        jr z,.cpc_find_failed
        ld a,4(ix)
        or a
        jr z,.cpc_find_advance
        ld e,2(ix)
        ld d,3(ix)
        ld a,e
        sub c
        ld a,d
        sbc a,b
        jr c,.cpc_find_advance
        ld (MEMORY_TMP_OLD_SIZE),de
        ld (MEMORY_TMP_BLOCK),hl
        pop ix
        jp .memory_allocate_selected
.cpc_find_advance:
        ld l,0(ix)
        ld h,1(ix)
        push hl
        pop ix
        jr .cpc_find_next
.cpc_find_failed:
        pop ix
        jp .memory_allocate_fail

.cpc_memory_get:
        ld a,h
        cp #0xc0
        jp c,__far_getb
        ld a,(hl)
        ret
        .endif

        ;; A desktop supplies its own bank-safe gates. The standalone Partner
        ;; substrate retains forwarding entries for the boot loader gates.
        .if YOS_CPC_AMSDOS + YOS_EXTERNAL_FAR_GATE
        .else
__far_getb::
        jp      PARTNER_FAR_GET
__far_putb::
        jp      PARTNER_FAR_PUT
        .endif
