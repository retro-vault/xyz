# yos

the y operating system
 

# yos internals
## memory layout
## startup code
## vectors and interrupts

## video 

`video.c` contains raw (and very fast!) video memory access 
functions that are used by both front ends; the `tty` and the
`zwin`.

## tty 

## services

### implementing syscalls in yos

every operating system provides a mechanism for system calls. 
to access the yos operating system api, you have to obtain
pointer to the yos services. a service is a table of functions. 
the yos operating system service is called (surprise, surprise!) 
yos. 

you acquire the service pointer and call a function like this.

~~~cpp
#include <yos.h>
/* get the yos operating system service */
yos_t *y=(yos_t*)query_service("yos");
y->printf("Hello world!\n");
~~~

### custom services

good news is you can register your own services, and provide your
own table of functions. as long as the process that registers 
the service is running, other processes can obtain the pointer
to it and call its api. 

this is venerated as one of the coolest features of yos! 
you can use it to implement a plethora of hacks: from shared 
resources, and dynamic linking, to interprocess communication.
simply create a process that register a service, and call its 
functions from other processes.

 > max name len for the service is 16 chars, together with 0 terminator.

### the query_service() function

the query_service() function is implemented in crt0 file and
by default accessible from any C program. from assembler,
you can call it like this:

~~~asm
    ;; put pointer to service name on the stack
    ld hl, #yos
    push hl
    ;; call reset vector 10
    rst 0x10
    ;; hl now contains pointer to table of functions
    yos: .ascii "yos"
~~~

### partial standard c library implementation

operating systems usually expose mminimal set of system functions. 

yos is a bit different.

parts of it require functions from the standard c library, such as
`strlen()` or `malloc()`. so to save precious memory and avoid 
duplication, yos exposes its guts to the world and lets you use
those functions instead of reimplementing them.

you get a partial implementation of `time.h`, `string.h`, `ctype.h`,
`stdio.h`... as part of the operating system.

## bookkeeping the system resources

everything yos allocates (a memory block, an event, 
a timer...) is a system object. yos keeps records of it,   including information about the owning thread. if the later 
is killed (or dies naturally), all owned system objects are
released. 

this makes it possible not only to kill threads, but also
to monitor resource usage per thread.

## memory management
## timers
## the keyboard driver
## threads
## synchronization events
## processes

### resident processes

resident processes are loaded to fixed addresses at 
the end of physical memory.

### godot and the relocation
## the network
### rs232 driver
### the data link and multiplexing the connection
#### burst comms
## servers

the data link protocol is used to obtain real time clock,
file, and inet proxy services from the pc. 

### rpc
### time server
### file server
### inet proxy server