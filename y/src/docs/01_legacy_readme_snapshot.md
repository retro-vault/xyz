![status.badge] [![language.badge]][language.url] [![standard.badge]][standard.url] 

The YOS
=======

  - [The Boot Process](#the-boot-process)
      - [Z80 Power Up](#z80-power-up)
      - [RST jumps to RAM?\!](#rst-jumps-to-ram)
      - [Special interrupts](#special-interrupts)
          - [The 50Hz Screen Refresh](#the-50hz-screen-refresh)
          - [The NMI interrupts](#the-nmi-interrupts)
      - [The Init Subroutine](#the-init-subroutine)
      - [RAM initialization](#ram-initialization)
      - [Safely Disabling and Enabling
        Interrupts](#safely-disabling-and-enabling-interrupts)
      - [Hook RST vectors](#hook-rst-vectors)
      - [The Stack and the Heap](#the-stack-and-the-heap)
  - [Resource Accounting](#resource-accounting)
      - [Linked lists](#linked-lists)
      - [Derivation of system object from list
        item](#derivation-of-system-object-from-list-item)
  - [Cleaning up resources](#cleaning-up-resources)
  - [Memory Management](#memory-management)
  - [Threads](#threads)
  - [Processes](#processes)
      - [The process\_start](#the-process_start)
      - [The process\_exit](#the-process_exit)
      - [services](#services)
          - [implementing syscalls in
            yos](#implementing-syscalls-in-yos)
          - [custom services](#custom-services)
          - [the query\_service() function](#the-query_service-function)
          - [partial standard c library
            implementation](#partial-standard-c-library-implementation)
          - [resident processes](#resident-processes)
      - [the clock](#the-clock)

# The Boot Process

## Z80 Power Up

When the *Z80* microprocessor is powered up, it starts executing program
at location `0x0000`. In *yos* this is the location of the
[crt0rom.s](https://github.com/tstih/xyz/blob/main/src/yos/startup/crt0rom.s)
file. Let’s take a look at its code.

The first instruction disables interrupts and the second jumps to the
`init` subroutine. The jump is followed by 4 bytes holding the *yos*
version. These three parts are exactly 8 bytes long.

``` asm
        .org    0x0000
        di                              ; disable interrupts
        jp      init                    ; init
        .db     1,0,0,0                 ; the version is 1.0.0.0
```

Z80 has eight single byte calls: `RST 0x00`, `RST 0x08`, `RST 0x10`,
`RST 0x18`, `RST 0x20`, `RST 0x28`, `RST 0x30`, and `RST 0x38`. Each
call pushes current address to the stack, and jumps to the predefined
location. Since RST command only has 8 bits it can’t store full 16-bit
address for the jump, so it is assumed that high byte for the jump is
always 0. Consequently, if you execute RST 0x18 it will call address
0x0018 (decimal: 24).

As mentioned, initial code is exactly 8 bytes long. It is followed by
the `RST 0x08` handler below i.e. the target address for the `RST 0x08`
call.

``` asm
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
        ;; rst 0x18
        jp      rst18
rst18ret:
        reti
        .db     0,0,0
        ;; rst 0x20
        jp      rst20
rst20ret:
        reti
        .db     0,0,0
        ;; ... the pattern repeats ...
```

Each two restart handlers are exactly 8 bytes apart. Observe the code
above. The `jp` command has 3 bytes, the `reti` adds 2 more, and 3
padding bytes are added at the end with the `.db` directive. All
together exactly 8 bytes, followed by the next restart vector.

Each restart handler starts with a jump to a label (called `rst8`,
`rst10`, etc…) So where do these labels point to?

## RST jumps to RAM?\!

The answer to previous question is: these labels point to *RAM*. The
reason for this is simple: we want to be able to change them. And we
can’t change the code in *ROM*. So our code in ROM needs to jump to
addresses in RAM which can be changes.

And here’s the code in RAM.

``` asm
__sys_vec_tbl::
        rst8:   .ds     3
        rst10:  .ds     3
        rst18:  .ds     3
        rst20:  .ds     3
        rst28:  .ds     3
        rst30:  .ds     3
        rst38:  .ds     3
        nmi:    .ds     3
```

These are undefined labels. The *yos* is a ROM program. It is not loaded
into the RAM. So it uses a trick. At start up it copies following code
to the RAM location with label `__sys_vec_tbl` address.

``` asm
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
```

And this is the piece of code that copies the vectors.

``` asm
        ;; move vector table to RAM
        ld      hl,#start_vectors
        ld      de,#__sys_vec_tbl
        ld      bc,#end_vectors - #start_vectors
        ldir
```

So what happens when the `RST 0x08` instruction is executed? The system
first jumps to the address 0x0008 and executes instruction `jp rst8`.
The label `rst8` points to initialized RAM. At that address in RAM there
is another jump `jp rst8ret`. This one jumps back to ROM, and continues
by executing instruction `reti`.

Because it jumps to *RAM* first you can change the jump at label `rst8`
hence you can install a hook that executes when `rst 0x08` is called.

## Special interrupts

### The 50Hz Screen Refresh

Every 1/50th of a second - the ZX Spectrum refreshes the screen. If it
is operating in the interrupt mode 1, it executes the `RST 0x38`. Hence,
the code at location `0x0038` is normally called 50 times per seconds.

> The *yos* already heavily utilizes this interrupt (for timers,
> drivers, and threads), so unless you know what you are doing, don’t
> hook into it. If your program needs to be notified every 1/50th of a
> second, use the *timer API* instead.

### The NMI interrupts

The non-maskable interrupt behaves in the same way as any other
interrupt. It is located at location `0x0066`, but instead of returning
with the `reti`, this interrupt returns with the `retn`.

The *ZX Spectrum* does not use *NMI* interrupt, but some hardware does.

## The Init Subroutine

The operating systems’ `init` subroutine sets up the stack, copies reset
vectors and global variables from *ROM* to *RAM*, enters the interrupt
mode 1, enables interrupts, and calls the C `main()` function.

``` asm
init:
        ld      sp,#__sys_stack         ; now sp to OS stack (on bss)
        call    gsinit                  ; init static vars

        ;; start interrupts
        im      1                       ; im 1, 50Hz interrupt on ZX Spectrum
        ei                              ; enable interrupts

        ;; call the main!
        call    _main

tarpit:
        halt                            ; halt
        jr      tarpit
```

## RAM initialization

Since the operating system runs in *ROM*, all the global *C* variables
must be copied to *RAM* at start up; otherwise, they would be constants.
In addition, our reset vectors must be initialized.

The code to do that is by convention placed into the `_GSINIT` segment,
which must end with `_GSFINAL`. Also, by convention the *SDCC* places
additional initialization code into this segment.

All initialization values for the global variables will be placed into
the *SDCC* segment `s_INITIALIZER` in *ROM*, and copied by our code to
the `s__INITIALIZED` segment in *RAM*.

``` asm
        .area   _GSINIT
gsinit:
        ;; move vector table to RAM
        ld      hl,#start_vectors
        ld      de,#__sys_vec_tbl
        ld      bc,#end_vectors - #start_vectors
        ldir

        ;; initialize vars from initializer
        ld      de, #s__INITIALIZED
        ld      hl, #s__INITIALIZER
        ld      bc, #l__INITIALIZER
        ld      a, b
        or      a, c
        jr      z, gsinit_none
        ldir
gsinit_none:
        .area   _GSFINAL
        ret
```

## Safely Disabling and Enabling Interrupts

Instructions `di` and `ei` can be quite harmful if not used correctly.
Imagine two subroutines: `subroutine_a` and `subroutine_b` both
disabling interrupts at start and enabling it when they return. What
happens if we call the latter from the first?

``` asm
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
```

`subroutine_b` does not know internals of `subroutine_a`. It simply
enables interrupts upon completion. Unfortunately that happens when
`subroutine_a` is still executing. So code after the call to
`subroutine_b` will not be guarded by a `di` anymore.

To protect our code against this we must do reference counting. These
are the two subroutines that do that.

``` asm
        ;; -------------------------
        ;; extern void ir_disable();
        ;; -------------------------
        ;; executes di with ref count.
        ;; affects: -
_ir_disable::   
        di
        push    hl
        ld      hl,#ir_refcount
        inc     (hl)
        pop     hl
        ret

        ;; ------------------------
        ;; extern void ir_enable();
        ;; ------------------------
        ;; executes ei with ref count.
        ;; affects: -
_ir_enable::
        push    af
        ld      a,(#ir_refcount)
        or      a
        jr      z,do_ei                 ; if a==0 then just ei      
        dec     a                       ; if a<>0 then dec a
        ld      (#ir_refcount),a        ; write back to counter
        or      a                       ; and check for ei
        jr      nz,dont_ei              ; not yet...
do_ei:      
        ei
dont_ei:    
        pop     af
        ret

ir_refcount:
        .ds     1
```

## Hook RST vectors

Finally, we can write routines that enable you to hook vectors. When
changing vectors we are going to disable interrupts. These routines will
simply write new vector to *RAM*.

``` asm
        ;; -----------------------------------------------------------
        ;; extern void sys_vec_set(void (*handler)(), uint8_t vec_num);
        ;; ------------------------------------------------------------
        ;; affects: bc, de, hl
_sys_vec_set::
        call    _ir_disable
        pop     hl                      ; ret address / ignore
        pop     bc                      ; bc = handler
        pop     de                      ; d = undefined, e = vector number
        ;; restore stack
        push    de
        push    bc
        push    hl
        ld      d,#0x00                 ; de = 16 bit vector number
        ld      hl,#__sys_vec_tbl       ; vector table start
        add     hl,de
        add     hl,de
        add     hl,de
        inc     hl                      ; hl = hl + 3 * de + 1
        ;; hl now points to vector address in RAM       
        ;; so set it to handler value in bc
        ld      (hl),c
        inc     hl
        ld      (hl),b
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
        add     hl,de
        add     hl,de
        add     hl,de
        inc     hl                      ; hl = hl + 3 * de + 1
        ld      e,(hl)                  ; vector into de
        inc     hl
        ld      d,(hl)
        ex      de,hl                   ; and into hl
        call    _ir_enable
        ret
```

## The Stack and the Heap

*Yos* maintains 512 bytes of separate operating system stack and 1024
bytes of operating system heap, which are both placed to the *BSS* (in
*RAM*) segment at the end of the
[crt0rom.s](https://github.com/tstih/xyz/blob/main/src/yos/startup/crt0rom.s)
file.

The common user heap (label `__heap`) starts immediately after the
system heap. Since each user process is assigned a separate stack, there
is no common user stack.

``` asm
        .area   _BSS
        ;; 512 bytes of operating system stack
        .ds     512
__sys_stack::


        .area   _HEAP
__sys_heap::
        ;; 1KB of system heap
        .ds     1024
__heap::
```

# Resource Accounting

Every system object in the **xyz os** is a resource. Examples of
resources are: threads, memory blocks, synchronization objects, etc. The
operating system keeps books of all allocated resources.

> Resource accounting makes it possible not only to automatically clean
> thread resources, but also to monitor resource usage.

## Linked lists

Resources of same type are kept in linked lists. Generic functions to
manage linked lists are inside `list.c`. These functions work on any
data structure that has a header, binary compatible with the
`list_item_t` structure.

``` cpp
typedef struct list_item_s
{
    void *next;         /* next list el. */
} list_item_t;
```

Linked link functions don’t care about the rest of the structure.

## Derivation of system object from list item

Besides being a member of a linked list, each systemm resource also has
an owner. The owner is typically a process that created the resource.
For example if a process allocates a memory block then it becomes the
owner of that memory block.

This is why each system resource has an enriched header, which is binary
compatible with the `sysobj_t` structure, but also an extension of a
`list_item_t`. In object oriented terms we would think of these as
derived classes.

``` cpp
typedef struct sysobj_s {
    union {
        list_item_t hdr;
        void* next;                     
    };
    void* owner;                        /* owners' id */
} sysobj_t;
```

Additional functionality for managing system objects is and the
`sysobj.c` file.

# Cleaning up resources

When a process dies it is the job of the operating system to release all
resources that this process allocated (and has not freed\!). But
resource cleansing operation does different things depending on resource
type. Killing a thread is obviously different to releasing a memory
block.

Hence forceach resource there exists a destructor function; and the kill
function calls them all in correct order.

…to be continued…

# Memory Management

xyz provides functions for allocating and freeing memory blocks, and an
ability to use multiple heaps.

> xyz uses one heap for operating system and the other for user
> programs.

…to be continued…

# Threads

# Processes

When you run a program from the network or a disk, you create a process.
Hence the process is an active (running) instance of a program.

each process has a **name** (max 8 chars), **process flags**, **main
thread** and optional child threads. A process in xyz is created by
calling function `process_start`, and terminated by calling function
`process_exit`.

## The process\_start

This function accepts five arguments: - the name of the process - TODO:
process flags - TODO: pointer to process memory or NULL for system
processes - entry point of the process - desired stack size.

It creates a new process\_t structure and adds it to the list of running
processes. It also creates and attaches a new thread to the process,
allocates stack for it, and resumes it.

## The process\_exit

This function is an analogy of the standard C `exit()` function. It
releases process resources and removes the process from the list of
running processes.

> If you don’t call this function explicitly, it is called implicitly by
> the operating system. When your program returns from main, the startup
> code `crt0` of a yos program regains control and exits. \# System
> Calls and Services

## services

### implementing syscalls in yos

every operating system provides a mechanism for system calls. to access
the yos operating system api, you have to obtain pointer to the yos
services. a service is a table of functions. the yos operating system
service is called (surprise, surprise\!) yos.

you acquire the service pointer and call a function like this.

``` cpp
#include <yos.h>
/* get the yos operating system service */
yos_t *y=(yos_t*)query_service("yos");
y->printf("Hello world!\n");
```

### custom services

good news is you can register your own services, and provide your own
table of functions. as long as the process that registers the service is
running, other processes can obtain the pointer to it and call its api.

this is venerated as one of the coolest features of yos\! you can use it
to implement a plethora of hacks: from shared resources, and dynamic
linking, to interprocess communication. simply create a process that
register a service, and call its functions from other processes.

> max name len for the service is 16 chars, together with 0 terminator.

### the query\_service() function

the query\_service() function is implemented in crt0 file and by default
accessible from any C program. from assembler, you can call it like
this:

``` asm
        ;; put pointer to service name on the stack
        ld hl, #yos
        push hl
        ;; call reset vector 10
        rst 0x10
        ;; hl now contains pointer to table of functions
        yos: .ascii "yos"
```

### partial standard c library implementation

operating systems usually expose mminimal set of system functions.

yos is a bit different.

parts of it require functions from the standard c library, such as
`strlen()` or `malloc()`. so to save precious memory and avoid
duplication, yos exposes its guts to the world and lets you use those
functions instead of reimplementing them.

you get a partial implementation of `time.h`, `string.h`, `ctype.h`,
`stdio.h`… as part of the operating system.

### resident processes

resident processes are loaded to fixed addresses at the end of physical
memory.

…to be continued…

## the clock

yos provides standard C function `clock()` and also internally maintains
the `time_t` structure.

> the clock depends on screen blank interval and can be affected by
> disabling interrupts for a longer periods. hence periodical update of
> the clock over the serial net is recommended.

…to be continued…

[language.url]:   https://isocpp.org/
[language.badge]: https://img.shields.io/badge/language-c-blue.svg

[standard.url]:   https://en.wikipedia.org/wiki/C_(programming_language)
[standard.badge]: https://img.shields.io/badge/standard-c11-blue.svg

[status.badge]:  https://img.shields.io/badge/status-development-red.svg