{fourth talk and discussion, using slide set "Rabbits.pdf"}

+--------------------------------------------------------------------------+
|                                 Rabbits                                  |
|                                                                          |
|                      and the rest of the Z80 family                      |
+==========================================================================+
|                           Philipp Klaus Krause                           |
|                                                                          |
|                                2025-04-11                                |
+--------------------------------------------------------------------------+

Okay.

The Z80 family and the family of ports for the Z80 family in SDCC will be
the last talk topic for today.

+--------------------------------------------------------------------------+
| Z80 family                                                               |
+==========================================================================+
| * Zilog Z80 - derived from Intel 8080                                    |
| * Z80N - ZX Spectrum Next softcore                                       |
| * Sharp SM83                                                             |
| * ASCII R800                                                             |
| * Zilog Z180                                                             |
| * Zilog Z380                                                             |
| * Zilog eZ80                                                             |
| * Rabbit 2000 (further derivatives: Rabbit family)                       |
| * Toshiba TLCS-90 (further derivatives: TLCS-870 and                     |
|   TLCS-900 families)                                                     |
| * NEC 78K (GCC)                                                          |
| * Renesas RL78 (GCC)                                                     |
+--------------------------------------------------------------------------+

I start with a quick introduction to the most important members of the Z80
family.

Of course the Z80 itself. It's derived from the Intel 8080.

You might remember. There was this guy that invented microprocessors or
something. I know. He invented the MOS semiconductor manufacturing process
at Fairchild and then was frustrated with Fairchild not caring enough about
semiconductors, so he went to Intel, where he designed microprocessors.

I think he did the first 8 bit processor at Intel, then was frustrated by
Intel not caring about microprocessors, really, so he left, then founded
Zilog to have the last word on integrated logic devices, which is where the
"Z" comes from, because it is the last letter of the alphabet. And the first
thing that Zilog did was the Z80, an 8080 compatible microprocessor.

Now, a very recent one, a softcore one in the ZX Spectrum Next is the Z80N.
It's basically a Z80, cycle and bug-for-bug compatible with the original
Z80, but with a few extensions. Not meant for compilers, unfortunately. They
didn't listen to any advice for what would be useful in a compiler, but at
least their multiplication instruction is useful to use in SDCC, so we have
a port for that, as well.

Sharp, a Japanese company, made the SM83.

Once popular in Japanese appliances, it's a quite simplified Z80. Sometimes
it feels like it's closer to the 8080 than the actual Z80. Throughout the
world it's mostly known as the processor core used in the Gameboy.

Now, Zilog itself didn't stop at the Z80. I mean: They thought that by 1980,
8 bit things would be obsolete and that they should move on to 16 bits and
they tried to introduce a large number of devices at various times. Some
successful, some not. Some derived from the Z80, some not.

One of their projects for a 16 bit architecture was the Z800. Now the Z800
never made it beyond prototype stage and never got into mass production, but
it heavily inspired the later Z280, which made it into mass production but
wasn't a commercial success, which is why neither the Z800 nor the Z280 is
on this slide, because they are not important enough as targets.

However, there was a Japanese publisher, and by publisher I mean really ink
and paper publisher, that got into computing devices, as they already were
in the computing area, maybe like the Heise publisher in Germany, and at
some point they made their own processor for some computers. And this was
the ASCII R800, which is heavily inspired by the Z800 and it's instruction
set is actually a subset of the Z280. This was definitely mass produced and
was important in Japan, so yes, we support that one in SDCC.

Now, something that was very successful from Zilog: The Z180. It's basically
a Z80 with some peripherals moved into the processor, with a memory
management unit added. It has different timings, it's faster and it added a
few instruction, in particular an eight times eight to sixteen bit
multiplication and some instructions for bitwise tests. We also support that
one in SDCC.

The Zilog Z380 was definitely more successful than the Z280 and it was
actually a 32 bit architecture. This is the first processor where Monte
Dalrymple did the processor design, who was previously at Zilog, but only
worked on peripherals before, but this is the first processor -- the first
and I think also the last processor -- he designed at Zilog. But we'll get
back to him later. As a 32 bit architecture it's not supported by SDCC.

