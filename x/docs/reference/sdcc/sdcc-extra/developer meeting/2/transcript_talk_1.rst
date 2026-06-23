{first talk and discussion, using slide set "Introduction.pdf"}

+--------------------------------------------------------------------------+
|                                   SDCC                                   |
|                                                                          |
|                         Small Device C Compiler                          |
+==========================================================================+
|                           Philipp Klaus Krause                           |
|                                                                          |
|                                2025-04-11                                |
+--------------------------------------------------------------------------+

**Isolde (host):**

So, welcome to the SDCC meeting!

I am not going to say much.

It is my pleasure to have Philipp here and Benedikt and everybody else who
is involved in the Small Device C Compiler.

Philipp did his PhD with me, so he is both an expert in theory, but also
used to develop the techniques we were using for our theoretical results for
improving the SDCC compiler, which has been quite impressive and he has got
a number of publications in that area, but he is also very active in the
open source software community and he has, {facing Philipp} you have, some
grants as well like NLnet and the few European grants that support that kind
of work.

So I think he is a very interesting person to talk to, bridging there two
areas -- being very practical as well -- plus the open source community as
well et cetera, but let me hand over to you.

Maybe you can say a bit about the program. It is a very open meeting, right?
So there is time for talks and discussions -- very informal -- maybe you can
say a bit more about how it is going to run.

**Philipp:**

Thanks!
So basically the idea is that we start with four short talks, about 15
minutes, about 15 minutes for our discussion each.

When we get tired in between, we have a break, and after those four talks
and the discussions for that, basically the rest of the meeting is just open
discussion on future work and the future of the Small Device C Compiler.

The four talks we have today:

I start with an introduction to the Small Device C Compiler for those not
that familiar with the details and at the end of this first talk we will get
a bit to the rough challenges and ideas for the future.

Afterwards, we'll have a talk on the treedec library, a library that is used
by the Small Device C Compiler for some of its graph algorithms.

Then there will be a talk on the current state and future plans of standard
compliance. That is: C standard compliance.

Of course there is other standards relevant to the Small Device C Compiler,
but the C language standard is definitely the most important one, and at the
end there will be a talk on basically the Z80 family and the port for it in
SDCC.

Okay. So I start with the first talk.

+--------------------------------------------------------------------------+
| What is SDCC?                                                            |
+==========================================================================+
| * Standard C compiler: ISO C90, ISO C99, ISO C11, ISO C23,               |
|   ISO C2Y                                                                |
| * Freestanding implementation or part of a hosted                        |
|   implementation                                                         |
| * Supporting tools (assembler, linker, simulator, ...)                   |
| * Works on many host systems                                             |
| * Targets various 8-bit architectures, has some unusual                  |
|   optimizations that make sense for these targets                        |
| * Latest release: 4.5.0 (2025-01-28)                                     |
| * User base: embedded developers and retrocomputing/-gaming              |
|   enthusiasts                                                            |
| * Also used in downstream projects (z88dk, gbdk, devkitSMS)              |
+--------------------------------------------------------------------------+

So what is the Small Device C Compiler?

Well, it is a standard C compiler.

We support the relevant ISO standards starting with ISO C90, which is
basically a rebrand of the 1989 ANSI C standard, up to the current ISO C23
standard and we also have started a bit of support for the ISO C2y standard,
which will probably be an ISO standard.

So there is currently -- both in the C and C++ communities -- some
discontent with ISO, so we can't fully rule out that the C2y or future
standards might not be ISO standards any more.

So it's a free standing implementation or part of a hosted implementation.
Basically these are two models of C implementation:

The freestanding implementation is required to support the language, but
only part of the standard library.

A hosted implementation has a more complete standard library such as file
I/O and so on.

SDCC is on one hand a traditional model compiler with things such as
compile, assemble and link and so on, but the project also provides other
tools: In particular assemblers, linkers and simulators for the target
architectures.

It works on common host systems, be it Hurd, OpenBSD or whatever, and of
course the standard stuff such as GNU/Linux.

