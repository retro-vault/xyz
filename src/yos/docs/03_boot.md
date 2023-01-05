# The Boot Process

## Z80 RST calls

Z80 has eight single byte *restart* commands: RST 0x00, RST 0x08, RST 0x10, RST 0x18, RST 0x20, RST 0x28, RST 0x30, and RST 0x38. Each command pushes current address to the stack, and jumps to the location that follows the command. Since RST command only has 8 bits it can't store full 16 bit address for the jump so it is assummed that the address is in page zero of memory. If you execute RST 0x18 it will call address 0x0018 (decimal: 24). 

*By convention memory pages in Z80 are 256 bytes long. This way the high byte of 16 byte address can be viewed as the page and the lower byte as offset within the page.*

For RST calls to work, some sort of handler code must be located at target addresses.

## Z80 Power Up 

When the Z80 microprocesssor is powered up, it starts executing program at location zero. This also happens to be the target location of a RST 0x00 call. Here is the xyz os code at location 0x0000.

~~~asm
        .org    0x0000
        di                              ; disable interrupts
        jp      init                    ; init
        .db     0,0,0,0                 ; 4 reserved bytes for version
~~~

The program at address zero disables interrupts and jumps to the init section. At the end 4 bytes are added so that next available address ix 0x08 i.e. the target address for the RST 0x08 call. This pattern is used for all restart call handlers. This is how the code following the above code looks.

~~~asm
        ;; rst 0x08 at address 0x0008
        jp      rst8
rst8ret:
        reti
        .db     0,0,0
        ;; rst 0x10 at address 0x0010
        jp      rst10
rst10ret:
        reti
        .db     0,0,0
~~~

This two vectors are exactly 8 bytes apart. The `jp` command has 3 bytes, the `reti` adds two more, and three padding bytes are added at the end. 

As we see each rst vector starts with a jump to a label (called rst8, rst10, etc...) So where do these labels point to?

## Why RST calls to RAM?

The answer to previous question is: these labels point to RAM. The reason for this is simple: we want to be able to change them. And we can't change the code in ROM. So our code in ROM needs to jump to addresses in RAM which can be changes. 

And here's the code in RAM.

~~~asm
__sys_vec_tbl::
        rst8:   .ds     3
        rst10:  .ds     3
        rst18:  .ds     3
        rst20:  .ds     3
        rst28:  .ds     3
        rst30:  .ds     3
        rst38:  .ds     3
        nmi:    .ds     3
~~~

As you see thse are undefined labels. Which is logical since xzy is a ROM program. It is not loaded in RAM. So it uses a trick. At start up it copies following code to the `__sys_vec_tbl` address.

~~~asm
start_vectors:
        jp      rst8ret
        jp      rst10ret
        jp      rst18ret
        jp      rst20ret
        jp      rst28ret
        jp      rst30ret
        jp      rst38ret
        jp      nmiret
end_vectors:
~~~

This is the piece of code that copies the vectors.

~~~asm
        ;; move vector table to RAM
        ld      hl,#start_vectors
        ld      de,#__sys_vec_tbl
        ld      bc,#end_vectors - #start_vectors
        ldir
~~~

Now addresses in RAM are defined. So what happens when the RST 0x08 instruction is detected? The system first executes instruction at 0x0008 which is `jp rst8`. The label `rst8` points to initialized RAM. At that address there is another jump `jp rst8ret`. This one jumps back to ROM, and continues by executing instruction `reti`. But now you can change the jump at label `rst8` hence you can create a hook that executes when `rst 0x08` is called.

# Safely Disabling and Enabling Interrupts

Instructions `di` and `ei` can be quite harmful if not used correctly. Imagine two subroutines: `subroutine_a` and `subroutine_b` both disabling interrupts at start and enabling it when they return. What happens if we call the later from the first?

~~~asm
subroutine_a:
        di
        call subroutine_b
        ;; your code here, but ei was already
        ;; called by subroutine_b so this code
        ;; is not guarded by a di anymore
        ei
        ret

subroutine_b:
        di
        ;; your code here
        ei
        ret
~~~

`subroutine_b` does not know internals of `subroutine_a`. It simply enables interrupts upon completion. Unfortunately that happens when `subroutine_a` is still executing. So code after the call to `subroutine_b` will not be guarded by a `di` anymore.

To protect our code against this we must do reference counting. Following two functions do exactly that. 

~~~asm
        ;; -------------------------
        ;; extern void ir_disable();
        ;; -------------------------
        ;; executes di with ref count.
        ;; affects: -
_ir_disable::	
        di
        push    hl
        ld		hl,#ir_refcount
        inc		(hl)
        pop     hl
        ret

        ;; ------------------------
        ;; extern void ir_enable();
        ;; ------------------------
        ;; executes ei with ref count.
        ;; affects: -
_ir_enable::
        push    af
        ld		a,(#ir_refcount)
        or		a
        jr		z,do_ei					; if a==0 then just ei		
        dec		a						; if a<>0 then dec a
        ld		(#ir_refcount),a	    ; write back to counter
        or		a						; and check for ei
        jr		nz,dont_ei				; not yet...
do_ei:		
        ei
dont_ei:	
        pop     af
        ret

ir_refcount:
        .ds		1
~~~

# Hook RST vectors

Finally we can write routines that enable you to hook vectors. When changing vectors we are going to disable interrupts. These routines will simply write new address to RAM after the jump opcode.

~~~asm
        ;; -----------------------------------------------------------
        ;; extern void sys_vec_set(void (*handler)(), uint8_t vec_num);
        ;; ------------------------------------------------------------
        ;; affects: bc, de, hl
_sys_vec_set::
        call    _ir_disable
        pop		hl                      ; ret address / ignore
        pop		bc                      ; bc = handler
        pop		de                      ; d = undefined, e = vector number
        ;; restore stack
        push	de
        push	bc
        push	hl
        ld		d,#0x00                 ; de = 16 bit vector number
        ld		hl,#__sys_vec_tbl       ; vector table start
        add		hl,de
        add		hl,de
        add		hl,de
        inc		hl						; hl = hl + 3 * de + 1
        ;; hl now points to vector address in RAM		
        ;; so set it to handler value in bc
        ld		(hl),c
        inc		hl
        ld		(hl),b
        call    _ir_enable
        ret

        ;; ------------------------------------------
        ;; extern void *sys_vec_get(uint8_t vec_num);
        ;; ------------------------------------------
        ;; affects: hl, de
_sys_vec_get::
        call    _ir_disable
        pop     de                      ; d = undefied, e = #vector
        ld      d,#0                    ; de = 16bit vector number
        ld      hl,#__sys_vec_tbl       ; vector table to hl
        add		hl,de
        add		hl,de
        add		hl,de
        inc		hl                      ; hl = hl + 3 * de + 1
        ld      e,(hl)                  ; vector into de
        inc     hl
        ld      d,(hl)
        ex      de,hl                   ; and into hl
        call    _ir_enable
        ret
~~~


...to be continued...

