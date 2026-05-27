{picks up right after the discussion that followed the fourth talk}

**Isolde (host):**

So, can you talk a bit about funding, so far?

**Philipp:**

Okay. Funding, so far...

Well, there were various funding sources over time.

I mean: The earliest was definitely DFG funded projects, where some compiler
work was explicitly included.

For the standardization -- not implementation of standards in SDCC, but
working in the standards committee -- we sometimes got European Union
funding, StandICT.

There was a Prototype Fund project, that heavily focused on C23 support.
Originally, C23 support and usability, but we kind of never got around to
doing any of the usability stuff, so we focused on improving C23 support,
mostly.

And then we've had a project by NLnet as part of one of the NGI0 programs, I
don't remember which one, to improve SDCC.

Then we had an NGI0 program from NLnet to work on the f8, the softcore
architecture I mentioned that can work on some FPGA's, now, that I actually
have, here. I also have Rabbits, but that is a different topic. And that
included work on supporting this architecture in SDCC, so the f8 port.

And we currently have ongoing applications for StandICT, again. StrandICT is
not big money: It's basically the travel money to get to the meetings and
the membership fee for DIN, to be allowed to attend the meeting.

And we have an application with NLnet, again, for the NGI0 Commons, which
includes some of the work that was presented by Benedikt and me, today, as
things we want to do and a few other things we want to do, that, well, we
want to do, but didn't want to make a whole talk out of: Mostly in
optimizations and link-time optimizations and such.

And foundational work in the assembler and linker that needs to be done for
this year, as well as some more f8 work, as well.

And of course, that is the funding we got, of course there is also plenty of
funding that we applied for and got rejected, both at Prototype Fund and for
example at Sovereign Technology Fund, but well, even at StandICT, I mean: We
had one successful project -- we had one or two successful ones -- and one
that got rejected, but yeah.

I mean, these funding things are -- they all have their advantages and
disadvantages, but one thing that seems to be true for all of them is that
there is usually a lot of demand and it's quite competitive.

And they especially they often tend to have a two round review process and
sometimes the second one with external reviewers for the Prototype Fund and
Sovereign Technology Fund seems to be quite tough depending on what reviewer
you get.

Then those reviewers often have their own ideas about how things should be,
and if you're unlucky, like it happened with our Sovereign Technology Fund
application, our first application, you get one of these people who really
don't like C, who say: "C is an unsafe language! C is part of the problem,
not the solution. We should defund C compilers!" and unfortunately, for
Sovereign Technology Fund, once you've got rejected once, you can't easily
reapply.

Unlike the Prototype Fund, where it's actually encouraged that when you got
rejected, that you work on your application and reapply next year. But with
the Sovereign Technology Fund, when you're out, you're out, so yeah.

And then of course, there's differences in what they fund: Often maintenance
work and bug fixing is hard to get funding for. Many of the funding agencies
want shiny new features and milestones, where you can say: "Okay, we did
this shiny new feature. That's what we were able to do with your funding."

The Sovereign Technology Fund would have been nice in that respect, because
they also explicitly fund maintenance, but it didn't work out for us.

**Michael:**

So, you need to do "sdrustc" to get funding? {laughs}

**Philipp:**

Well, if I had known that we get such a reviewer, of course we could have
prepared better. We could have said: "Well, we support newer C standards
than commercial compilers. Those newer C standards do have mechanisms for
safety and security. And people are actually using the C backend of LLVM to
compile Rust code and then using that as input to SDCC, so we need to
improve our support for static inline functions to be able to handle them
more efficiently, because the C backend of LLVM uses a lot of static inline
function and our current handling of this is a bit inefficient in terms of
code size" and stuff like that, but you have no idea what type of external
reviewer you get, so do you want to focus on something like this or do you
want to focus on those who care more about the environment and you want to
emphasize how the small architectures save energy and resources, like: "You
have a compiler for 8 bit systems. That makes it more viable to use 8 bit
microcontrollers, which reduces resource and energy usage" and these things,
and then you have your 500 word limit, where you can make your pitch for
this question and... yeah.

**Michael:**