It, well, it wasn't a complete commercial failure like the Z280, but it
wasn't really that successful, either. As far as I know, it's found in some
telecommunications equipment and I have no idea if that equipment is still
in use.

Much more successful was the Zilog eZ80. This is kind of a mixed 8/16/24 bit
architecture. It has some odd aspects, some of them similar to the Z380, in
that it has some mode bits. So you can switch it between an 8/16 bit and a
24 bit mode, and there's 24 bit registers and if you do anything in 16 bit
mode, then the upper 8 bits of that register become unspecified.

So it's faster in 8/16 bit mode, but in 24 bit mode you get 24 bit
computations and can naturally work with 24 bit addresses. The Zilog eZ80 is
currently supported by an SDCC port, but we only use the 8/16 mode and it's
supported by an LLVM port that only uses the 24 bit mode.

Okay, so a lot of companies were based on Zilog products and at some point
and some point they tended to sometimes also make their own Z80 derivatives.
I mean: A lot of Z80 derivatives were made throughout time. Zilog made about
two billion Z80s in total, but I guess about the same number were again made
by people making Z80 clones, be it Toshiba, or be it the Volkseigener
Betrieb Mikroelektronik "Karl Marx".

And the Z180 actually was not originally developed by Zilog, but by Hitachi,
so in this case Zilog even went back to include extensions from a different
company. Now, one company that got frustrated with Zilog introducing new
products and then discontinuing them, also decided to just start making
their own processors. I think they were called Z-World, because they were
making products based on Zilog devices. They then turned themselves into
Rabbit Semiconductors and hired Monte Dalrymple who designed the Zilog Z380
to design microprocessors for them, starting with the Rabbit 2000, which was
the first device in the Rabbit family. Important enough that we'll get to it
later, and yes, SDCC has some support for that.

Toshiba, after making Z80 clones, also started making derivatives that are
even more different from the original than the Rabbits are. Starting with
the TLCS-90 and then following up with the TLCS-870 and TLCS-900 families,
the latter being a 32 bit device.

NEC and Renesas also make derivatives of the Z80 architecture, similarly to
the Rabbit, the Toshiba devices and the eZ80, I guess, and the Z380, they
are quite a bit more powerful than the original Z80. However, for the 78K
and the RL78 there's GCC support, so I didn't really bother looking into
doing anything for them in SDCC, because there's already a free compiler
supporting them.

+--------------------------------------------------------------------------+
| Z80 family TODO                                                          |
+==========================================================================+
| * Better handling of global variables                                    |
| * Pointers into __sfr named address space for I/O                        |
+--------------------------------------------------------------------------+

Okay. So what do we want to do for the Z80 family -- the whole Z80 family in
SDCC?

Basically, we should handle global variables better. Currently, we basically
use register IY, point it to it, if there is a register IY. If ther is no
register IY, we use HL and pointed to it and very rarely do we try to access
it directly.

Which method of the three is actually the best and most efficient depends on
a lot of stuff and the very crude mechanism that we have today often does
not give us the best result, so there's definitely some work to access
global variables in the Z80-related ports more efficiently and make smarter
decisions. [I.e.] Should we use a pointer register to point to it? HL versus
IY or should we try to use a direct addressing mode? All of them have their
advantages, yeah.

Direct addressing mode is the fastest if you need only a byte that you put
into register A or you need a two-byte value that goes into a register pair.
The others are more efficient if you need to access more bytes following
that location and you might want to store it, use it as an operand or might
want to store it into an individual 8 bit register. That is: Other than A.

The other aspect is that the Z80 and most of the Z80 derivatives, but not
all of them, do not have memory-mapped I/O in the traditional sense.
Instead, they have a separate I/O address space accessed with separate
instructions. Technically, the architecture would allow to have pointers
into that space. It's not currently supported by SDCC, though, yeah?

We have the __sfr keyword for I/O, but we currently do not support pointers
into this. __sfr comes from special function register, because for some
microcontrollers, I/O locations are called special function registers, most
famously the 8051 and actually even some Z80 derivatives use that name.
Though most of them say just "I/O locations".

