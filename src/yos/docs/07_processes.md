# Processes

When you run a program from the network or a disk,
you create a process. Hence the process is an active 
(running) instance of a program.

each process has a **name** (max 8 chars), **process flags**, 
**main thread** and optional child threads. A process in xyz
is created by calling function `process_start`, and terminated
by calling function `process_exit`.

## The process_start

This function accepts five arguments: 
 - the name of the process
 - TODO: process flags
 - TODO: pointer to process memory or NULL for system processes 
 - entry point of the process
 - desired stack size.

It creates a new process_t structure and adds it to the list
of running processes. It also creates and attaches a new thread 
to the process, allocates stack for it, and resumes it.

## The process_exit

This function is an analogy of the standard C `exit()` function. 
It releases process resources and removes the process from 
the list of running processes. 

 > If you don't call this function explicitly, it is called
 > implicitly by the operating system. When your program returns
 > from main, the startup code `crt0` of a yos program regains 
 > control and exits.