Would you think it still makes sense for new developments to go for an 8 bit
controller?

**Philipp:**

Excuse me?

**Michael:**

Would you think -- if somebody would develop a new project -- do you think
it would it still make sense for people, so, ...

**Philipp:**

Definitely!

**Michael:**

... except for legacy reasons, to

**Philipp:**

No, there's plenty of use cases for 8 bit devices, still. I mean: These
Padauk people from Taiwan with their very simple, very cheap ones, I mean...

**Michael:**

But you can get a RISC-V microcontroller for 10 Cents.

**Philipp:**

Exactly! That's far too expensive!

**Michael:**

{chuckles} Okay.

**Philipp:**

Yeah. If you can do it with an 8 bit controller that costs you less than one
Cent, you don't want the 10 Cent device.

**Michael:**

Yeah. Sure.

**Philipp:**

And maybe it's also the energy stuff. Or the amount of memory that you can
do with. And there's definitely a lot of...

I mean: Last time I bought a cyclocomputer for my bike, or rather I bought a
few for the whole family, and one extra, so that I could take it apart,
there was a 4 bit microcontroller in there.

But these Padauk microcontrollers, yeah, I mentioned that the cheap ones are
under one Cent, but actually, I looked at their business reports a few years
ago: The average price at which they sell a system-on-a-chip is below one
Cent, and they are making in the three-digit million range, well, not in
profit but in gross income, so we can estimate that they are definitely
making billions of these every year. And there's a lot of other Chinese --
mainland Chinese -- companies that are also doing very low-end 4 and 8 bit
microcontrollers.

I didn't mention them, because many of them are too simple to target in a C
compiler, definitely. I mean, even for Padauk, yeah, at the very lowest-end,
the PDK13, so called because the program memory consists of 13 bit words,
the data momory by the architecture is limited to 64 bytes.

Still has interesting devices, because -- not in the PDK13 but another
subarchitecture -- they have actually systems on a chip where the whole
system has only 60 bytes, but it has 8 cores. So 60 bytes of RAM shared
among 8 cores. We don't have good multi-threading support in SDCC now, and
definitely not for those architectures, because they definitely lack
something that we would want to be able to do C atomics efficiently on them.
So basically, what you can do now is program one core in SDCC and have
assembler programs running on the rest. But there's definitely -- I think --
there's a future for 8 bit architectures which is also the reason why I
developed this f8 8 bit architecture, and I hope that can also find its
niche where you don't need the power of a 32 bit RISC-V or ARM device.

**Michael:**

Well, concerning energy, I would love to see numbers, but I know this is
difficult, because you know, especially if you are handling not 8 bit
quantities, but something else, you have to use separate instructions for
each 8 bits and handle carry bits and all of this stuff and that might be
just more efficient on a 16 or 32 bit architecture, instead.

**Philipp:**

But when you're handling 8 and 16 bit quantities and you always have to
transfer a whole 32 bit around and do a whole 32 bit operation, you're also
loosing some.

**Michael:**

Exactly.

**Philipp:**

And, we are also quite efficient in code size, here. Yeah? I mean: I don't
have it on the screen, but in my earlier talk we had how we're doing in code
size compared to other compilers.

{part of the conversation got lost, here}

And of couse, on aspect you can do when targeting these small systems is:
You can put more effort into optimizing, because, well, it matters a lot on
the small devices and the input program isn't that big, anyway.

**Michael:**

So that's an interesting point, because usually, a C compiler does not
extend optimizations beyond function level or something.

**Philipp:**

Well, we also don't do much.

**Michael:**

Yeah, because, like, because you have a small program because of the
restricted address space, did you try to do some whole program optimization?

**Philipp:**

Not really, I mean, there is only very few, currently, because of the model
of the compiler: Well, it's a single-pass thing, yeah?

**Michael:**

Okay. Yeah. That is unfortunate.

**Philipp:**

So, we basically handle one function at a time. Sure: That function may be
big, because maybe it was written big by the programmer or maybe it got big
because a lot of stuff was inlined.

**Micheal:**

But that may be a funding opportunity for a research project, you know...