It targets various 8 bit architectures and has unusual optimizations that
make sense for these targets, in particular the register allocator, which
uses tree decomposition of the control flow graphs to be able to efficiently
allocate variables into registers.

We typically have about one release per year:

The last few years it always was at the beginning of the year, same this
year.

The user base consists of two groups about the same size:

The one are embedded developers for 8 bit microcontrollers and the other
groups are retrocomputing and retrogaming enthusiasts, because well, there
is 8 bit architectures there, as well.

And sometimes there is even some overlap and some old 8 bit architecture is
used in embedded systems, but also used in some retro things.

We'll get to that especially during the last talk with the Z80 derivatives.

Some users of SDCC do not use it directly, but by downstream projects that
include (typically) packaged SDCC together with libraries.

The most important ones are probably the Z80 related retrocomputing
communities:

z88dk, the Gameboy development kit and the devkit for SEGA 8 bit systems.
Historically this also sometimes happened with Infineon when they were
producing 8051 microcontrollers, and they did so the first time -- actually
are doing now again, because they bought Cypress and now they have 8051
again --, but I don't think they revived their fork of SDCC for that.

+--------------------------------------------------------------------------+
| Ports                                                                    |
+==========================================================================+
| * MCS-51, DS390, STM8, f8, HC08, S08, PDK13, PDK14,                      |
|   PDK15 (PIC14, PIC16)                                                   |
| * MOS 6502, WDC 65C02                                                    |
| * Z80, Z80N, Z180, eZ80, TLCS-90, SM83, Rabbits, R800                    |
+--------------------------------------------------------------------------+

Okay this is a list of target architectures we currently support:

MCS-51 is the architecture of the 8051 microcontrollers originally by Intel.
Probably the most important singular architecture there is, because while
Intel hasn't been making these things for decades there is still dozens of
manufacturers of 8051 compatible microcontrollers and they are kind of
everywhere.

I mean: I see laptops on the desks and a typical laptop will have an 8051
core in the keyboard controller.

If we have one of the very common Realtek Wifi chips, there's embedded
controllers in there. Some of them are ARM, but, as far as I know, nearly
every Realtek also has an 8051 compatible controller in there.

The Dallas Semiconductor DS390 is a derivative of the 8051.
STMicroelectronics' STM8 is a quite common European 8 bit controller.
The f8 is an 8 bit soft core architecture, [a] relatively recent one.
The HC08 and S08 by NXP (now) are also another common 8 bit microcontroller
family.

Then we get to the Padauk ones:

Padauk is a Taiwanese manufacturer of very low-cost 8 bit microcontrollers.
Their low end devices are less than one cent each for a typical system on
a chip and even the more expensive ones tend to be 3 to 5 cents each.
We also have support for the Microchip PIC architectures, but it's
unmaintained.

There are basically the 8 bit families relevant mostly to/in the embedded
area.

Then we recently got ports for the 6502 and one of its derivatives and the
rest is basically Z80 derivatives, starting with the original Z80, and going
to the Rabbits port, and the eZ80 port are the most advanced ones for the
last we'll have a separate talk, the last one today, on the Z80 families.

+--------------------------------------------------------------------------+
| Optimal Register Allocation in Polynomial Time                           |
+==========================================================================+
| * Register allocator based on graph-structure theory                     |
| * Optimal register allocation in polynomial time                         |
| * Flexible through use of cost function                                  |
| * Provides substantial improvements in code quality                      |
| * Slow compilation for targets with many registers                       |
| * Compilation speed / code quality trade-off:                            |
| * ``--max-allocs-per-node``                                              |
+--------------------------------------------------------------------------+

So one of these optimizations I mentioned before is optimal register
allocation in polynomial time.

Now if we have unrestricted input, register allocation is an NP-hard
problem.

However, if you fix an upper bound on the number of registers and fix an
upper bound on the tree-width of the control-flow graph, then it can be done
in polynomial time using methods from graph structure theory.

The upper bound on the tree-width of the control-flow graph is not something
entirely artificial, but it actually is a natural property of many programs.
For many programming languages it's actually given and for the programming
language C and, since SDCC is a C compiler, having a bound on the tree-width
of the control-flow graph translates to an upper bound on the number of
labels for "goto" per function.

