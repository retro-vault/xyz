# yos

the y operating system
 

# yos internals
## memory layout
## startup code
## vectors and interrupts
## video 
## tty output

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