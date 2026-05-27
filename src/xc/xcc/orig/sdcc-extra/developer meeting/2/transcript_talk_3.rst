{third talk and discussion, using slide set "ISO-C-Compliance.pdf"}

+--------------------------------------------------------------------------+
|                                   SDCC                                   |
|                                                                          |
|                        ISO C Standard Compliance                         |
+==========================================================================+
|                             Benedikt Freisen                             |
|                                                                          |
|                                2025-04-11                                |
+--------------------------------------------------------------------------+

**Isolde (host):**

Okay. Welcome back, everybody.

Next speaker is Benedikt Freisen on the ISO C standard compliance.

**Benedikt:**

Alright.

Now that we have heard about other aspects of the Small Device C Compiler,
the supported target architectures and such, I would like to continue with
ISO C standard compliance.

+--------------------------------------------------------------------------+
| Structure of this talk                                                   |
+==========================================================================+
| This talk will outline the status quo and objectives surrounding C       |
| standard compliance in SDCC, structured as follows:                      |
|                                                                          |
| * Motivation: Why ISO C?                                                 |
| * Recent Achievements                                                    |
| * Work(s) in Progress                                                    |
| * Challenges                                                             |
| * Modern C — Examples                                                    |
| * Future Construction Sites                                              |
| * Time for Questions / Feedback / Live Demo                              |
+--------------------------------------------------------------------------+

Structured in the following way:

First, I would like to start with motivation: "Why do we want ISO C in the
first place?", our recent achievements in SDCC, works in progress,
challenges we are facing.

Then I would like to give a few examples of modern C, [i.e.] what you can do
with it; things we have to tackle in the future, and then I would like to
close with a bit of time for questions, feedback and potentially a live
demo, although this is not my computer, so we would have to switch for that.

+--------------------------------------------------------------------------+
| Motivation: Why ISO C?                                                   |
+==========================================================================+
| The C programming language was originally standardized by ANSI           |
| in 1989, all subsequent versions by ISO                                  |
|                                                                          |
| * Features: Language extensions for new use cases                        |
| * Compatibility: Prevention of diverging dialects                        |
|                                                                          |
| Therefore, we want a Standard C compiler! (ISO C90 - C2Y)                |
| Obvious challenge: Standard C is constantly evolving                     |
+--------------------------------------------------------------------------+

First of all: Why do we even want ISO C?

Well, the C language, developed by Kernighan and Ritchie, was originally
standardized by ISO in 1989. Err... by ANSI in 1989, and then never touched
by ANSI, again. All subsequent versions were by ISO.

This addresses the important need for new language features arising from new
use cases and the desire to keep implementations from diverging. Therefore,
we wnat a standard C compiler for the various standardized ISO dialects.

And since these are constantly evolving, we have to keep working.

+--------------------------------------------------------------------------+
| Recent Achievements (pre 4.5.0)                                          |
+==========================================================================+
| Six out of the seven new features in 4.5.0 were related to current       |
| and future standard compliance:                                          |
|                                                                          |
| * Full atomic_flag support for msc51 and ds390 ports                     |
| * (Experimental f8 port)                                                 |
| * ISO C2y case range expressions                                         |
| * ISO C2y _Generic selection expression with a type operand              |
| * K&R-style function syntax (preliminarily with the semantics of         |
|   non-K&R ISO-style functions)                                           |
| * ISO C23 enums with user-specified underlying type                      |
| * struct / union in initializers                                         |
+--------------------------------------------------------------------------+

Our recent achievements prior to the most recent release are summarized
quite well in this excerpt from the release notes.

And as you can already see, six out of the seven features mentioned for that
release were related to standard compliance.

+--------------------------------------------------------------------------+
| Recent Achievements (post 4.5.0)                                         |
+==========================================================================+
| Five out of the six features added since 4.5.0 relate to ISO C, one      |
| to GNU C:                                                                |
|                                                                          |
| * C2y _Countof operator                                                  |
| * C2y octal                                                              |
| * C2y if-declaration                                                     |
| * Conditional operator with omitted second operand (GNU                  |
| * extension)                                                             |
| * C99 compound literals                                                  |
| * C23 compound literals with storage class specifiers                    |
+--------------------------------------------------------------------------+

But work has not stopped there. We have continued with a few features and
once again, five out of the six we added since then are related to ISO
standard compliance and one of them adds a popular addition from GNU C.