Now the typical C programmer uses "goto" rarely and the typical use of
"goto" is to have like one cleanup label at the end of the function, so the
bounded tree-width is something we actually do have for C programs.

The whole register allocator is very flexible:

We have a cost function. Basically, the register allocator can ask questions
to code generation: "How many bytes would you generate for this part of the
program assuming things would be allocated a certain way?"

It gets slow for many registers.

The theoretical upper bound on the number of registers is in the exponent
and we allow for the compilation speed versus the quality of the generated
code a tradeoff that is an upper bound on intermediate results in the
algorithm that we use: If we set it very high we get the provably optimal
still polynomial times and if we set it to a lower value  then at some point
the heuristic kicks in, which means that we no longer get provably optimal
results, but we still tend to get reasonable results for typical code.

+--------------------------------------------------------------------------+
| Bytewise Register Allocation and Spilling                                |
+==========================================================================+
| * Decide on the storage of variables bytewise                            |
| * Decide for each individual byte in a variable whether to store         |
|   it in memory or a register                                             |
| * Consider any byte of any register as a possible storage location       |
+--------------------------------------------------------------------------+

{referring to the slide title} Another aspect that we have.

Not in general, even with this optimal register allocator, though most
target architectures use it now.

It's only the MCS-51, DS390 and the PIC ports they do not use this one yet.
One feature that is currently only implemented for the STM8 and f8 is the
bytewise register allocation spilling, meaning if you have variables in your
program that are bigger than 8 bits then instead of allocating them to
registers, spilling them, rematerializing them as a whole, we actually do
this on individual bytes.

If you have like a long long variable or a long variable, 32 bits, four
bytes each, each individual byte might be put into a register or spilled
onto the stack or rematerialized as is needed, which is quite useful to the
small architectures that have very few registers.

It becomes less important for those architectures that have a few more.
So basically any byte of any register could hold any byte of any local
variable in the program, and all these possibilities are computed by the
register allocator.

+--------------------------------------------------------------------------+
| SDCC vs. non-free compilers: STM8 Benchmark scores                       |
+==========================================================================+
| .. image:: scores-2024.png                                               |
+--------------------------------------------------------------------------+

So how does this, err, how is the practical result of that?

This is from results from about a year ago for the STM8 target architecture:

The purple lines are SDCC, the other colors are three non-free, commercial
compilers for the same architecture.

I've used three standard benchmarks:

Whetstone, Dhrystone and Coremark, because these benchmarks are small enough
that they fit on small 8 bit microcontrollers, but are still big enough to
give meaningful results.

Is anyone not familiar with the benchmarks?

Okay. So Whetstone is a floating point benchmark. Now you might think
floating point is not that relevant on the smaller architectures.
They don't have floating point hardware, which means floating point is
implemented in software.

Implementing floating point in software means a lot of bitwise shifting,
bitwise AND, bitwise OR, which is exactly what people are doing on the small
8 bit architectures.

So though people are typically not doing floating point on small 8 bit
architectures, a floating point software benchmark still tends to be
reasonably representative of what people do.

Then there's the Dhrystone benchmark: That's an integer benchmark from
systems programming that's definitely another relevant one, and there's
Coremark which is a newer benchmark.

In terms of performance, it places heavy emphasis on matrix multiplications,
integer matrix multiplications, which is a bit unfortunate, because on the
small architecture this kind of dominates the whole thing.

Dhrystone has often been accused of putting to much emphasis on standard
library string operations which to some degree is true, so for benchmarks of
course be aware that Dhrystone and Coremark are heavily influenced by
certain aspects.

Anyway, we see that SDCC is doing relatively well here.

For Whetstone the point is that for STM8 we do not use a hand written
assembler implementation of the floating point library.

Instead, it is just C code that gets compiled, while the others use
handwritten assembler implementations like we do for the 8051 but not for
STM8.

Still, we are doing quite well in performance of Dhrystone and Coremark
when we compare to other C compilers, and looking at the code size we are
also quite competitive.

