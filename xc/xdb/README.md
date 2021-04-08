# xdb

a source level debugger for the xyz project

## projects

the xyz source level debugger **xdb** emulates gdb, fooling most gdb front ends 
into thinking they are talking to a real gdb. it reads listings, created by **sdcc**
tools to map C and assembler code with the binary. 

following projects are in this folder

 1. gdb-sim this is **the xdb**.
 2. gdb-server-sim. this is a simulator for gdb-server. you can use it to fake
    server side. it listens to port 2000, dumps protocol requests, and enables you
    to examine the responses.