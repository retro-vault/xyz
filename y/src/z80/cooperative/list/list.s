        .ifndef YOS_CPC_AMSDOS
        .equ YOS_CPC_AMSDOS,0
        .endif
        ;; list.s
        ;;
        ;; Allocation-free intrusive forward lists. Public functions implement
        ;; the xcc/sdcccall(1) C ABI. Two internal primitives operate on the
        ;; same two-byte link stored in banked RAM for libmemory.
        ;;
        ;; MIT License (see: LICENSE)
        ;; Copyright (C) 2026 Tomaz Stih
        ;;
        ;; 2026-08-12   tstih

        .module list
        .optsdcc -mz80 sdcccall(1)

        .include "partner.inc"
        .include "list.inc"

        .globl  _list_init
        .globl  _list_empty
        .globl  _list_front
        .globl  _list_next
        .globl  _list_push_front
        .globl  _list_push_back
        .globl  _list_insert_after
        .globl  _list_pop_front
        .globl  _list_remove
        .globl  _list_count
        .globl  __list_far_next
        .globl  __list_far_set_next

        .area   _CODE

        ;; void list_init(list_t *list)
_list_init::
        ld      a,h
        or      l
        ret     z
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ret

        ;; uint8_t list_empty(const list_t *list)
_list_empty::
        ld      a,h
        or      l
        jr      z,.list_true
        ld      a,(hl)
        inc     hl
        or      (hl)
        jr      z,.list_true
        xor     a
        ret
.list_true:
        ld      a,#1
        ret

        ;; list_node_t *list_front(const list_t *list)
_list_front::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ret

        ;; list_node_t *list_next(const list_node_t *node)
_list_next::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ret
.list_pointer_null:
        ld      de,#0
        ret

        ;; list_node_t *list_push_front(list_t *list, list_node_t *node)
_list_push_front::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      a,d
        or      e
        jr      z,.list_pointer_null
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        dec     hl
        ex      de,hl                   ; HL=node, DE=list
        ld      (hl),c
        inc     hl
        ld      (hl),b
        dec     hl
        ex      de,hl                   ; HL=list, DE=node
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret

        ;; list_node_t *list_push_back(list_t *list, list_node_t *node)
_list_push_back::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      a,d
        or      e
        jr      z,.list_pointer_null
        push    hl                      ; preserve list address
        ex      de,hl                   ; clear node->next
        xor     a
        ld      (hl),a
        inc     hl
        ld      (hl),a
        dec     hl
        ex      de,hl                   ; DE=node
        pop     hl                      ; HL=list
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        dec     hl
        ld      a,b
        or      c
        jr      nz,.list_push_back_walk
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret
.list_push_back_walk:
        push    de                      ; node being appended
        ld      h,b
        ld      l,c                     ; current node
.list_push_back_next:
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        dec     hl
        ld      a,d
        or      e
        jr      z,.list_push_back_found
        ex      de,hl
        jr      .list_push_back_next
.list_push_back_found:
        pop     de
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret

        ;; list_node_t *list_insert_after(list_node_t *position,
        ;;                                list_node_t *node)
_list_insert_after::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      a,d
        or      e
        jr      z,.list_pointer_null
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        dec     hl
        ex      de,hl                   ; HL=node, DE=position
        ld      (hl),c
        inc     hl
        ld      (hl),b
        dec     hl
        ex      de,hl                   ; HL=position, DE=node
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ret

        ;; list_node_t *list_pop_front(list_t *list)
_list_pop_front::
        ld      a,h
        or      l
        jr      z,.list_pointer_null
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        dec     hl
        ld      a,d
        or      e
        ret     z
        ex      de,hl                   ; HL=node, DE=list
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        xor     a
        ld      (hl),a
        dec     hl
        ld      (hl),a                  ; detached node->next = NULL
        ex      de,hl                   ; HL=list, DE=node
        ld      (hl),c
        inc     hl
        ld      (hl),b
        ret

        ;; list_node_t *list_remove(list_t *list, list_node_t *node)
_list_remove::
        ld      a,h
        or      l
        jp      z,.list_pointer_null
        ld      a,d
        or      e
        jp      z,.list_pointer_null
        ld      b,d
        ld      c,e                     ; BC=target
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        dec     hl                      ; HL=list, DE=head
        ld      a,d
        or      e
        jp      z,.list_pointer_null
        ld      a,e
        cp      c
        jr      nz,.list_remove_walk_start
        ld      a,d
        cp      b
        jr      z,.list_remove_head
.list_remove_walk_start:
        ex      de,hl                   ; HL=current
.list_remove_walk:
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        dec     hl                      ; DE=current->next
        ld      a,d
        or      e
        jp      z,.list_pointer_null
        ld      a,e
        cp      c
        jr      nz,.list_remove_advance
        ld      a,d
        cp      b
        jr      z,.list_remove_after
.list_remove_advance:
        ex      de,hl
        jr      .list_remove_walk
.list_remove_after:
        push    hl                      ; predecessor
        ld      h,b
        ld      l,c                     ; target
        ld      e,(hl)
        inc     hl
        ld      d,(hl)                  ; target successor
        xor     a
        ld      (hl),a
        dec     hl
        ld      (hl),a                  ; detach target
        pop     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      d,b
        ld      e,c
        ret
.list_remove_head:
        push    hl                      ; list
        ld      h,b
        ld      l,c                     ; target/head
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        xor     a
        ld      (hl),a
        dec     hl
        ld      (hl),a
        pop     hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        ld      d,b
        ld      e,c
        ret

        ;; size_t list_count(const list_t *list)
_list_count::
        ld      a,h
        or      l
        jr      z,.list_count_zero
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      bc,#0
.list_count_loop:
        ld      a,h
        or      l
        jr      z,.list_count_done
        inc     bc
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        jr      .list_count_loop
.list_count_done:
        ld      d,b
        ld      e,c
        ret
.list_count_zero:
        ld      de,#0
        ret

        ;; Internal bank-aware link primitives. Bank-local lists still store a
        ;; compact 16-bit next address because all of their nodes share A=bank.
        ;;
        ;; __list_far_next: A=bank, HL=node; returns DE=next.
__list_far_next::
        ld      c,a
        ld      a,h
        or      l
        jr      z,.list_far_null
        call    PARTNER_FAR_GET
        ld      e,a
        inc     hl
        call    PARTNER_FAR_GET
        ld      d,a
        ret
.list_far_null:
        ld      de,#0
        ret

        ;; __list_far_set_next: A=bank, HL=node, DE=next.
__list_far_set_next::
        ld      c,a
        ld      a,h
        or      l
        ret     z
        ld      a,e
        call    PARTNER_FAR_PUT
        inc     hl
        ld      a,d
        call    PARTNER_FAR_PUT
        ret