+--------------------------------------------------------------------------+
| SDCC vs. non-free compilers: STM8 Code size                              |
+==========================================================================+
| .. image:: sizes-2024.png                                                |
+--------------------------------------------------------------------------+

Basically, we are kind of second best, for Raisonace generated smaller code
for all cases, but Raisonance also tended to be kind of the slowest and
while Raisonance is really only doing well in terms of code size, they don't
support any recent C standards and so on, for example.

Anyway, we can see that SDCC is a competitive compiler, at least for the
STM8 architecture, and this is in terms of code size and speed definitely
also due to the register allocation, and that, I think, we are definitely
better at than the other compilers.

+--------------------------------------------------------------------------+
| Regression testing                                                       |
+==========================================================================+
| * Regression testing of nightly snapshots                                |
| * ≈ 32000 tests (thrice as many as 2020) compiled and                    |
| * executed on simulators                                                 |
| * Tests mostly from fixed bugs and from GCC                              |
| * Targets architectures: MCS-51, DS390, Z80, Z180, eZ80,                 |
|   Rabbit 2000, Rabbit 2000A, Rabbit 3000A, SM83, TLCS-90,                |
|   HC08, S08, STM8, PDK14, PDK15.                                         |
| * Host OS: GNU/Linux, macOS, “Windows” (cross-compiled on                |
|   GNU/Linux, tested via wine)                                            |
| * Host architectures: x86, amd64, ppc64, aarch64                         |
+--------------------------------------------------------------------------+

Okay.
It's a compiler, it should do its job reliably, because if your compiler has
a bug and you write a bug free program in source code and then the compiled
binary has a bug, that's something very annoying and hard to debug, so
reliability is important.

We have a compile farm where there's a nightly regression testing, so
basically, each machine in the farm checks out SDCC from the current
repository, compiles it and then uses SDCC to compile a large number of test
programs and executes them on simulators and then checks that the results
are what we expect.

In addition, there's also a small number of tests that get compiled and are
supposed to not compile and we just check that the error messages are at the
right line for code that should not compile and that we get errors and
warnings and so on in the right places but the main thing is definitly the
tests  that get compiled, should compile and then get executed on the
simulators.

The main two sources of these test cases are bugs that we fixed in the past
and the GCC test suite.

This is done for the stable target architectures and it's done on a few host
systems:

In particular a GNU/Linux, macOS and what's called the "windows snapshots"
is actually compiled for windows but then executed via Wine on GNU/Linux and
there's currently four different host architectures on which this is done,
because, well, we can definitely have a bug in the compiler that only gets
exposed when it's a 32 or a 64 bit system, only a big versus a small endian
system. -- The main motivation why we are having a big endian 64 bit PowerPC
in the compile farm.

+--------------------------------------------------------------------------+
| Challenges                                                               |
+==========================================================================+
| Many target architectures of SDCC have been discontinued. How            |
| can SDCC stay relevant outside of the retrocomputing niche?              |
|                                                                          |
| * STM8 - SDCC is doing well, and ST put many STM8 devices                |
|   back to active                                                         |
| * MCS-51 - SDCC port needs major work to be competitive vs.              |
|   Keil                                                                   |
| * PDK - Not an easy target for a C compiler                              |
| * ALP - Don’t know much about current state                              |
| * S08 - SDCC port needs some work, hard to get into dev                  |
|   community                                                              |
| * eZ80 - hard to get into dev community                                  |
| * TLCS-870/C1 - no SDCC port yet, hard to get into dev                   |
|   community                                                              |
| * f8                                                                     |
+--------------------------------------------------------------------------+

Okay. Basically that's it about what SDCC is.

Then the production test SDCC, but SDCC is currently facing some challenges:
Basically we support 8 bit architectures, but a lot of them have been
discontinued, recently.

A lot of the traditional 8 bit microcontroller manufacturers have either
gotten out of the market or been trying to shift their users to 32 bit ARM,
like STMicroelectronics.

I mean: Yes, there's the STM8, it's doing well, but as I said, ST put many
STM8 devices back to active.