+--------------------------------------------------------------------------+
| Work(s) in Progress                                                      |
+==========================================================================+
| Various ISO C features have already been worked on, but are not          |
| yet complete, e.g.:                                                      |
|                                                                          |
| * C23 constexpr, i.e. named compile-time constant expressions            |
| * C23 qualifier-preserving library functions                             |
| * C90 flat initializer lists for nested arrays                           |
| * C23 checked integer arithmetic for 64 bit types                        |
+--------------------------------------------------------------------------+

We are already working on a couple of the missing features, for instance
from C23 the constant expressions that allow you to use named constants
-- named compile time constants -- without resorting to the preprocessor.

Qualifier-preserving library functions are a bit of a hack that uses the
preprocessor and a few other features to keep qualifiers such as const on
function arguments so that they propagate to the return value.

For instance, you have a string function that takes a character pointer as
input, but would like to use it such that calling it with a constant
character pointer returns a constant character pointer.

We don't necessarily need that in a freestanding implemention,
but it would definitely be nice to have, because it catches bugs.

Then, we're still working on support for flat initializer lists for nested
arrays. I will give an example on that later on, because it is a rather
obscure feature that many people won't even assume the language has, but
it's still necessary for standard compliance.

And also for catching bugs and a few other use cases, we would like to
complete support for checked integer arithmetic, that was added in C23 and
included several functions that allow to do basic arithmetic operations like
addition, multiplication and such and check, whether there was an overflow.

+--------------------------------------------------------------------------+
| Challenges                                                               |
+==========================================================================+
| There are obstacles preventing the speedy implementation of the          |
| aforementioned features:                                                 |
|                                                                          |
| * Qualifier-preserving wrapper macros rely on _Generic and ?:            |
|   specifics that are inaccurately implemented                            |
| * Flat initilizer lists can contain (top level) element designators,     |
|   but we have a bottom-up parser                                         |
| * constexpr must not trip over identifier shadowing                      |
| * 64 bit checked integer arithmetic must make do with 64 bit             |
|   intermediate values                                                    |
+--------------------------------------------------------------------------+

It's obviously not an entirely smooth ride.

We're facing a few challenges, for instance that the qualifier-preserving
wrapper macros rely on various language features that we don't fully
implement, yet.

And the flat initializer lists can also contain element designators that
didn't exist in original ANSI C, but they make everything a bit chaotic,
particularly because the element designators work on the top level of the
array, but we have a bottom-up parser.

Constant expressions are also already half-implemented in my local source
tree, but there's a problem, because while they are constant, while they
only contain constants in their initializers, there's thill the problem of
variable shadowing.
In other words: The same identifier might no longer be the same thing, and a
constant expression initializer cannot simply transplant initializer lists
from the constant expressions it contains.

The last problem, related to checked integer arithmetic, is that in the 64
bit case, which is the only one that is still missing, we have to somehow
make it work with 64 bit arithmetic for intermediate values.
That is possible, but it gets a bit funky.

+--------------------------------------------------------------------------+
| Challenges: Flat Array Initializer Lists                                 |
+==========================================================================+
| ANSI/ISO C inherits the K&R C bug/feature that initializer lists         |
| may ignore an array type's structure. This is valid::                    |
|                                                                          |
|   int array [ 2 ] [ 2 ] = { 1 , 2 , 3 };                                 |
|                                                                          |
| However, ISO C99 added the ability to designate specific array           |
| members::                                                                |
|                                                                          |
|   int array [ 2 ] [ 2 ] = { 1 , [2] = 2 , 3 };                           |
|                                                                          |
| Mixing both means trouble! (At least for compiler devs)                  |
+--------------------------------------------------------------------------+

Flat initializer lists are exactly this:
You have a multi-dimensional array, but a one-dimensional initializer list.
The problem here is that the original C compiler by Kernighan and Ritchie
did hardly any checking, and people apparently misunderstood that as a
feature of the language, and you can therefore use a one-dimensional
initializer list like that to fill up a two-dimensional array and it will
simply keep going and initialize it as if you did a memcpy from a one-
dimensional array to a two-dimensional array.

However, the feature added by C99, namely designated initializers for
particular array members complicates things a bit, because you can now say:
"I would now like to initialize the second element of the array, not the
first one!" by writing it like this.
[Note: The designator on the slide should have been 1, not 2]

But you still have the flat initializer lists and are confronted with the
problem: "What do we do when we encounter something like this?"
Well, it gets complicated.