+--------------------------------------------------------------------------+
| eZ80                                                                     |
+==========================================================================+
| * Z180 derivative                                                        |
| * Additional instructions, registers extended to 24 bits                 |
| * Z80 mode: registers handled as 16 bits, ADL mode: registers            |
|   handled as 24 bit; separate stacks                                     |
| * Mode can be switched persistently, but also for individual             |
| * instructions with prefix byte                                          |
| * LLVM port uses ADL mode                                                |
| * SDCC port uses Z80 mode (more efficient when handling                  |
|   8/16 bit data)                                                         |
| * SDCC port limited to 16 bit address space                              |
| * TODO: __far intrinsic named address space to allow use of              |
|   24-bit addresses for data using ADL mode prefix                        |
| * Mixing use of 16-bit and 24-bit register accesses complicates          |
|   interrupt handling                                                     |
+--------------------------------------------------------------------------+

Okay. So let's get to the eZ80.

They're listed as a Z80 derivative. Actually, it's more of a derivative of
the Z180. So the instruction set is a superset of the Z180, however the
peripherals of the eZ80 are not a superset of the Z180.

As I said, additional instructions, registers extended to 24 bits and two
modes: Z80 mode where things are 16 bits and the ADL mode where we have
registers handled as 24 bits.

One thing that makes things more complicated is that there's two separate
stacks. So instead of the stack pointer being 16 bit and then being extended
to 24, now there's a 16 bit stack pointer and a separate 24 bit stack
pointer. So if you switch between the modes, you can't just push a 16 bit
value on the stack and pop a 24 bit value from the stack, because the stack
you're pushing to is a different stack than you're popping from.

So that makes it very hard to transfer data between parts that use the 16
bit mode and parts that use the 24 bit mode. It's possible to switch it
persistently, but there's also a prefix byte to switch it just for
individual instructions.

Well, the LLVM port as I said obviously uses ADL mode. We currently use Z80
mode and we're currently limited to a 16 bit address space. So the idea is:
We introduce an intrinsic named address space "__far" to allow the use of 24
bit addresses by using the ADL mode prefix on individual load and store
instructions dealing with those "__far" address spaces, because that now
allows us to still have the advantages of the Z80 mode, namely very
efficient handling of 8 and 16 bit data.

Sure, we could do it like LLVM and just use the ADL mode, but then any 16
bit operation would really be a 24 bit operation with upper 8 bits that we
don't really need and would take an extra cycle or two, which we would loose
efficiency for, and then we would be in the same space as LLVM, which is no
point competing with, anyway.

So the idea is: We keep our advantages of having the Z80 mode where we are
fast and we can handle 8 and 16 bit quantities efficiently, but, to allow
people to use a bit more memory, we have this additional named address space
for stuff that people explicitly want to place outside the lower 64k.

The main complication is in interrupt handling, because now we need to use
both stacks, because the standard stack for the 16 bit mode is still there
for nearly everything, but, if there is an interrupt, we need to store all
24 for bits of the registers, because we use individual instructions with
ADL mode prefix where the upper 8 bits of the 24 bit register matter, so
that needs to be correctly saved and restored from the stack.

So the interrupt handler needs to store everything onto the ADL mode stack
and restore from there, so that means in the startup code we need to have
two stacks, we need to estimate an upper bound on how much space we use on
the ADL mode stack, initialize the stack pointer efficiently, make sure we
don't exceed that, especially if there's multiple interrupt priorities.

So the interrupt nesting depth needs to be taken into account, but in
general this would make it possible to use more memory for data in the eZ80
port.

+--------------------------------------------------------------------------+
| Rabbit family                                                            |
+==========================================================================+
| * Z80 derivative, inspired by Z180                                       |
| * Rabbit 2000: additional instructions                                   |
| * Rabbit 3000A: additional instructions                                  |
| * Rabbit 4000: additional instructions and registers                     |
| * Rabbit 6000: additional instructions                                   |
| * From Rabbit 4000: 4 modes: Default (better 8-bit support),             |
|   Enhanced (better 16/32-bit support), 2 undocumented                    |
| * SDCC ports for Rabbit 2000, Rabbit 2000A, Rabbit 3000A                 |
| * SDCC ports limited to 16 bit address space                             |
+--------------------------------------------------------------------------+