In between they had made an announcement they'll discontinue them and they
then took it back for like half of them but still STMicroelectronics, I mean
for years there definitely was an internal struggle in the company between
the 8 bit people and the 32 bit people.

So these are architectures that are listed here that are still in production
today and relevant today:

The STM8, MCS-51.

As I said, there's a lot of manufactures that they won't die easily even if
one of the manufactures will stop there are still some left.
It's not a really nice architecture for a C compiler and even though they
are our oldest supported architectures, it's not up to standard of the other
ports.

It still uses linear scan range register allocator which, well, is fast but
does not generate code as well as our new one and the Keil compiler is very
entrenched in this market.

Keil is a non-free compiler that generated quite efficient code but doesn't
really support C standards well.

It used to be that Keil used to be expensive for the user, but that was a
reason people would choose SDCC.

Today that Keil...
A few years ago Keil somehow managed to convince hardware manufacturers to
pay them to make Keil then available at no monetary cost to the programmers,
which of course makes it a bit harder for SDCC to compete here.

Then there's the Padauk devices: The very cheap ones.

It's definitely not an easy target for a C compiler. It's very clear that
whoever designed them did not intend them to be programmed in C but assumed
assembler programmers.

There's ALP. I don't know what the current situation is: That they were very
closely related to the PDK ones.

It's a mainland chinese company. There were a few questions on the mailing
lists, but I guess they were working on some SDCC fork.

They might want to contribute their work upstream, as far as I know.

I think the main risk is probably them not getting their controllers into
mass production and then nothing else will happen, but I'm not sure about
their current state.

Then there's NXP S08.
The SDCC port needs a bit of work, but it's kind of doing okay. It uses the
new register allocator, it has reasonable support for features, even though
a few gaps are here or there, but it's hard to get into the community: It's
a small community of developers. They are all using the non-free compiler
from the hardware manufacturer.

Sometimes they complain a bit when support for older devices gets dropped,
but I haven't seen a lot of them move to SDCC.

The eZ80: A Z80 derivative that's still alive and still made by Zilog
themselves.

Again, it's probably hard to get into developer community and, apart from a
non-free compiler, there is also an LLVM port supporting them, though.
We'll get to the details of that in the last talk today.

Also Toshiba: The TLCS, another Z80 derivative, not  yet supported by SDCC.
Again, hard to get into the developer community.

And then there's the f8, which is a soft core that I designed and that works
on a few FPGAs now, and we have a port for it.

There the problem is how to get people to use the f8, but we do have support
for f8 and we are also working on the f8l which is a simplified variant
about only half the size on the die, it dropped some instruction such as the
multiplication for that but it's is still a quite memory efficient
architecture both in terms of using data memory and of code size.

Well yeah, that's for the introduction for SDCC and the current challenges.
We can discuss this before we get to the next talk.

**Michael:**

So is anyone working on like current support for 16 bit architectures like
the Z8000 or TMS9900 or similar or MSP430 for example, which would be even
more common?

**Philipp:**

Not really.
I mean: Many of these so called 8 bit architectures are actually mixed 8/16
or even sometimes a bit mixed 8/16/32 architectures there.

However, we currently don't support like a lot of pure 16 bit architectures.
No. We don't support any pure 16 bit architectures now.

As far as I know they are actually not that common because a lot of people
move directly to 32 bit like ARM and then there's the other aspect:

At least for me one of the main motivations is to have a free toolchain.
So I typically look into architectures that are not yet supported by another
free compiler.

So if there's already a GCC or LLVM port, at least I personally don't put a
lot of effort into doing anything in SDCC.

**Michael:**

Sure.

**Philipp:**

The eZ80 is a bit of an exception, because as we'll to the last, there's
different design goals of certain use cases SDCC versus LLVM might make more
sense and then the 6502.

Well, I didn't write the port but that's also something that is also
supported by an LLVM port now, so I think the SDCC port was first there.

**Michael:**

For the 6502: Did you compare against the cc65 compiler?


**Philipp:**