**Philipp:**

Our first step as for link-time optimization would be just for elimination
of ... just for code size optimization.
It is in our current NLnet application to do simple link-time -- not in the
compiler but in the linker -- just code size optimization for now.

**Michael:**

Sure.

**Philipp:**

And we do have a few small inter-procedural optimizations in the compiler
for functions in the same translation unit, but it's very little. We're
definitely not at the level of LLVM.

**Michael:**

Yeah. Sure.

**Philipp:**

And we won't get there anytime soon.

**Michael:**

You don't have that many developers, I guess, but some stuff might be
available or interesting. Like: If you have a limited code space, then if
you do full program analysis then you could redefine the optimal functions
to inline or something like that until you're running out of program memory.

**Philipp:**

In principle: Yes. I mean: Even if we don't do full program, but just full
translation unit, it already would be quite some optimization potential.

**Michael:**

Yeah. Beyond function is difficult in C, of course.

**Philipp:**

But for now I think, with the TODOs I presented and the ones we have in the
application, we'll definitely be busy for more than a year, I'd say.

**Michael:**

How many people are actually working on SDCC at the moment?

**Philipp:**

Well, what do we have? We erm...

**Michael:**

{counting people in the room} One, two, three, four...

**Philipp:**

Yeah. Kind of, I mean: Felix left, now, because he had to do something else.
He tends to work on the preprocessor. Then there's Benedikt and Janko and
me. And then there's Maarten and Daniel and Erik and... erm... and Gabriele,
and I think that's mostly it, these days.

**Michael:**

Which is already impressive, to get something going with so many different
target platforms with such a small team.

**Philipp:**

Yeah. Most people of course don't work on SDCC full time, anyway, but just
do a little bit on the side.

**Michael:**

That's one thing I found interesting: -- Sorry, that's a bit off topic, but
you know, there's all this stupid AI talk about: "Why coding? So let AI do
the coding!" And I was wondering the other way around: "What can a single
person, a single experienced programmer, really do?"

Like: There's examples of single people writing a Unix-like kernel and stuff
like that. Or a compiler or stuff. So what's the limit where you can
actually get by with your single, whatever, mental and time capabilities you
have?

That's something I'd like to write for the GI Spektrum or something like
ACMQ or something just for people to think about on the philosophical level,
like: "Will we need something like that in the future or will it really
support us?"

**Philipp:**

I don't know.

**Michael:**

But that's why I like these projects that are manageable in size, because a
single person is still able to understand the complete software here.

**Philipp:**

Uh...

**Michael:**

Like a simple Unix kernel or a simple compiler: It might work, and that's
what I'm using in my teaching, you know: So students develop a simple
preemptive multi-tasking kernel from scratch or a Pascal-like compiler in a
compiler course or something like that.

**Philipp:**

Sure, that's definitely possible. SDCC might have gone beyond that -- if you
look at the whole thing -- by now.

It definitely takes quite some effort to have some idea about all the parts.

**Michael:**

Yeah. But still, it's feasible, I guess, because there's not 20 million
lines of code you have to go through, which takes two lifetimes or
something.

**Philipp:**

Yeah.

**Michael:**

Yeah. Nice!

Interesting. Thank you!

Sorry for off-topic.

**Isolde (host):**

Any more comments?

So they're still in for [incomprehensible] parts in terms to speak
afterwards or also tomorrow.

Maybe we should... Do you want to start around nine, tomorrow?

Or should we...

**Philipp:**

Yeah. Is nine cum tempore fine?

**Isolde (host):**

Nine c.t.?

**Philipp:**

I don't have a strong preference. I don't really want to start earlier,
because I need to walk over a bit...

**Isolde (host):**

A chance to really do a hands-on...

**Philipp:**

We might even do a bit of programming, tomorrow. We'll see.

**Isolde (host):**

So students could come and sit and watch how it's done and and learn and so
on.

Okay, well, then thank you to all the speakers of today!

{applause}

**Phiipp:**

{addressing the room} Okay. Nine c.t.!

**Isolde (host):**

They're here to answer questions, as well.

{end of the official part}