Okay. Let's get to the Rabbit family.

So this one is inspired by the Z180. It's not fully binary compatible with
the Z80. The Rabbit 2000 already has additional instructions. It does not
have the Z180 multiplication or test instructions. It has a different
multiplication instruction that's 16 times 16 to 32 bits, which is more
powerful if that's what you want to do, but if you just want to do 8 times 8
to 16 bits multiplication, it's annoying that it overwrites all your
registers for a 32 bit result.

It was followed by the Rabbit 3000A with a few additional instructions and
the original Rabbit 3000 has a lot of different peripherals. Not relevant
for a compiler.

Then came the Rabbit 4000 where we are getting to some 32 bit stuff. They're
adding a few 32 bit registers and again more instructions.

The Rabbit 5000 adds more peripherals: I think it now has Wifi on the chip.
So, long before any ESP8266 devices, the Rabbits were the first
microcontrollers with Wifi and, already earlier, Ethernet on the chip. But
from a compiler perspective, the Rabbit 5000 only changed the instruction
timing to allow higher clock speeds and adds a few additional bugs.

And then came the Rabbit 6000 with, well, from the compiler perspective,
more instructions, from the whole system perspective, lots more peripherals.

Now, starting with the Rabbit 4000, there's multiple modes. Unlike the eZ80
or the Z380, the mode doesn't just affect how much data an instruction
handles, but it totally changes the opcode map.

The default mode is basically what was there: The nearly Z80 compatible one,
but definitely the most compatible from the Rabbit 2000 onwards.

And the enhanced mode where some of the 8 and 16 bit instructins are getting
longer and 32 bit instructions are getting shorter. For the 16 bit
instructions it's kind of both: Some are getting longer, some are getting
shorter. 32 bit instructions are definitely getting shorter in the enhanced
mode, 8 bit instructions shorter in default mode, so depending on what kind
of data you are handling, one of the two modes would be more efficient.

And these are just the two documented modes. There's also two undocumented
modes that are kind of mixtures of these two.

Yeah.

We currently have SDCC ports for the Rabbit 2000, the 2000A and the 3000A.
The 2000A is basically just a newer revision of the Rabbit 2000 that fixes
enough bugs that we can finally make proper use of block move instructions
that were just too broken in the original Rabbit 2000s.

And, currently, our ports are again limited to a 16 bit address space, like
the eZ80. There's not really a hard limit, I mean: People can use bank
switching, manually, or they can use the non-intrinsic named address spaces
for memory pages, but it's not a very comfortable thing to do.

+--------------------------------------------------------------------------+
| Rabbit family TODO                                                       |
+==========================================================================+
| * SDCC ports for Rabbit 4000, Rabbit 5000, Rabbit 6000                   |
|   (assembler, compiler, simulator) using one of the                      |
|   undocumented modes                                                     |
| * Use JK register pair - first time new register allocator is used       |
|   for more than 9 registers.                                             |
| * No plans to support the PW, PX, PY, PZ 32-bit registers                |
| * __far intrinsic named address space to allow use of 20-bit             |
|   addresses                                                              |
| * medium and large memory models to allow use of 20/24 bit               |
|   and 24/32 bit addresses for code                                       |
| * Better flash tool and on-target debugging                              |
+--------------------------------------------------------------------------+

So, we have a quite a bit / much bigger TODO for the Rabbit family.

The idea is to introduce ports for the Rabbit 4000, 5000 and 6000, which
means work in the assembler, compiler and simulator. One of the two
undocumented modes probably makes the most sense, here, because we don't
want to loose the efficiency in handling 8 bit and 16 bit valies, yeah?

SDCC is a compiler originally for 8 bit architectures. We have a lot of
optimizations already in the compiler to narrow down variables to save
space, so being able to handle narrow variables, efficiently, is an
important aspect and we don't want to loose that, here.

So, we do not want to switch to enhanced mode just to be able to do some 32
bit stuff faster if we are loosing too much on the 8 bit side.