+--------------------------------------------------------------------------+
| Modern C — Examples: _Generic                                            |
+==========================================================================+
| Generics (since C11) allow selecting expressions based on the type       |
| of a controlling expression::                                            |
|                                                                          |
|   _Generic(i, default : 0, int : 1, long : 2)                            |
|                                                                          |
| C2y will add the ability to select based on a type itself, which,        |
| combined with C23's typeof permits selection based on the                |
| qualified type::                                                         |
|                                                                          |
|   _Generic(typeof(i), int : 0, cntint : 1, default : 2)                  |
+--------------------------------------------------------------------------+

Let's just continue for now with examples for modern C.

For example the generics -- not quite as modern, they exist for 14 years,
now -- but they are quite nice because they allow you to have acontrolling
expression and select one expression or another or yet another based on the
type of the controlling expression:
If it's an int, you get "1" back, if it's a long, you get a "2" and if it's
anything else, you get a "0".
That's obviously not particularly useful if you write it down like that, but
it will usually be wrappedin a macro and can then be used for pseudo-generic
functions. They are not actually generic functions, but they will look like
it.

And with C2y there will be the possibility to use it not with an expression
of a particular type, but with a named type itself, which is also quite
useful if you combine it with C23's typeof, because you can then base the
selection on the qualified type.
In other words: You can select different expressions based on whether the
type is const-qualified or not,
whereas the top example here uses the unqualified version of the type.

+--------------------------------------------------------------------------+
| Modern C — Examples: typeof with function pointers                       |
+==========================================================================+
| C23's typeof permits a more intuitive spelling of function pointers::    |
|                                                                          |
|   void (∗signal(int sig, void (∗func)(int)))(int);                       |
|                                                                          |
| becomes::                                                                |
|                                                                          |
|   typeof(void (int)) ∗signal(int sig, typeof(void (int)) ∗func);         |
|                                                                          |
| But SDCC does not like this syntax, yet. :-/                             |
+--------------------------------------------------------------------------+

The typeof I just mentioned is also quite useful to write C pointers -- C
function pointers -- a bit more intuitively: For example, even seasoned C
programmers will struggle to understand, what exactly the type of this
pointer even is.

But with C23's typeof you can write it down quite nicely, such that you can
read it from left to right: A pointer to a function that takes an integer
and returns void. And you can read it out like: "typeof void int pointer"
That's the function return type.

And then you can work with it a bit more nicely. Unfortunately not yet in
SDCC, because our implementation of typeof is a little buggy.

+--------------------------------------------------------------------------+
| Modern C — Examples: case ranges                                         |
+==========================================================================+
| C2y will finally standardize GNU C's case ranges::                       |
|                                                                          |
|   switch (a) {                                                           |
|     case 0 ... 5: foo(); break;                                          |
|     case 6 ... 10: bar(); break;                                         |
|     default: baz(); break;                                               |
|   }                                                                      |
|                                                                          |
| The implementation in SDCC is based on a user-contributed                |
| patch. Those are welcome!¹                                               |
|                                                                          |
| ¹ Particularly when a feature has been standardized                      |
+--------------------------------------------------------------------------+

One more example: The committee finally decided to standardize the GNU C
extension that is case ranges, which are quite useful and which everyone who
has started programming in Pascal has missed in C.

That is already done. We already have that thanks to a user contribution.
Those are very welcome in case anybody ever wants to write and submit
patches. Particularly welcome when they address missing features that are
already standardized. Otherwise we might hesitate a bit, because it means
maintenance effort.

+--------------------------------------------------------------------------+
| Modern C — Examples: Octal Literals                                      |
+==========================================================================+
| Octal literals in C are notoriously unintuitive, as demonstrated by      |
| this pencil "correction" in a library copy of the book "The C            |
| Programming Language":                                                   |
|                                                                          |
| .. image:: octal.jpg                                                     |
|                                                                          |
| C2y will allow the more obviously prefixed spelling 0o177 known          |
| from languages like Rust.                                                |
| It will also allow escape sequences like "\o{177}".                      |
+--------------------------------------------------------------------------+

One more example of a new feature in C2y is that they decided to adopt a
more intuitive spelling of octal constants -- octal literal constants.

This for instance is a real-world example from the book "The C Programming
Language" in the 1978 edition, where a very eager reader felt a need to in
quotation marks "correct" the book.

What this expression -- what this statement is supposed to do is:
It masks the value in n such that only the lower seven bits remain.

And the book is actually correct, because this is an octal literal:
We have one bit in this digit and three one bits in this and this digit.
{pointing at the 1, 7 and 7 in the 0177 in the picture}