I didn't compare. I think some others did comparisons of an earlier version
of this SDCC port but yeah. I mean that port is done by Gabriele who
unfortunately is not here, today.

**Michael:**

The 6502 is not very friendly for a C compiler anyways, I guess.

**Philipp:**

I wouldn't say that. It's probably as friendly as an 8051 or PDK, maybe a
little bit friendlier than PDK but not as friendly as an STM8 or Z80. Sure.

**Michael:**

Yeah, sure.

**Philipp:**

But some of these architectures that we support are worse than the 6502.

**Michael:**

So does the 6502 backend -- while you're not working on it -- does it use
the hardware stack on the 6502 which is -- like you know -- constrained to
256 bytes, or ...

**Philipp:**

I think it does.

**Michael:**

It otherwise will be very expensive to emulate that.

**Philipp:**

As far as I know it does, but I'd have to look into the details.

**Michael:**

Just programmed 6502s for too many decades and ...

**Philipp:**

Well, now you can program it with SDCC.

**Michael:**

Right. Thank you.

**N.N. 1:**

Would you like to go back to slide number four and can you please elaborate
more on the problem of more registers and the problem there was? Something?

+--------------------------------------------------------------------------+
| Optimal Register Allocation in Polynomial Time                           |
+==========================================================================+
| * Register allocator based on graph-structure theory                     |
| * Optimal register allocation in polynomial time                         |
| * Flexible through use of cost function                                  |
| * Provides substantial improvements in code quality                      |
| * Slow compilation for targets with many registers                       |
| * Compilation speed / code quality trade-off:                            |
| * ``--max-allocs-per-node``                                              |
+--------------------------------------------------------------------------+

**Philipp:**

Ah! You mean the register allocator here?

**N.N. 1:**

Yeah.

**Philipp:**

Okay.

**N.N. 1:**

Yeah, this is a point -- the second last point is the combination, is ...

**Philipp:**

Yeah. "Slow compilation for targets with many registers" Okay.

**N.N. 1:**

Where is the problem?

**Philipp:**

The problem is, well, the algorithm we have that can do these things in
polynomial time, which is already great, is still exponential in the number
of registers.

**N.N. 1:**

Mhm.

**Philipp:**

So if we have many registers, things would get very slow.
We have the heuristic that we can use then, but then we are no longer
provably optimal.

**N.N. 1:**

Mhm.

**Philipp:**

So this register allocator / annotator that uses the tree decomposition of
the control flow graph... let me see, let me see...

The Z80 is the one architecture with the most registers that we are
targeting and from the perspective of the compiler the Z80 has the
accumulator A, then the B, C, D, E, H, L and eighth: IY register, so we are
at nine: Nine 8 bit registers that we are using and that's the most we've
currently tried.

**N.N. 1:**

Mhm.

**Philipp:**

When we use this register allocator for the 8051, we will have to see how
well we can deal with a larger number of registers.

And the intermediate point -- in the last talk I mentioned it -- the Rabbit
4000 -- we'll try to get to eleven registers, which is one of the reasons
why I want to do the Rabbit 4000 port before I redo the 8051 port.

**N.N. 1:**

Mhm.

**Philipp:**

On the other hand it might be possible to speed it up using the knowledge
that certain registers are interchangeable.

Yeah. For the Z80, there are no two registers that are fully
interchangeable, unless you count IX versus IY, but then we use IX as a
frame pointer, anyway, and so it is not available to the register allocator,
so all of those nine registers that we have are different.

**N.N. 1:**

Mhm.

**Philipp:**

So we can't just say: "Okay. They are interchangeable!" and reduce the
number of intermediate results, because A is 8 bit accumulator, HL can be
used as a 16 bit accumulator -- it has its halves H and L -- BC and DE are
nearly the same, but then the I/O operations are only for BC.

And then there's "exchange DE with HL" operation there that's only available
for DE.

And then we have a few block move instructions where BC and DE have
different roles, and IY is an index register which is totally different
again, anyway, so yeah.

While on the 8051 we do have register R0 and R1 which are totally identical,
[i.e.] could be exchanged and then R2 to R7 are also the same.