The JK register pair would be used -- from the Rabbit 4000, I think --,
which means that we are pushing our new register allocator further than
eight registers, for the first time, namely we are getting to eleven, now.

As I said before, that is probably a good preparation for some future work
on the 8051 port, where it would be even more.

I currently do not have any plans to use the 32 bit registers. They would be
very useful if you want to have a flat 32 bit address space. They would be
somewhat useful for a "__far" that uses a 32 bit address space, but of
course the drawback is: Additional registers means higher interrupt
overhead, because we need to save and restore them on interrupts, and at
this point I don't think that it's worth the tradeoff.

Again, we should use the "__far" intrinsic named address space to allow more
than 16 bit addresses to be accessed, conveniently. In this case, that's a
20 bit address space, as already available in the original Rabbit 2000. From
the Rabbit 4000 on, this is actually a physical 24 bit address space.

And, apart from using additional memory for data, I also want to use
additional memory for code. That's something we're actually doing already on
the STM8: On the STM8, we're using a 16 bit address space for data, but,
depending on the compiler options, we actually have function pointers that
are 24 bits, and basically, the idea is to do the same now for the Rabbits
by using the appropriate jump and call instructions. It gets a bit more
complicated for the Rabbits, because, unlike the STM8, where you just access
a flat address space with these long calls and long jumps, here for Rabbit
we have to map bits into an 8 bit window and a 16 bit thing and we just
update the mapping automatically in some cases. So this results in
complications for the assembler and linker that need to be taken care of.

And another problem with the Rabbits is the flash tool and on-target
debugging. I mean: There's the OpenRabbit tool, which mostly works, but
basically only really for flashing. For on-target debugging we don't have
something like that, yet.

I mean: For STM8, there's OpenOCD which you can use. And then you can emit
ELF/DWARF debug info from SDCC and actually can debug on-target with GDB,
which is very comfortable, because you can debug, like, [incomprehensible]
on the host system.

+--------------------------------------------------------------------------+
| TLCS family                                                              |
+==========================================================================+
| * TLCS-90: Z80 with additional instructions, different opcode            |
|   map                                                                    |
| * TLCS-870: TLCS-90 with additional and removed                          |
|   instructions, unknown opcode map                                       |
| * TLCS-870/X: TLCS-870 with additional instructions,                     |
|   unknown opcode map                                                     |
| * TLCS-870/C: TLCS-870 with additional instructions, different           |
|   opcode map                                                             |
| * TLCS-870/C1 and TLCS-870/C1E: TLCS-870/C with                          |
|   additional instruction, different timing                               |
| * TLCS-900 family: 32-bit registers                                      |
| * SDCC port for TLCS-90                                                  |
| * SDCC port limited to 16 bit address space                              |
| * Lack of documentation                                                  |
+--------------------------------------------------------------------------+

Yeah.

The last one then of these families I wanted to mention is the TLCS family.

We do support the TLCS-90: A Z80 with additional instructions and different
opcocde map.

And there's some successors, in particular the TLCS-870: Some additional
instructions, some removed ones. I don't know the opcode map, yet, but I'll
see what I can find out.

Then Toshiba did another variant of this, called /C1: I don't know if the C
stands for "compact" or the C programming language, but the basic idea is to
have reduced code size for C programs. They only did one extended version,
which is somewhat more powerful, and then out of the C version there were
other additions: They added one more instruction and changed the instruction
timing.

And there's the TLCS-900 family which is the 32 bit variant of the whole
thing.

We have a port for the TLCS-90. It's limited to 16 bit address space. There
is some mechanism for kind-of-far-pointer-like stuff in the TLCS-90, but
it's not as important as for the eZ80 and the Rabbits, because most of the
devices actually never used that much memory.

There's a bit of a lack of documentation. I mean: It's not as horrible as
some architectures, but it's still that Toshiba doesn't document as well as
Zilog or Rabbit. I mean: For the Rabbit we also have a lack of documentation
for the Rabbit 6000 in particular, but the earlier ones, especially up to
the Rabbit 4000, are very well documented.