In other words: We have seven one bits in sequence, but many people mistake
this for a decimal constant padded with a zero.

C2y will adopt the Rust spelling and allows something like this {pointing at
the 0o177}, which immediately tells you that it's an octal and it also adds
brace-delimited escape sequences for strings.

+--------------------------------------------------------------------------+
| Future Construction Sites                                                |
+==========================================================================+
| Besides the points that are already being worked on, the following       |
| ones remain:                                                             |
|                                                                          |
| * Changes to lexer and parser for better C23 attribute support           |
| * Proper IEEE 754 floating point types and library                       |
| * Whatever the committee decides to put in C2y, e.g.                     |
|                                                                          |
|   * Lambda expressions (subset of what C++ has)                          |
|   * Deferred blocks (executed at the end of the containing block)        |
|   * Named loops (allowing e.g. continue outer;)                          |
|                                                                          |
| All of the above will presumably come with their own unique              |
| challenges.                                                              |
+--------------------------------------------------------------------------+

Future things we have to address are for instance that our lexer and parser
don't handle C23 attributes fully, yet.

We also need a proper IEEE 754 compliant floating point library and types at
some point.
Currently, we only support float and everything else is treated like float,
and our float is also not quite IEEE 754 compliant, because I believe we
don't support NaNs, yet.
{facing Philipp} Or do we support NaNs and not subnormal numbers or...?

**Philipp:**

We don't support subnormals. I'm not sure about NaN.

**Benedikt:**

Anyway, not quite IEEE 7543 compliant. Yes, we are dealing with very slow
devices, but people commonly expect IEEE 754 when they see a float or double
or long double, so it would be better to have them, and when I'm already
talking about double and long double, those definitely shouldn't be 32 bits.

**Philipp:**

The C standard would allow them to be 48 bits. The IEEE standard doesn't.

**Benedikt:**

But not 32.

**Philipp:**

Sure.

**Benedikt:**

And then we also ultimately have to add whatever the committee to put into
C2y or other future standards, such as lambda expressions, which will
probably be something like C++ has, but not to the full extent.

Deferred blocks are quite an interesting feature: You essentially write a
brace-enclosed block, but it isn't executed in that exact position, but at
the end of the current block. You can therefore wrap it in a macro, for
instance to disguise heap allocation as something you can use similar[ly] to
stack allocation. You can malloc a block of memory in the same macro
expansion and free it at the end of the block.
That would be quite nice, but probably hard to implement.

Named loops will also be interesting because you can then write something
like "continue outer;", if that's the name of the outer loop, iun nested
loops. Depending on what the committee decides on, that will be relatively
easy or a bit harder. We'll have to see.

+--------------------------------------------------------------------------+
| Time for Questions / Feedback / Live Demo                                |
+==========================================================================+
| * Any questions?                                                         |
| * Do you have feedback?                                                  |
| * Would you like to see details?                                         |
+--------------------------------------------------------------------------+

And... That was it, basically.

**Isolde (host):**

Questions?

**Michael:**

Just for understanding: So you are using a hand-written bottom-up parser if
I understood correctly? Or parser generators?

**Benedikt:**

We are actually using a flex lexer and a bison parser.

**Michael:**

Okay. Right.

**Benedikt:**

Which brings about it's own problems, such as...

**Michael:**

Yes.

**Benedikt:**

To convince flex to parse unicode identifiers, we ultimately resorted to
semi-automatically generating regular expressions for byte sequences that
are (a) valid utf8 code points and (b) utf8 code points that are valid in
identifiers.

That regular expression is definitely not pretty.

**Michael:**

So is nobody working on a unicode-aware version of flex? Or..

**Philipp:**

No. It seems like people who want to make a lexer generator that can handle
unicode want to make some great, shiny new thing that's much more powerful
than lex, and the people who maintain the old flex never bothered adding
unicode support.

**Michael:**

Yeah. Everything's seven bit ASCII. Yes.

**Philipp:**

Yes, there's other lexer and parser generator things that can handle
unicode, but they're not standard things.

**Michael:**

And replacing the lexer generator by something handwritten...
That should be feasible in your realm, or not?

**Benedikt:**

It's probably feasible in our case, because it doesn't use a lot of regular
expressions, otherwise.

It uses one monstrosity of a regular expression for unicode identifiers, but
otherwise it's not terribly difficult to parse a string literal or a
numeral.

**Michael:**

But you have to find someone who wants to work on that, yes.

**Benedikt:**