So if we can use that somehow to reduce the number of intermediate results,
that might help, but in general we do not have a better bound than this
exponential one.

It might exist, because we have lower bounds also for the complexity of this
register allocation problem, and it might be possible that you get something
like square root of the number of registers in a better algorithm.

So there is still a gap in the theoretical bounds for the problem.

**N.N. 1:**

Thank you.

**Isolde (host):**

More questions?
Comments?

**Philipp:**

Okay. Well I guess then we can go to the talk about the treedec library
which SDCC uses to obtain those tree decompositions that we use for example
in the register allocator.

There's other optimizations using tree decomposition in SDCC but the
register allocator is definitely the most important one.

+--------------------------------------------------------------------------+
| SDCC vs. non-free compilers: STM8 Code size                              |
+==========================================================================+
| .. image:: sizes-2024.png                                                |
+--------------------------------------------------------------------------+

That definitely gives us an edge over other compilers in relevant use cases,
namely compiling nearly every program.

The other tree decomposition based algorithms are either in placement of
bank selection instructions, which does only apply to a small subset of
programs, or it's in redundancy elimination, where it allows us to do
something in linear time that would otherwise be qubic time, but it's still
polynomial versus polynomial, but register allocation is where it really
makes a difference.

Okay.

+--------------------------------------------------------------------------+
| Ports                                                                    |
+==========================================================================+
| * MCS-51, DS390, STM8, f8, HC08, S08, PDK13, PDK14,                      |
|   PDK15 (PIC14, PIC16)                                                   |
| * MOS 6502, WDC 65C02                                                    |
| * Z80, Z80N, Z180, eZ80, TLCS-90, SM83, Rabbits, R800                    |
+--------------------------------------------------------------------------+

**Isolde (host):**

I was actually wondering: So SDCC is the only free compiler?

**Philipp:**

For STM8?

+--------------------------------------------------------------------------+
| SDCC vs. non-free compilers: STM8 Code size                              |
+==========================================================================+
| .. image:: sizes-2024.png                                                |
+--------------------------------------------------------------------------+

**Isolde (host):**

Yeah. Because here you are comparing it...

**Philipp:**

Yeah. For STM8, SDCC is the only free compiler targeting this architecture.

**N.N. 2:**

Free compiler means open source or what do you mean?

**Philipp:**

Well, okay, so it's a bit of a mixture, but nearly everything is GPL3
compatible at least.

There's a few test cases that are under MPL, and in principle I think it is
still possible to get a compiler that's GPL2, but then you only have
Thorup's heuristics for getting tree decompositions, not the methods from
the treedec library, and you still have a GPL3 preprocessor and assembler
and linker.

**Isolde (host):**

So I was wondering:
Would the industry generally be interested in using this, because, you know,
if you have to sort of maintain your own or buy a non-free compiler et
cetera, do you see interest?

**Philipp:**

Well, for the STM8 I think nearly no-one is using Raisonance anymore.
IAR is also only a very niche one these days. Basically the only people
using IAR are those who need some of the certifications that are required
for certain medical or other safety critical things, because SDCC and Cosmic
don't have those certifications.

So Raisonance and IAR are rarely used these days. It's nearly only Cosmic
and SDCC and I guess that it's hard to know how much they are used.

Of course I can check GitHub and see: "Oh there's things for the STM8 here
and which compilers do they support?", but I guess there's a lot of internal
code in companies that you don't see and you don't know which compilers they
support, but if you look at GitHub projects only, then it's definitely SDCC
probably at a bit more than half and Cosmic at a bit less than half and IAR
and the Raisonance are rarely supported.

**N.N. 2:**

Are there any certifications that this SDCC currently supports that can
further use downstream for safety critical systems or something?

**Philipp:**

No. I am not aware of anyone ever trying to certify SDCC for these things.
I mean even for GCC I think it once in a while happens that a certain GCC
version was certified, but then there was this niche application of this
certified thing.

Everyone else just uses the latest GCC which then does not have this
certification, but yeah.

Okay.

**Isolde (host):**

So, yeah.

There are no more questions.

Let's thank Philipp!