+--------------------------------------------------------------------------+
| TLCS family TODO                                                         |
+==========================================================================+
| * SDCC ports for TLCS-870, TLCS-870/C, TLCS-870/C1                       |
| * Handle different code size in cost function                            |
| * __far intrinsic named address space to allow use of 20-bit             |
|   addresses for TLCS-90                                                  |
| * No plans to support TLCS-870/X, TLCS-900 family                        |
| * Free flash tool                                                        |
+--------------------------------------------------------------------------+

Yeah, so what to do about the TLCS family?

Well, my idea is to have SDCC ports for th e TLCS-870, TLCS-870/C and
TLCS-870/C1. The relevant one is the one on the right, because that one is
still a current microcontroller family. The other ones are mostly there,
because it should be relatively cheap to support them, because they are kind
of in between the existing TLCS-90 port and the TLCS-870/C1 port that we
want, anyway.

Again, there would be an idea to have a "__far" intrinsic named address
space for 20 bit addresses for the TLCS-90. So again, it's not as important
as for the others. You know, the TLCS-90 is not as important as the eZ80 or
the Rabbits, because it's not used much today. I mean: There's not a lot of
users of the TLCS-90 port.

I don't think there's much point to do TLCS-870/X or TLCS-900 support,
because they're no longer in production, anyway, and they are substantially
different from the rest, so that it would be substantial extra effort to
support it.

A free flash tool would be nice, but I don't know how to do that.

+--------------------------------------------------------------------------+
| Summary                                                                  |
+==========================================================================+
| * Additional ports to better support Rabbit 4000, Rabbit 5000,           |
|   Rabbit 6000, TLCS-870, TLCS-870/C, TLCS-870/C1                         |
|   (assembler, compiler, simulator)                                       |
| * __far for eZ80 and Rabbits                                             |
| * Larger address space for code for Rabbits                              |
| * Various smaller improvements                                           |
| * Work on eZ80 and Rabbit this year (i.e. in time for SDCC               |
|   4.6.0), TLCS-870 family later                                          |
+--------------------------------------------------------------------------+

So, let's get to the summary.

The plan for the Z80 family is to have additional ports for the Rabbit 4000,
5000, 6000, TLCS-870, 870/C, 870/C1. That requires work in the assember,
compiler and simulator.

The "__far" named address space for the eZ80 and Rabbits and TLCS-90: That
one, that's important.

A larger address space for code for the Rabbits: That one is very important,
because currently, we can only use them as essentially faster Z80s with
additional instructions. We can't use the big upper space, yet.

The few smaller improvements that I mentioned throughout.

And my plan is to get the eZ80 and Rabbit work done this year. Let's see
about the Toshiba TLCS-870 stuff. That might take longer.

Well, I think that's it for now.

**Isolde (host):**

Thank you very much!

Any questions?

Comments?

Volunteers?

**Philipp:**

Volunteers to implement some of this stuff, of course.

I mean: The Rabbits are really interesting devices. Even the other Z80.

I mean: The Rabbits are like the ESP8266 or the ESP32, but they were much
earlier.

And in early IoT devices or even if you have, like, a Voice-over-IP phone,
it's likely that there's a Rabbit in there. From the -- I think -- Rabbit
4000 onward, they had Ethernet integrated, from the 5000 (at least) they had
Wifi. I don't know if with Rabbit 5000 or 6000, they also had Fast Ethernet.

And of course: Once the compiler work is all there, then one could try to
reproduce the whole development system in free software. Like: Port real
time operating systems, port a TCP/IP stack, do all the network stuff on
them.

Yes. Unfortunately, the Rabbits also will soon no longer be produced.

I made the plans to do all the work, they are still current, but there was a
notice from Digi, which bought Rabbit Semiconuctor, that:

Last order: January this year, last deliveries: end of this year.

In principle, someone else could start producing Rabbit compatibles, one
day, but it seems unfortunately unlikely, at this point.

Any questions?

Well, I guess otherwise, this is the part to end the presentation part and
the rest will be mostly discussions among SDCC developers on what else to do
and how to do the things we want to do and when to do them, and how to maybe
get some funding for it.

{the general discussion will pick up right here}