And in defense of the people who maintain flex, I understand where they are
coming from:
It essentially generates a lot of parsing tables and you don't necessarily
want to do that with a one-million-entry alphabet.

**Michael:**

Certainly no fun.

Are unicode identifiers required in recent C standards, actually?

**Benedikt:**

They are not required, but it's otherwise a bit of a pain to have...
The thing is that support for universal characters has been mandatory since
1994, but only as escape sequences:
You write a backslash, a "u" and then a number, but who wants that in an
identifier?

**Michael:**

Yeah. Sure.

**Benedikt:**

And because that's already required and we are already using utf8,
internally, it would be a bit of a shame not to somehow parse it from the
input.

**Michael:**

So, one thing I was wondering, because the Plan 9 C compiler already
supports unicode since like 1990-whatever, because the Plan 9 guys invented
utf8, and maybe taking a look at their lexer might be interesting.

**Benedikt:**

I presume that that part is simply handwritten.

**Michael:**

Yeah. Just an idea.

**Philipp:**

Maybe one remark:
Named loops break the bound on the tree-width of the control-flow graph, so
we'll definitely have to look into how this feature will be used, if we want
to know how this will affect us there.

**Benedikt:**

Further questions?

**Isolde (host):**

Can you maybe talk a bit more generally about how decisions are made, what
gets put into the standard and how the community works?

**Benedikt:**

I would love to know how the decisions are made, whats gets included in the
standard, I'm not involved in that.
We just have to deal with whatever the committee comes up with.

**Philipp:**

Okay. The convener basically asks: "Okay, let's hold a straw poll! Who's in
favor?" People raise their hands. "Who's against?" People raise their hands.
"Who abstains?" People raise their hands.

And then it's basically a simple majority. So yes, things get, in my
opinion, into the standard too easily. And I mean: The named loops thing for
example got in very narrowly. And if, like, two people who usually are at
the meatings, but happened to be not there at that time, and one of them is
me, the vote would have gone differently, but yeah, to get to the meeting
you need to be (typically) sent by a national standards body, so if you are
German you need to be a DIN member and be sent by the DIN towards ISO to
participate in standardization, or you can be a guest, but you can only be a
guest twice. They stretch the rules sometimes, then it was twice in a row.

Basically, if you have a proposal submitted, then you can go to two meetings
at which it is discussed, without being a member, but after that you have to
join.

It's, yeah, it's a bit chaotic, but still kind of works a bit better, I
think, than the C++ method, but it definitely feels something like the named
loops were not fully thought out and sometimes something gets in that
shouldn't, and it feels like sometimes, depending on who is there and their
[incomprehensible] of the rules, the rules are being upheld more strictly or
less strictly.

**Isolde (host):**

Mhm.

**N.N. 2:**

So there's like no debate? It's like simple majority wins?

**Philipp:**

Oh. No! There is definitely a debate before. I mean: Usually you have a
paper submitted to the committee. Then you get a time slot at a meeting to
discuss it, typically half an hour, and within that half an hour, well, you
present your idea. It gets discussed by all the people present and typically
it's a hybrid meeting, so other people there, as well. And then towards the
end of that typically half an hour time slot there's the votes.

**N.N. 2:**

And for the syntax it comes up with the personal preference like the new
language syntax like he was mentioning, like comma or is it like...

**Philipp:**

Well, there's a proposal, there's a syntax in it.

**N.N. 2:**

Yeah.

**Philipp:**

And I mean: If it gets voted in, it's in.

Of course it could very well happen that it does not get voted in and people
say: "We don't like the syntax."

**N.N. 2:**

Yeah.

**Philipp:**

I mean: I think that sometimes things get in too easily, but most things
that are proposed don't get into the standard. Most are rejected. Sometimes
definitely rejected, but more often something: "Well, we don't like this, we
don't like that. Come back with an updated proposal half a year later!"
And syntax is one of the aspects.

**N.N. 2:**

Yeah. Syntax is more of like personal preference, like more psychology more
than any science behind it.

**Philipp:**

Yeah, but then the idea is fit into the existing stuff, basically.
You don't try to make it into a different..., I mean...

**N.N. 2:**

Yeah.

**Philipp:**

And of course there's always the point of existing practice, yeah?

I mean: It still happens that things get in that they don't have enough
existing practice, but traditionally you need to have two reference
implementations in existing compilers. Sometimes it already being in C++
is counted as one of them.

---

{facing Benedikt} How is the current state of the attribute support?

I mean that's always been a problem with all those conflicts that we get
when we try to extend it.

**Benedikt:**

