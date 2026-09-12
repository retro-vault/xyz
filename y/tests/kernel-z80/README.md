# YOS Kernel Regression

Run from the repository root:

```sh
make -C y/src/z80 test
```

The owning Makefile builds all artifacts under `build/yos-z80/` and
`bin/y/z80/spectrum/bin/`. Besides kernel, graphics and scheduler tests,
the harness executes the actual `shell.sys` and `shelllib.svc` produced
by XCC/XAS/XLD/XPROG.

`mock_filesystem.h` intercepts the initialized esxDOS **RAM gates**, not
the loader or its POSIX wrappers. It simulates short reads, native errors
and firmware register clobbers. Every intercepted gate must reject an IM2
interrupt, and success/error returns must preserve enclosing critical
sections. The shell loads the library and calls its
self-registered, relocated interface. An instruction checkpoint verifies
that registration is still private while the initializer runs.

`test_libraries.h` covers two clients, repeated acquisitions, independent
private instances, different ABIs, surviving sibling threads, last-client
cleanup, malformed descriptors/XL/exports, CRC errors, failed initialization,
and exhausted image/object/reference heaps.

This is deterministic ROM-level coverage, not real esxDOS firmware or
hardware validation. Use the [Fuse cold-boot runner](../fuse/README.md) for
visual validation with actual esxDOS. The shell should display "Library OK"
below its greeting.