Well, it's currently what the online source code says.

**Philipp:**

Yeah. Ah, so you haven't looked into like extending it, because I think that
the standard allows attributes in more places than we currently allow in
SDCC.

**Benedikt:**

I believe what's currently coming back to bite us is the hack for
declarations after statements which I introduced rather superficially in the
parser, which we might... I believe we can stick to the basic approach to
treat declarations after statements like a newly opened block, but we would
have to do it one level deeper and somehow move it out of the parser to
avoid causing grammar conflicts.

**Philipp:**

We also sometimes look at this reference parser thing that Jens has been
working on a few years ago.

I mean it was a lex/yacc parser and it was able to parse C23.

**Benedikt:**

Yeah, but it didn't include our ugly hack.

**Philipp:**

Sure.

They do things differently, but I think we did pull some of the ideas from
there when we updated our parser during the Prototype Fund project.

Maybe there's still more. I don't know what happened to that one recently,
anyway.

**Isolde (host):**

So what were you going to show us?

**Benedikt:**

Depends on what everybody wants to see.
The source code itself, maybe?
Aspects of it?
Otherwise we can cut it short and discuss openly.

**Isolde (host):**

Preferences?

**N.N. 2:**

What do you think are the most importants aspects that are more meaningful
for all of us?

Asking this way.

**Benedikt:**

{paging through slides}

Well, perhaps we could take another look at new language features or
something like that?

Or... I don't know.

**Isolde (host):**

Yes!

**Benedikt:**

+--------------------------------------------------------------------------+
| Modern C — Examples: case ranges                                         |
+==========================================================================+
| C2y will finally standardize GNU C's case ranges::                       |
|                                                                          |
|   switch (a) {                                                           |
|     case 0 ... 5: foo(); break;                                          |
|     case 6 ... 10: bar(); break;                                         |
|     default: baz(); break;                                               |
|   }                                                                      |
|                                                                          |
| The implementation in SDCC is based on a user-contributed                |
| patch. Those are welcome!¹                                               |
|                                                                          |
| ¹ Particularly when a feature has been standardized                      |
+--------------------------------------------------------------------------+

Well this for instance does not count as exactly new, because the GNU C
compiler has had it since at leat about 2000, but it's really quite useful
that you can now -- or that you will be able to -- specify number ranges
instead of only a single constant.

**N.N. 2:**

[dsfgsd]

**Benedikt:**

That's basically the same as writing:
"case 0: case 1: case 2: case 3: case4: case 5: foo(); break;"
It's just a shorthand.

And as I mentrioned: GCC has had it since almost forever, but it's finally
being standardized.

+--------------------------------------------------------------------------+
| Modern C — Examples: typeof with function pointers                       |
+==========================================================================+
| C23's typeof permits a more intuitive spelling of function pointers::    |
|                                                                          |
|   void (∗signal(int sig, void (∗func)(int)))(int);                       |
|                                                                          |
| becomes::                                                                |
|                                                                          |
|   typeof(void (int)) ∗signal(int sig, typeof(void (int)) ∗func);         |
|                                                                          |
| But SDCC does not like this syntax, yet. :-/                             |
+--------------------------------------------------------------------------+

And what I particularly like is this spelling for function pointers. That
would simplify quite a few things, if you are reading through a header file
and immediately know what the function actually returns.

And that's actually already standard compliant since C23. We just don't
really support it, yet.

**N.N. 2:**

Why?

**Benedikt:**

Because our implementation is still a bit buggy relating to the typeof
operator. I haven't looked into it, yet.

**Michael:**

So, for floating point compliance: Are you currently using like a C
implementation of a floating point emulator or other assembler optimized
versions?

**Benedikt:**

We're using a C implementation that borrows a lot of code from a GCC
software floating point library from 35 years ago.

**Michael:**

Yeah. I know that quite well. It's not a nice piece of code.

**Benedikt:**

It isn't, but it's at least comprehensible compared to more modern GCC
softfloat libraries, because you still recognize it as C code. It's not
completely garbled with macros.

Other than that we've looked into Berkeley SoftFloat which we can at least
compile. I don't know if it actually works but we can at least compile it.

A few people have suggested their own hand-written assembly libraries for
floating point operations -- 32 bit floating point operations -- for Z80 or
6502. Those will have the same drawbacks as our current implementation, but
they will be substantially faster.

**Michael:**

I guess so, unless you invest a lot of work in your optimizer.

**Benedikt:**

Anything else?

In that case...

{ends presentation}
