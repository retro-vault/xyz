/*
   falcon-ntt.c
   falcon NTT. Falcon is a post-quantum-cryptography (PQC) signature scheme.
   The performance bottlenecks of Falcon signature verification are Keccak
   (tested in sha3-256.c) and the NTT, tested here. The bottleneck for key
   generation and signing is 64-bit floating-point arithmetic, for which there
   isno hope of a performant implementation on typical SDCC targets.
*/

#include <testfwk.h>

// Compared to other PQC signature schemes, Falcon has low memory requirements, but still too much for some targets.
#if defined(__SDCC_pdk13) || defined(__SDCC_pdk14) || defined(__SDCC_pdk15) \
  || defined(__SDCC_mcs51) && !defined(__SDCC_MODEL_LARGE) && !defined(__SDCC_MODEL_HUGE)
#define LACK_OF_MEMORY
#endif

// bug #3924
#if defined(SDCC_MOS) && defined(__SDCC_STACK_AUTO)
#define LACK_OF_MEMORY
#endif

#pragma disable_warning 336

#ifndef LACK_OF_MEMORY

// config.h
/*
 * Manual configuration file for the Falcon implementation. Here can
 * be set some compilation-time options.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#ifndef FALCON_CONFIG_H__
#define FALCON_CONFIG_H__

/*
 * Each option is a macro which should be defined to either 1 or 0.
 * If any of the options below is left undefined, then a default value
 * will be used by the code, possibly using compile-time autodetection
 * from compiler-defined macros.
 *
 * Explicitly setting a parameter can be done by uncommenting/modifying
 * its definition below, in this file, or equivalently by setting it as
 * a compiler flag.
 */

/*
 * Use the native 'double' C type for floating-point computations. Exact
 * reproducibility of all tests requires that type to faithfully follow
 * IEEE-754 "round-to-nearest" rules.
 *
 * Native double support will use the CPU hardware and/or
 * compiler-provided functions; the latter is typically NOT
 * constant-time, while the former MAY be constant-time, or not. On
 * recent x86 CPU in 64-bit mode, SSE2 opcodes are used and they provide
 * constant-time operations for all the operations used in Falcon,
 * except for some special cases of divisions and square roots, but it
 * can be shown that theses cases imply only negligible leak of
 * information that cannot be leveraged into a full attack.
 *
 * If neither FALCON_FPNATIVE nor FALCON_FPEMU is defined, then use of
 * the native 'double' C type is the default behaviour unless
 * FALCON_ASM_CORTEXM4 is defined to 1, in which case the emulated code
 * will be used.
 *
#define FALCON_FPNATIVE   1
 */

/*
 * Use emulated floating-point implementation.
 *
 * Emulation uses only integer operations with uint32_t and uint64_t
 * types. This is constant-time, provided that the underlying platform
 * offers constant-time opcodes for the following operations:
 *
 *  - Multiplication of two 32-bit unsigned integers into a 64-bit result.
 *  - Left-shift or right-shift of a 32-bit unsigned integer by a
 *    potentially secret shift count in the 0..31 range.
 *
 * Notably, the ARM Cortex M3 does not fulfill the first condition,
 * while the Pentium IV does not fulfill the second.
 *
 * If neither FALCON_FPNATIVE nor FALCON_FPEMU is defined, then use of
 * the native 'double' C type is the default behaviour unless
 * FALCON_ASM_CORTEXM4 is defined to 1, in which case the emulated code
 * will be used.
 *
#define FALCON_FPEMU   1
 */

/*
 * Enable use of assembly for ARM Cortex-M4 CPU. By default, such
 * support will be used based on some autodection on the compiler
 * version and target architecture. Define this variable to 1 to force
 * use of the assembly code, or 0 to disable it regardless of the
 * autodetection.
 *
 * When FALCON_ASM_CORTEXM4 is enabled (whether defined explicitly or
 * autodetected), emulated floating-point code will be used, unless
 * FALCON_FPNATIVE or FALCON_FPEMU is explicitly set to override the
 * choice. Emulated code with ARM assembly is constant-time and provides
 * better performance than emulated code with plain C.
 *
 * The assembly code for the M4 can also work on a Cortex-M3. If the
 * compiler is instructed to target the M3 (e.g. '-mcpu=cortex-m3' with
 * GCC) then FALCON_ASM_CORTEXM4 won't be autodetected, but it can be
 * enabled explicitly. Take care, though, that the M3 multiplication
 * opcode (multiplication of two 32-bit unsigned integers with a 64-bit
 * result) is NOT constant-time.
 *
#define FALCON_ASM_CORTEXM4   1
 */

/*
 * Enable use of AVX2 intrinsics. If enabled, then the code will compile
 * only when targeting x86 with a compiler that supports AVX2 intrinsics
 * (tested with GCC 7.4.0, Clang 6.0.0, and MSVC 2015, both in 32-bit
 * and 64-bit modes), and run only on systems that offer the AVX2
 * opcodes. Some operations leverage AVX2 for better performance.
 *
#define FALCON_AVX2   1
 */

/*
 * Enable use of FMA intrinsics. This setting has any effect only if
 * FALCON_AVX2 is also enabled. The FMA intrinsics are normally available
 * on any x86 CPU that also has AVX2. Note that setting this option will
 * slightly modify the values of expanded private keys, but will normally
 * not change the values of non-expanded private keys, public keys or
 * signatures, for a given keygen/sign seed (non-expanded private keys
 * and signatures might theoretically change, but only with low probability,
 * less than 2^(-40); produced signatures are still safe and interoperable).
 *
#define FALCON_FMA   1
 */

/*
 * Assert that the platform uses little-endian encoding. If enabled,
 * then encoding and decoding of aligned multibyte values will be
 * slightly faster (especially for hashing and random number
 * generation). If not defined explicitly, then autodetection is
 * applied.
 *
#define FALCON_LE   1
 */

/*
 * Assert that the platform tolerates accesses to unaligned multibyte
 * values. If enabled, then some operations are slightly faster. Note
 * that ARM Cortex M4 do _not_ fully tolerate unaligned accesses; for
 * such systems, this option should not be enabled. If not defined
 * explicitly, then autodetection is applied.
 *
#define FALCON_UNALIGNED   1
 */

/*
 * Use a PRNG based on ChaCha20 and seeded with SHAKE256, instead of
 * SHAKE256 directly, for key pair generation purposes. This speeds up
 * key pair generation, especially on platforms where SHAKE256 is
 * comparatively slow: on the ARM Cortex M4, average key generation time
 * is reduced by 19% with this setting; on a recent x86 Skylake, the
 * reduction is smaller (less than 8%).
 *
 * However, this setting changes the private/public key pair obtained
 * from a given seed, thus preventing reproducibility of the
 * known-answer tests vectors. For compatibility with existing KAT
 * vectors (e.g. in PQClean, pqm4 and NIST implementations), this
 * setting is not enabled by default.
 *
#define FALCON_KG_CHACHA20   1
 */

/*
 * Use an explicit OS-provided source of randomness for seeding (for the
 * Zf(get_seed)() function implementation). Three possible sources are
 * defined:
 *
 *  - getentropy() system call
 *  - /dev/urandom special file
 *  - CryptGenRandom() function call
 *
 * More than one source may be enabled, in which case they will be tried
 * in the order above, until a success is reached.
 *
 * By default, sources are enabled at compile-time based on these
 * conditions:
 *
 *  - getentropy(): target is one of: Linux with Glibc-2.25+, FreeBSD 12+,
 *    or OpenBSD.
 *  - /dev/urandom: target is a Unix-like system (including Linux,
 *    FreeBSD, NetBSD, OpenBSD, DragonFly, macOS, Android, Solaris, AIX).
 *  - CryptGenRandom(): target is Windows (Win32 or Win64).
 *
 * On most small embedded systems, none will be enabled and Zf(get_seed)()
 * will always return 0. Applications will need to provide their own seeds.
 *
#define FALCON_RAND_GETENTROPY   1
#define FALCON_RAND_URANDOM      1
#define FALCON_RAND_WIN32        1
 */

#endif

// inner.h
#ifndef FALCON_INNER_H__
#define FALCON_INNER_H__

/*
 * Internal functions for Falcon. This is not the API intended to be
 * used by applications; instead, this internal API provides all the
 * primitives on which wrappers build to provide external APIs.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

/*
 * IMPORTANT API RULES
 * -------------------
 *
 * This API has some non-trivial usage rules:
 *
 *
 *  - All public functions (i.e. the non-static ones) must be referenced
 *    with the Zf() macro (e.g. Zf(verify_raw) for the verify_raw()
 *    function). That macro adds a prefix to the name, which is
 *    configurable with the FALCON_PREFIX macro. This allows compiling
 *    the code into a specific "namespace" and potentially including
 *    several versions of this code into a single application (e.g. to
 *    have an AVX2 and a non-AVX2 variants and select the one to use at
 *    runtime based on availability of AVX2 opcodes).
 *
 *  - Functions that need temporary buffers expects them as a final
 *    tmp[] array of type uint8_t*, with a size which is documented for
 *    each function. However, most have some alignment requirements,
 *    because they will use the array to store 16-bit, 32-bit or 64-bit
 *    values (e.g. uint64_t or double). The caller must ensure proper
 *    alignment. What happens on unaligned access depends on the
 *    underlying architecture, ranging from a slight time penalty
 *    to immediate termination of the process.
 *
 *  - Some functions rely on specific rounding rules and precision for
 *    floating-point numbers. On some systems (in particular 32-bit x86
 *    with the 387 FPU), this requires setting an hardware control
 *    word. The caller MUST use set_fpu_cw() to ensure proper precision:
 *
 *      oldcw = set_fpu_cw(2);
 *      Zf(sign_dyn)(...);
 *      set_fpu_cw(oldcw);
 *
 *    On systems where the native floating-point precision is already
 *    proper, or integer-based emulation is used, the set_fpu_cw()
 *    function does nothing, so it can be called systematically.
 */

// yyyPQCLEAN+0 yyyNIST+0 yyySUPERCOP+0
//#include "config.h"
// yyyPQCLEAN- yyyNIST- yyySUPERCOP-
// yyySUPERCOP+1
// yyyCONF*
// yyySUPERCOP-

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined FALCON_AVX2 && FALCON_AVX2 // yyyAVX2+1
/*
 * This implementation uses AVX2 and optionally FMA intrinsics.
 */
#include <immintrin.h>
#ifndef FALCON_LE
#define FALCON_LE   1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   1
#endif
#if defined __GNUC__
#if defined FALCON_FMA && FALCON_FMA
#define TARGET_AVX2   __attribute__((target("avx2,fma")))
#else
#define TARGET_AVX2   __attribute__((target("avx2")))
#endif
#elif defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4752 )
#endif
#if defined FALCON_FMA && FALCON_FMA
#define FMADD(a, b, c)   _mm256_fmadd_pd(a, b, c)
#define FMSUB(a, b, c)   _mm256_fmsub_pd(a, b, c)
#else
#define FMADD(a, b, c)   _mm256_add_pd(_mm256_mul_pd(a, b), c)
#define FMSUB(a, b, c)   _mm256_sub_pd(_mm256_mul_pd(a, b), c)
#endif
#endif // yyyAVX2-

// yyyNIST+0 yyyPQCLEAN+0
/*
 * On MSVC, disable warning about applying unary minus on an unsigned
 * type: this is perfectly defined standard behaviour and we do it
 * quite often.
 */
#if defined _MSC_VER && _MSC_VER
#pragma warning( disable : 4146 )
#endif

// yyySUPERCOP+0
/*
 * Enable ARM assembly on any ARMv7m platform (if it was not done before).
 */
#ifndef FALCON_ASM_CORTEXM4
#if (defined __ARM_ARCH_7EM__ && __ARM_ARCH_7EM__) \
	&& (defined __ARM_FEATURE_DSP && __ARM_FEATURE_DSP)
#define FALCON_ASM_CORTEXM4   1
#else
#define FALCON_ASM_CORTEXM4   0
#endif
#endif
// yyySUPERCOP-

#if defined __i386__ || defined _M_IX86 \
	|| defined __x86_64__ || defined _M_X64 || \
	(defined _ARCH_PWR8 && \
		(defined __LITTLE_ENDIAN || defined __LITTLE_ENDIAN__))

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   1
#endif

#elif defined FALCON_ASM_CORTEXM4 && FALCON_ASM_CORTEXM4

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#elif (defined __LITTLE_ENDIAN__ && __LITTLE_ENDIAN__) \
	|| (defined __BYTE_ORDER__ && defined __ORDER_LITTLE_ENDIAN__ \
		&& __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

#ifndef FALCON_LE
#define FALCON_LE     1
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#else

#ifndef FALCON_LE
#define FALCON_LE     0
#endif
#ifndef FALCON_UNALIGNED
#define FALCON_UNALIGNED   0
#endif

#endif

/*
 * We ensure that both FALCON_FPEMU and FALCON_FPNATIVE are defined,
 * with compatible values (exactly one of them must be non-zero).
 * If none is defined, then default FP implementation is 'native'
 * except on ARM Cortex M4.
 */
#if !defined FALCON_FPEMU && !defined FALCON_FPNATIVE

#if (defined __ARM_FP && ((__ARM_FP & 0x08) == 0x08)) \
	|| (!defined __ARM_FP && defined __ARM_VFPV2__)
#define FALCON_FPEMU      0
#define FALCON_FPNATIVE   1
#elif defined FALCON_ASM_CORTEXM4 && FALCON_ASM_CORTEXM4
#define FALCON_FPEMU      1
#define FALCON_FPNATIVE   0
#else
#define FALCON_FPEMU      0
#define FALCON_FPNATIVE   1
#endif

#elif defined FALCON_FPEMU && !defined FALCON_FPNATIVE

#if FALCON_FPEMU
#define FALCON_FPNATIVE   0
#else
#define FALCON_FPNATIVE   1
#endif

#elif defined FALCON_FPNATIVE && !defined FALCON_FPEMU

#if FALCON_FPNATIVE
#define FALCON_FPEMU   0
#else
#define FALCON_FPEMU   1
#endif

#endif

#if (FALCON_FPEMU && FALCON_FPNATIVE) || (!FALCON_FPEMU && !FALCON_FPNATIVE)
#error Exactly one of FALCON_FPEMU and FALCON_FPNATIVE must be selected
#endif

// yyySUPERCOP+0
/*
 * For seed generation from the operating system:
 *  - On Linux and glibc-2.25+, FreeBSD 12+ and OpenBSD, use getentropy().
 *  - On Unix-like systems, use /dev/urandom (including as a fallback
 *    for failed getentropy() calls).
 *  - On Windows, use CryptGenRandom().
 */

#ifndef FALCON_RAND_GETENTROPY
#if (defined __linux__ && defined __GLIBC__ \
	&& (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 25))) \
	|| (defined __FreeBSD__ && __FreeBSD__ >= 12) \
	|| defined __OpenBSD__
#define FALCON_RAND_GETENTROPY   1
#else
#define FALCON_RAND_GETENTROPY   0
#endif
#endif

#ifndef FALCON_RAND_URANDOM
#if defined _AIX \
	|| defined __ANDROID__ \
	|| defined __FreeBSD__ \
	|| defined __NetBSD__ \
	|| defined __OpenBSD__ \
	|| defined __DragonFly__ \
	|| defined __linux__ \
	|| (defined __sun && (defined __SVR4 || defined __svr4__)) \
	|| (defined __APPLE__ && defined __MACH__)
#define FALCON_RAND_URANDOM   1
#else
#define FALCON_RAND_URANDOM   0
#endif
#endif

#ifndef FALCON_RAND_WIN32
#if defined _WIN32 || defined _WIN64
#define FALCON_RAND_WIN32   1
#else
#define FALCON_RAND_WIN32   0
#endif
#endif
// yyySUPERCOP-

/*
 * For still undefined compile-time macros, define them to 0 to avoid
 * warnings with -Wundef.
 */
#ifndef FALCON_AVX2
#define FALCON_AVX2   0
#endif
#ifndef FALCON_FMA
#define FALCON_FMA   0
#endif
#ifndef FALCON_KG_CHACHA20
#define FALCON_KG_CHACHA20   0
#endif
// yyyNIST- yyyPQCLEAN-

// yyyPQCLEAN+0 yyySUPERCOP+0
/*
 * "Naming" macro used to apply a consistent prefix over all global
 * symbols.
 */
#ifndef FALCON_PREFIX
#define FALCON_PREFIX   falcon_inner
#endif
#define Zf(name)             Zf_(FALCON_PREFIX, name)
#define Zf_(prefix, name)    Zf__(prefix, name)
#define Zf__(prefix, name)   prefix ## _ ## name  
// yyyPQCLEAN- yyySUPERCOP-

// yyyAVX2+1
/*
 * We use the TARGET_AVX2 macro to tag some functions which, in some
 * configurations, may use AVX2 and FMA intrinsics; this depends on
 * the compiler. In all other cases, we just define it to emptiness
 * (i.e. it will have no effect).
 */
#ifndef TARGET_AVX2
#define TARGET_AVX2
#endif
// yyyAVX2-

/*
 * Some computations with floating-point elements, in particular
 * rounding to the nearest integer, rely on operations using _exactly_
 * the precision of IEEE-754 binary64 type (i.e. 52 bits). On 32-bit
 * x86, the 387 FPU may be used (depending on the target OS) and, in
 * that case, may use more precision bits (i.e. 64 bits, for an 80-bit
 * total type length); to prevent miscomputations, we define an explicit
 * function that modifies the precision in the FPU control word.
 *
 * set_fpu_cw() sets the precision to the provided value, and returns
 * the previously set precision; callers are supposed to restore the
 * previous precision on exit. The correct (52-bit) precision is
 * configured with the value "2". On unsupported compilers, or on
 * targets other than 32-bit x86, or when the native 'double' type is
 * not used, the set_fpu_cw() function does nothing at all.
 */
#if FALCON_FPNATIVE  // yyyFPNATIVE+1
#if defined __GNUC__ && defined __i386__
static inline unsigned
set_fpu_cw(unsigned x)
{
	unsigned short t;
	unsigned old;

	__asm__ __volatile__ ("fstcw %0" : "=m" (t) : : );
	old = (t & 0x0300u) >> 8;
	t = (unsigned short)((t & ~0x0300u) | (x << 8));
	__asm__ __volatile__ ("fldcw %0" : : "m" (t) : );
	return old;
}
#elif defined _M_IX86
static inline unsigned
set_fpu_cw(unsigned x)
{
	unsigned short t;
	unsigned old;

	__asm { fstcw t }
	old = (t & 0x0300u) >> 8;
	t = (unsigned short)((t & ~0x0300u) | (x << 8));
	__asm { fldcw t }
	return old;
}
#else
static inline unsigned
set_fpu_cw(unsigned x)
{
	return x;
}
#endif
#else  // yyyFPNATIVE+0
static inline unsigned
set_fpu_cw(unsigned x)
{
	return x;
}
#endif  // yyyFPNATIVE-

#if FALCON_FPNATIVE && !FALCON_AVX2  // yyyFPNATIVE+1 yyyAVX2+0
/*
 * If using the native 'double' type but not AVX2 code, on an x86
 * machine with SSE2 activated for maths, then we will use the
 * SSE2 intrinsics.
 */
#if defined __GNUC__ && defined __SSE2_MATH__
#include <immintrin.h>
#endif
#endif  // yyyFPNATIVE- yyyAVX2-

#if FALCON_FPNATIVE  // yyyFPNATIVE+1
/*
 * For optimal reproducibility of values, we need to disable contraction
 * of floating-point expressions; otherwise, on some architectures (e.g.
 * PowerPC), the compiler may generate fused-multiply-add opcodes that
 * may round differently than two successive separate opcodes. C99 defines
 * a standard pragma for that, but GCC-6.2.2 appears to ignore it,
 * hence the GCC-specific pragma (that Clang does not support).
 */
#if defined __clang__
#pragma STDC FP_CONTRACT OFF
#elif defined __GNUC__
#pragma GCC optimize ("fp-contract=off")
#endif
#endif  // yyyFPNATIVE-

// yyyPQCLEAN+0
/*
 * MSVC 2015 does not know the C99 keyword 'restrict'.
 */
#if defined _MSC_VER && _MSC_VER
#ifndef restrict
#define restrict   __restrict
#endif
#endif
// yyyPQCLEAN-

/* ==================================================================== */
/*
 * SHAKE256 implementation (shake.c).
 *
 * API is defined to be easily replaced with the fips202.h API defined
 * as part of PQClean.
 */

// yyyPQCLEAN+0
typedef struct {
	union {
		uint64_t A[25];
		uint8_t dbuf[200];
	} st;
	uint64_t dptr;
} inner_shake256_context;

#define inner_shake256_init      Zf(i_shake256_init)
#define inner_shake256_inject    Zf(i_shake256_inject)
#define inner_shake256_flip      Zf(i_shake256_flip)
#define inner_shake256_extract   Zf(i_shake256_extract)

void Zf(i_shake256_init)(
	inner_shake256_context *sc);
void Zf(i_shake256_inject)(
	inner_shake256_context *sc, const uint8_t *in, size_t len);
void Zf(i_shake256_flip)(
	inner_shake256_context *sc);
void Zf(i_shake256_extract)(
	inner_shake256_context *sc, uint8_t *out, size_t len);

/*
// yyyPQCLEAN+1

#include "fips202.h"

#define inner_shake256_context                shake256incctx
#define inner_shake256_init(sc)               shake256_inc_init(sc)
#define inner_shake256_inject(sc, in, len)    shake256_inc_absorb(sc, in, len)
#define inner_shake256_flip(sc)               shake256_inc_finalize(sc)
#define inner_shake256_extract(sc, out, len)  shake256_inc_squeeze(out, len, sc)

// yyyPQCLEAN+0
 */
// yyyPQCLEAN-

/* ==================================================================== */
/*
 * Encoding/decoding functions (codec.c).
 *
 * Encoding functions take as parameters an output buffer (out) with
 * a given maximum length (max_out_len); returned value is the actual
 * number of bytes which have been written. If the output buffer is
 * not large enough, then 0 is returned (some bytes may have been
 * written to the buffer). If 'out' is NULL, then 'max_out_len' is
 * ignored; instead, the function computes and returns the actual
 * required output length (in bytes).
 *
 * Decoding functions take as parameters an input buffer (in) with
 * its maximum length (max_in_len); returned value is the actual number
 * of bytes that have been read from the buffer. If the provided length
 * is too short, then 0 is returned.
 *
 * Values to encode or decode are vectors of integers, with N = 2^logn
 * elements.
 *
 * Three encoding formats are defined:
 *
 *   - modq: sequence of values modulo 12289, each encoded over exactly
 *     14 bits. The encoder and decoder verify that integers are within
 *     the valid range (0..12288). Values are arrays of uint16.
 *
 *   - trim: sequence of signed integers, a specified number of bits
 *     each. The number of bits is provided as parameter and includes
 *     the sign bit. Each integer x must be such that |x| < 2^(bits-1)
 *     (which means that the -2^(bits-1) value is forbidden); encode and
 *     decode functions check that property. Values are arrays of
 *     int16_t or int8_t, corresponding to names 'trim_i16' and
 *     'trim_i8', respectively.
 *
 *   - comp: variable-length encoding for signed integers; each integer
 *     uses a minimum of 9 bits, possibly more. This is normally used
 *     only for signatures.
 *
 */

size_t Zf(modq_encode)(void *out, size_t max_out_len,
	const uint16_t *x, unsigned logn);
size_t Zf(trim_i16_encode)(void *out, size_t max_out_len,
	const int16_t *x, unsigned logn, unsigned bits);
size_t Zf(trim_i8_encode)(void *out, size_t max_out_len,
	const int8_t *x, unsigned logn, unsigned bits);
size_t Zf(comp_encode)(void *out, size_t max_out_len,
	const int16_t *x, unsigned logn);

size_t Zf(modq_decode)(uint16_t *x, unsigned logn,
	const void *in, size_t max_in_len);
size_t Zf(trim_i16_decode)(int16_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t Zf(trim_i8_decode)(int8_t *x, unsigned logn, unsigned bits,
	const void *in, size_t max_in_len);
size_t Zf(comp_decode)(int16_t *x, unsigned logn,
	const void *in, size_t max_in_len);

/*
 * Number of bits for key elements, indexed by logn (1 to 10). This
 * is at most 8 bits for all degrees, but some degrees may have shorter
 * elements.
 */
extern const uint8_t Zf(max_fg_bits)[];
extern const uint8_t Zf(max_FG_bits)[];

/*
 * Maximum size, in bits, of elements in a signature, indexed by logn
 * (1 to 10). The size includes the sign bit.
 */
extern const uint8_t Zf(max_sig_bits)[];

/* ==================================================================== */
/*
 * Support functions used for both signature generation and signature
 * verification (common.c).
 */

/*
 * From a SHAKE256 context (must be already flipped), produce a new
 * point. This is the non-constant-time version, which may leak enough
 * information to serve as a stop condition on a brute force attack on
 * the hashed message (provided that the nonce value is known).
 */
void Zf(hash_to_point_vartime)(inner_shake256_context *sc,
	uint16_t *x, unsigned logn);

/*
 * From a SHAKE256 context (must be already flipped), produce a new
 * point. The temporary buffer (tmp) must have room for 2*2^logn bytes.
 * This function is constant-time but is typically more expensive than
 * Zf(hash_to_point_vartime)().
 *
 * tmp[] must have 16-bit alignment.
 */
void Zf(hash_to_point_ct)(inner_shake256_context *sc,
	uint16_t *x, unsigned logn, uint8_t *tmp);

/*
 * Tell whether a given vector (2N coordinates, in two halves) is
 * acceptable as a signature. This compares the appropriate norm of the
 * vector with the acceptance bound. Returned value is 1 on success
 * (vector is short enough to be acceptable), 0 otherwise.
 */
int Zf(is_short)(const int16_t *s1, const int16_t *s2, unsigned logn);

/*
 * Tell whether a given vector (2N coordinates, in two halves) is
 * acceptable as a signature. Instead of the first half s1, this
 * function receives the "saturated squared norm" of s1, i.e. the
 * sum of the squares of the coordinates of s1 (saturated at 2^32-1
 * if the sum exceeds 2^31-1).
 *
 * Returned value is 1 on success (vector is short enough to be
 * acceptable), 0 otherwise.
 */
int Zf(is_short_half)(uint32_t sqn, const int16_t *s2, unsigned logn);

/* ==================================================================== */
/*
 * Signature verification functions (vrfy.c).
 */

/*
 * Convert a public key to NTT + Montgomery format. Conversion is done
 * in place.
 */
void Zf(to_ntt_monty)(uint16_t *h, unsigned logn);

/*
 * Internal signature verification code:
 *   c0[]      contains the hashed nonce+message
 *   s2[]      is the decoded signature
 *   h[]       contains the public key, in NTT + Montgomery format
 *   logn      is the degree log
 *   tmp[]     temporary, must have at least 2*2^logn bytes
 * Returned value is 1 on success, 0 on error.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(verify_raw)(const uint16_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, uint8_t *tmp);

/*
 * Compute the public key h[], given the private key elements f[] and
 * g[]. This computes h = g/f mod phi mod q, where phi is the polynomial
 * modulus. This function returns 1 on success, 0 on error (an error is
 * reported if f is not invertible mod phi mod q).
 *
 * The tmp[] array must have room for at least 2*2^logn elements.
 * tmp[] must have 16-bit alignment.
 */
int Zf(compute_public)(uint16_t *h,
	const int8_t *f, const int8_t *g, unsigned logn, uint8_t *tmp);

/*
 * Recompute the fourth private key element. Private key consists in
 * four polynomials with small coefficients f, g, F and G, which are
 * such that fG - gF = q mod phi; furthermore, f is invertible modulo
 * phi and modulo q. This function recomputes G from f, g and F.
 *
 * The tmp[] array must have room for at least 4*2^logn bytes.
 *
 * Returned value is 1 in success, 0 on error (f not invertible).
 * tmp[] must have 16-bit alignment.
 */
int Zf(complete_private)(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint8_t *tmp);

/*
 * Test whether a given polynomial is invertible modulo phi and q.
 * Polynomial coefficients are small integers.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(is_invertible)(
	const int16_t *s2, unsigned logn, uint8_t *tmp);

/*
 * Count the number of elements of value zero in the NTT representation
 * of the given polynomial: this is the number of primitive 2n-th roots
 * of unity (modulo q = 12289) that are roots of the provided polynomial
 * (taken modulo q).
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(count_nttzero)(const int16_t *sig, unsigned logn, uint8_t *tmp);

/*
 * Internal signature verification with public key recovery:
 *   h[]       receives the public key (NOT in NTT/Montgomery format)
 *   c0[]      contains the hashed nonce+message
 *   s1[]      is the first signature half
 *   s2[]      is the second signature half
 *   logn      is the degree log
 *   tmp[]     temporary, must have at least 2*2^logn bytes
 * Returned value is 1 on success, 0 on error. Success is returned if
 * the signature is a short enough vector; in that case, the public
 * key has been written to h[]. However, the caller must still
 * verify that h[] is the correct value (e.g. with regards to a known
 * hash of the public key).
 *
 * h[] may not overlap with any of the other arrays.
 *
 * tmp[] must have 16-bit alignment.
 */
int Zf(verify_recover)(uint16_t *h,
	const uint16_t *c0, const int16_t *s1, const int16_t *s2,
	unsigned logn, uint8_t *tmp);

#if 0
/* ==================================================================== */
/*
 * Implementation of floating-point real numbers (fpr.h, fpr.c).
 */

/*
 * Real numbers are implemented by an extra header file, included below.
 * This is meant to support pluggable implementations. The default
 * implementation relies on the C type 'double'.
 *
 * The included file must define the following types, functions and
 * constants:
 *
 *   fpr
 *         type for a real number
 *
 *   fpr fpr_of(int64_t i)
 *         cast an integer into a real number; source must be in the
 *         -(2^63-1)..+(2^63-1) range
 *
 *   fpr fpr_scaled(int64_t i, int sc)
 *         compute i*2^sc as a real number; source 'i' must be in the
 *         -(2^63-1)..+(2^63-1) range
 *
 *   fpr fpr_ldexp(fpr x, int e)
 *         compute x*2^e
 *
 *   int64_t fpr_rint(fpr x)
 *         round x to the nearest integer; x must be in the -(2^63-1)
 *         to +(2^63-1) range
 *
 *   int64_t fpr_trunc(fpr x)
 *         round to an integer; this rounds towards zero; value must
 *         be in the -(2^63-1) to +(2^63-1) range
 *
 *   fpr fpr_add(fpr x, fpr y)
 *         compute x + y
 *
 *   fpr fpr_sub(fpr x, fpr y)
 *         compute x - y
 *
 *   fpr fpr_neg(fpr x)
 *         compute -x
 *
 *   fpr fpr_half(fpr x)
 *         compute x/2
 *
 *   fpr fpr_double(fpr x)
 *         compute x*2
 *
 *   fpr fpr_mul(fpr x, fpr y)
 *         compute x * y
 *
 *   fpr fpr_sqr(fpr x)
 *         compute x * x
 *
 *   fpr fpr_inv(fpr x)
 *         compute 1/x
 *
 *   fpr fpr_div(fpr x, fpr y)
 *         compute x/y
 *
 *   fpr fpr_sqrt(fpr x)
 *         compute the square root of x
 *
 *   int fpr_lt(fpr x, fpr y)
 *         return 1 if x < y, 0 otherwise
 *
 *   uint64_t fpr_expm_p63(fpr x)
 *         return exp(x), assuming that 0 <= x < log(2). Returned value
 *         is scaled to 63 bits (i.e. it really returns 2^63*exp(-x),
 *         rounded to the nearest integer). Computation should have a
 *         precision of at least 45 bits.
 *
 *   const fpr fpr_gm_tab[]
 *         array of constants for FFT / iFFT
 *
 *   const fpr fpr_p2_tab[]
 *         precomputed powers of 2 (by index, 0 to 10)
 *
 * Constants of type 'fpr':
 *
 *   fpr fpr_q                 12289
 *   fpr fpr_inverse_of_q      1/12289
 *   fpr fpr_inv_2sqrsigma0    1/(2*(1.8205^2))
 *   fpr fpr_inv_sigma[]       1/sigma (indexed by logn, 1 to 10)
 *   fpr fpr_sigma_min[]       1/sigma_min (indexed by logn, 1 to 10)
 *   fpr fpr_log2              log(2)
 *   fpr fpr_inv_log2          1/log(2)
 *   fpr fpr_bnorm_max         16822.4121
 *   fpr fpr_zero              0
 *   fpr fpr_one               1
 *   fpr fpr_two               2
 *   fpr fpr_onehalf           0.5
 *   fpr fpr_ptwo31            2^31
 *   fpr fpr_ptwo31m1          2^31-1
 *   fpr fpr_mtwo31m1          -(2^31-1)
 *   fpr fpr_ptwo63m1          2^63-1
 *   fpr fpr_mtwo63m1          -(2^63-1)
 *   fpr fpr_ptwo63            2^63
 */
//#include "fpr.h"

/* ==================================================================== */
/*
 * RNG (rng.c).
 *
 * A PRNG based on ChaCha20 is implemented; it is seeded from a SHAKE256
 * context (flipped) and is used for bulk pseudorandom generation.
 * A system-dependent seed generator is also provided.
 */

/*
 * Obtain a random seed from the system RNG.
 *
 * Returned value is 1 on success, 0 on error.
 */
int Zf(get_seed)(void *seed, size_t seed_len);

/*
 * Structure for a PRNG. This includes a large buffer so that values
 * get generated in advance. The 'state' is used to keep the current
 * PRNG algorithm state (contents depend on the selected algorithm).
 *
 * The unions with 'dummy_u64' are there to ensure proper alignment for
 * 64-bit direct access.
 */
typedef struct {
	union {
		uint8_t d[512]; /* MUST be 512, exactly */
		uint64_t dummy_u64;
	} buf;
	size_t ptr;
	union {
		uint8_t d[256];
		uint64_t dummy_u64;
	} state;
	int type;
} prng;

/*
 * Instantiate a PRNG. That PRNG will feed over the provided SHAKE256
 * context (in "flipped" state) to obtain its initial state.
 */
void Zf(prng_init)(prng *p, inner_shake256_context *src);

/*
 * Refill the PRNG buffer. This is normally invoked automatically, and
 * is declared here only so that prng_get_u64() may be inlined.
 */
void Zf(prng_refill)(prng *p);

/*
 * Get some bytes from a PRNG.
 */
void Zf(prng_get_bytes)(prng *p, void *dst, size_t len);

/*
 * Get a 64-bit random value from a PRNG.
 */
static inline uint64_t
prng_get_u64(prng *p)
{
	size_t u;

	/*
	 * If there are less than 9 bytes in the buffer, we refill it.
	 * This means that we may drop the last few bytes, but this allows
	 * for faster extraction code. Also, it means that we never leave
	 * an empty buffer.
	 */
	u = p->ptr;
	if (u >= (sizeof p->buf.d) - 9) {
		Zf(prng_refill)(p);
		u = 0;
	}
	p->ptr = u + 8;

	/*
	 * On systems that use little-endian encoding and allow
	 * unaligned accesses, we can simply read the data where it is.
	 */
#if FALCON_LE && FALCON_UNALIGNED  // yyyLEU+1
	return *(uint64_t *)(p->buf.d + u);
#else  // yyyLEU+0
	return (uint64_t)p->buf.d[u + 0]
		| ((uint64_t)p->buf.d[u + 1] << 8)
		| ((uint64_t)p->buf.d[u + 2] << 16)
		| ((uint64_t)p->buf.d[u + 3] << 24)
		| ((uint64_t)p->buf.d[u + 4] << 32)
		| ((uint64_t)p->buf.d[u + 5] << 40)
		| ((uint64_t)p->buf.d[u + 6] << 48)
		| ((uint64_t)p->buf.d[u + 7] << 56);
#endif  // yyyLEU-
}

/*
 * Get an 8-bit random value from a PRNG.
 */
static inline unsigned
prng_get_u8(prng *p)
{
	unsigned v;

	v = p->buf.d[p->ptr ++];
	if (p->ptr == sizeof p->buf.d) {
		Zf(prng_refill)(p);
	}
	return v;
}

/* ==================================================================== */
/*
 * FFT (falcon-fft.c).
 *
 * A real polynomial is represented as an array of N 'fpr' elements.
 * The FFT representation of a real polynomial contains N/2 complex
 * elements; each is stored as two real numbers, for the real and
 * imaginary parts, respectively. See falcon-fft.c for details on the
 * internal representation.
 */

/*
 * Compute FFT in-place: the source array should contain a real
 * polynomial (N coefficients); its storage area is reused to store
 * the FFT representation of that polynomial (N/2 complex numbers).
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void Zf(FFT)(fpr *f, unsigned logn);

/*
 * Compute the inverse FFT in-place: the source array should contain the
 * FFT representation of a real polynomial (N/2 elements); the resulting
 * real polynomial (N coefficients of type 'fpr') is written over the
 * array.
 *
 * 'logn' MUST lie between 1 and 10 (inclusive).
 */
void Zf(iFFT)(fpr *f, unsigned logn);

/*
 * Add polynomial b to polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_add)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Subtract polynomial b from polynomial a. a and b MUST NOT overlap. This
 * function works in both normal and FFT representations.
 */
void Zf(poly_sub)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Negate polynomial a. This function works in both normal and FFT
 * representations.
 */
void Zf(poly_neg)(fpr *a, unsigned logn);

/*
 * Compute adjoint of polynomial a. This function works only in FFT
 * representation.
 */
void Zf(poly_adj_fft)(fpr *a, unsigned logn);

/*
 * Multiply polynomial a with polynomial b. a and b MUST NOT overlap.
 * This function works only in FFT representation.
 */
void Zf(poly_mul_fft)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Multiply polynomial a with the adjoint of polynomial b. a and b MUST NOT
 * overlap. This function works only in FFT representation.
 */
void Zf(poly_muladj_fft)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Multiply polynomial with its own adjoint. This function works only in FFT
 * representation.
 */
void Zf(poly_mulselfadj_fft)(fpr *a, unsigned logn);

/*
 * Multiply polynomial with a real constant. This function works in both
 * normal and FFT representations.
 */
void Zf(poly_mulconst)(fpr *a, fpr x, unsigned logn);

/*
 * Divide polynomial a by polynomial b, modulo X^N+1 (FFT representation).
 * a and b MUST NOT overlap.
 */
void Zf(poly_div_fft)(fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Given f and g (in FFT representation), compute 1/(f*adj(f)+g*adj(g))
 * (also in FFT representation). Since the result is auto-adjoint, all its
 * coordinates in FFT representation are real; as such, only the first N/2
 * values of d[] are filled (the imaginary parts are skipped).
 *
 * Array d MUST NOT overlap with either a or b.
 */
void Zf(poly_invnorm2_fft)(fpr *restrict d,
	const fpr *restrict a, const fpr *restrict b, unsigned logn);

/*
 * Given F, G, f and g (in FFT representation), compute F*adj(f)+G*adj(g)
 * (also in FFT representation). Destination d MUST NOT overlap with
 * any of the source arrays.
 */
void Zf(poly_add_muladj_fft)(fpr *restrict d,
	const fpr *restrict F, const fpr *restrict G,
	const fpr *restrict f, const fpr *restrict g, unsigned logn);

/*
 * Multiply polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void Zf(poly_mul_autoadj_fft)(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Divide polynomial a by polynomial b, where b is autoadjoint. Both
 * a and b are in FFT representation. Since b is autoadjoint, all its
 * FFT coefficients are real, and the array b contains only N/2 elements.
 * a and b MUST NOT overlap.
 */
void Zf(poly_div_autoadj_fft)(fpr *restrict a,
	const fpr *restrict b, unsigned logn);

/*
 * Perform an LDL decomposition of an auto-adjoint matrix G, in FFT
 * representation. On input, g00, g01 and g11 are provided (where the
 * matrix G = [[g00, g01], [adj(g01), g11]]). On output, the d00, l10
 * and d11 values are written in g00, g01 and g11, respectively
 * (with D = [[d00, 0], [0, d11]] and L = [[1, 0], [l10, 1]]).
 * (In fact, d00 = g00, so the g00 operand is left unmodified.)
 */
void Zf(poly_LDL_fft)(const fpr *restrict g00,
	fpr *restrict g01, fpr *restrict g11, unsigned logn);

/*
 * Perform an LDL decomposition of an auto-adjoint matrix G, in FFT
 * representation. This is identical to poly_LDL_fft() except that
 * g00, g01 and g11 are unmodified; the outputs d11 and l10 are written
 * in two other separate buffers provided as extra parameters.
 */
void Zf(poly_LDLmv_fft)(fpr *restrict d11, fpr *restrict l10,
	const fpr *restrict g00, const fpr *restrict g01,
	const fpr *restrict g11, unsigned logn);

/*
 * Apply "split" operation on a polynomial in FFT representation:
 * f = f0(x^2) + x*f1(x^2), for half-size polynomials f0 and f1
 * (polynomials modulo X^(N/2)+1). f0, f1 and f MUST NOT overlap.
 */
void Zf(poly_split_fft)(fpr *restrict f0, fpr *restrict f1,
	const fpr *restrict f, unsigned logn);

/*
 * Apply "merge" operation on two polynomials in FFT representation:
 * given f0 and f1, polynomials moduo X^(N/2)+1, this function computes
 * f = f0(x^2) + x*f1(x^2), in FFT representation modulo X^N+1.
 * f MUST NOT overlap with either f0 or f1.
 */
void Zf(poly_merge_fft)(fpr *restrict f,
	const fpr *restrict f0, const fpr *restrict f1, unsigned logn);

/* ==================================================================== */
/*
 * Key pair generation.
 */

/*
 * Required sizes of the temporary buffer (in bytes).
 *
 * This size is 28*2^logn bytes, except for degrees 2 and 4 (logn = 1
 * or 2) where it is slightly greater.
 */
#define FALCON_KEYGEN_TEMP_1      136
#define FALCON_KEYGEN_TEMP_2      272
#define FALCON_KEYGEN_TEMP_3      224
#define FALCON_KEYGEN_TEMP_4      448
#define FALCON_KEYGEN_TEMP_5      896
#define FALCON_KEYGEN_TEMP_6     1792
#define FALCON_KEYGEN_TEMP_7     3584
#define FALCON_KEYGEN_TEMP_8     7168
#define FALCON_KEYGEN_TEMP_9    14336
#define FALCON_KEYGEN_TEMP_10   28672

/*
 * Generate a new key pair. Randomness is extracted from the provided
 * SHAKE256 context, which must have already been seeded and flipped.
 * The tmp[] array must have suitable size (see FALCON_KEYGEN_TEMP_*
 * macros) and be aligned for the uint32_t, uint64_t and fpr types.
 *
 * The private key elements are written in f, g, F and G, and the
 * public key is written in h. Either or both of G and h may be NULL,
 * in which case the corresponding element is not returned (they can
 * be recomputed from f, g and F).
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(keygen)(inner_shake256_context *rng,
	int8_t *f, int8_t *g, int8_t *F, int8_t *G, uint16_t *h,
	unsigned logn, uint8_t *tmp);

/* ==================================================================== */
/*
 * Signature generation.
 */

/*
 * Expand a private key into the B0 matrix in FFT representation and
 * the LDL tree. All the values are written in 'expanded_key', for
 * a total of (8*logn+40)*2^logn bytes.
 *
 * The tmp[] array must have room for at least 48*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(expand_privkey)(fpr *restrict expanded_key,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
	unsigned logn, uint8_t *restrict tmp);

/*
 * Compute a signature over the provided hashed message (hm); the
 * signature value is one short vector. This function uses an
 * expanded key (as generated by Zf(expand_privkey)()).
 *
 * The sig[] and hm[] buffers may overlap.
 *
 * On successful output, the start of the tmp[] buffer contains the s1
 * vector (as int16_t elements).
 *
 * The minimal size (in bytes) of tmp[] is 48*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(sign_tree)(int16_t *sig, inner_shake256_context *rng,
	const fpr *restrict expanded_key,
	const uint16_t *hm, unsigned logn, uint8_t *tmp);

/*
 * Compute a signature over the provided hashed message (hm); the
 * signature value is one short vector. This function uses a raw
 * key and dynamically recompute the B0 matrix and LDL tree; this
 * saves RAM since there is no needed for an expanded key, but
 * increases the signature cost.
 *
 * The sig[] and hm[] buffers may overlap.
 *
 * On successful output, the start of the tmp[] buffer contains the s1
 * vector (as int16_t elements).
 *
 * The minimal size (in bytes) of tmp[] is 72*2^logn bytes.
 *
 * tmp[] must have 64-bit alignment.
 * This function uses floating-point rounding (see set_fpu_cw()).
 */
void Zf(sign_dyn)(int16_t *sig, inner_shake256_context *rng,
	const int8_t *restrict f, const int8_t *restrict g,
	const int8_t *restrict F, const int8_t *restrict G,
	const uint16_t *hm, unsigned logn, uint8_t *tmp);

/*
 * Internal sampler engine. Exported for tests.
 *
 * sampler_context wraps around a source of random numbers (PRNG) and
 * the sigma_min value (nominally dependent on the degree).
 *
 * sampler() takes as parameters:
 *   ctx      pointer to the sampler_context structure
 *   mu       center for the distribution
 *   isigma   inverse of the distribution standard deviation
 * It returns an integer sampled along the Gaussian distribution centered
 * on mu and of standard deviation sigma = 1/isigma.
 *
 * gaussian0_sampler() takes as parameter a pointer to a PRNG, and
 * returns an integer sampled along a half-Gaussian with standard
 * deviation sigma0 = 1.8205 (center is 0, returned value is
 * nonnegative).
 */

typedef struct {
	prng p;
	fpr sigma_min;
} sampler_context;

TARGET_AVX2
int Zf(sampler)(void *ctx, fpr mu, fpr isigma);

TARGET_AVX2
int Zf(gaussian0_sampler)(prng *p);

/* ==================================================================== */

#endif
#endif


// vrfy.c
/*
 * Falcon signature verification.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

//#include "inner.h"

/* ===================================================================== */
/*
 * Constants for NTT.
 *
 *   n = 2^logn  (2 <= n <= 1024)
 *   phi = X^n + 1
 *   q = 12289
 *   q0i = -1/q mod 2^16
 *   R = 2^16 mod q
 *   R2 = 2^32 mod q
 */

#define Q     12289
#define Q0I   12287
#define R      4091
#define R2    10952

/*
 * Table for NTT, binary case:
 *   GMb[x] = R*(g^rev(x)) mod q
 * where g = 7 (it is a 2048-th primitive root of 1 modulo q)
 * and rev() is the bit-reversal function over 10 bits.
 */
static const uint16_t GMb[] = {
	 4091,  7888, 11060, 11208,  6960,  4342,  6275,  9759,
	 1591,  6399,  9477,  5266,   586,  5825,  7538,  9710,
	 1134,  6407,  1711,   965,  7099,  7674,  3743,  6442,
	10414,  8100,  1885,  1688,  1364, 10329, 10164,  9180,
	12210,  6240,   997,   117,  4783,  4407,  1549,  7072,
	 2829,  6458,  4431,  8877,  7144,  2564,  5664,  4042,
	12189,   432, 10751,  1237,  7610,  1534,  3983,  7863,
	 2181,  6308,  8720,  6570,  4843,  1690,    14,  3872,
	 5569,  9368, 12163,  2019,  7543,  2315,  4673,  7340,
	 1553,  1156,  8401, 11389,  1020,  2967, 10772,  7045,
	 3316, 11236,  5285, 11578, 10637, 10086,  9493,  6180,
	 9277,  6130,  3323,   883, 10469,   489,  1502,  2851,
	11061,  9729,  2742, 12241,  4970, 10481, 10078,  1195,
	  730,  1762,  3854,  2030,  5892, 10922,  9020,  5274,
	 9179,  3604,  3782, 10206,  3180,  3467,  4668,  2446,
	 7613,  9386,   834,  7703,  6836,  3403,  5351, 12276,
	 3580,  1739, 10820,  9787, 10209,  4070, 12250,  8525,
	10401,  2749,  7338, 10574,  6040,   943,  9330,  1477,
	 6865,  9668,  3585,  6633, 12145,  4063,  3684,  7680,
	 8188,  6902,  3533,  9807,  6090,   727, 10099,  7003,
	 6945,  1949,  9731, 10559,  6057,   378,  7871,  8763,
	 8901,  9229,  8846,  4551,  9589, 11664,  7630,  8821,
	 5680,  4956,  6251,  8388, 10156,  8723,  2341,  3159,
	 1467,  5460,  8553,  7783,  2649,  2320,  9036,  6188,
	  737,  3698,  4699,  5753,  9046,  3687,    16,   914,
	 5186, 10531,  4552,  1964,  3509,  8436,  7516,  5381,
	10733,  3281,  7037,  1060,  2895,  7156,  8887,  5357,
	 6409,  8197,  2962,  6375,  5064,  6634,  5625,   278,
	  932, 10229,  8927,  7642,   351,  9298,   237,  5858,
	 7692,  3146, 12126,  7586,  2053, 11285,  3802,  5204,
	 4602,  1748, 11300,   340,  3711,  4614,   300, 10993,
	 5070, 10049, 11616, 12247,  7421, 10707,  5746,  5654,
	 3835,  5553,  1224,  8476,  9237,  3845,   250, 11209,
	 4225,  6326,  9680, 12254,  4136,  2778,   692,  8808,
	 6410,  6718, 10105, 10418,  3759,  7356, 11361,  8433,
	 6437,  3652,  6342,  8978,  5391,  2272,  6476,  7416,
	 8418, 10824, 11986,  5733,   876,  7030,  2167,  2436,
	 3442,  9217,  8206,  4858,  5964,  2746,  7178,  1434,
	 7389,  8879, 10661, 11457,  4220,  1432, 10832,  4328,
	 8557,  1867,  9454,  2416,  3816,  9076,   686,  5393,
	 2523,  4339,  6115,   619,   937,  2834,  7775,  3279,
	 2363,  7488,  6112,  5056,   824, 10204, 11690,  1113,
	 2727,  9848,   896,  2028,  5075,  2654, 10464,  7884,
	12169,  5434,  3070,  6400,  9132, 11672, 12153,  4520,
	 1273,  9739, 11468,  9937, 10039,  9720,  2262,  9399,
	11192,   315,  4511,  1158,  6061,  6751, 11865,   357,
	 7367,  4550,   983,  8534,  8352, 10126,  7530,  9253,
	 4367,  5221,  3999,  8777,  3161,  6990,  4130, 11652,
	 3374, 11477,  1753,   292,  8681,  2806, 10378, 12188,
	 5800, 11811,  3181,  1988,  1024,  9340,  2477, 10928,
	 4582,  6750,  3619,  5503,  5233,  2463,  8470,  7650,
	 7964,  6395,  1071,  1272,  3474, 11045,  3291, 11344,
	 8502,  9478,  9837,  1253,  1857,  6233,  4720, 11561,
	 6034,  9817,  3339,  1797,  2879,  6242,  5200,  2114,
	 7962,  9353, 11363,  5475,  6084,  9601,  4108,  7323,
	10438,  9471,  1271,   408,  6911,  3079,   360,  8276,
	11535,  9156,  9049, 11539,   850,  8617,   784,  7919,
	 8334, 12170,  1846, 10213, 12184,  7827, 11903,  5600,
	 9779,  1012,   721,  2784,  6676,  6552,  5348,  4424,
	 6816,  8405,  9959,  5150,  2356,  5552,  5267,  1333,
	 8801,  9661,  7308,  5788,  4910,   909, 11613,  4395,
	 8238,  6686,  4302,  3044,  2285, 12249,  1963,  9216,
	 4296, 11918,   695,  4371,  9793,  4884,  2411, 10230,
	 2650,   841,  3890, 10231,  7248,  8505, 11196,  6688,
	 4059,  6060,  3686,  4722, 11853,  5816,  7058,  6868,
	11137,  7926,  4894, 12284,  4102,  3908,  3610,  6525,
	 7938,  7982, 11977,  6755,   537,  4562,  1623,  8227,
	11453,  7544,   906, 11816,  9548, 10858,  9703,  2815,
	11736,  6813,  6979,   819,  8903,  6271, 10843,   348,
	 7514,  8339,  6439,   694,   852,  5659,  2781,  3716,
	11589,  3024,  1523,  8659,  4114, 10738,  3303,  5885,
	 2978,  7289, 11884,  9123,  9323, 11830,    98,  2526,
	 2116,  4131, 11407,  1844,  3645,  3916,  8133,  2224,
	10871,  8092,  9651,  5989,  7140,  8480,  1670,   159,
	10923,  4918,   128,  7312,   725,  9157,  5006,  6393,
	 3494,  6043, 10972,  6181, 11838,  3423, 10514,  7668,
	 3693,  6658,  6905, 11953, 10212, 11922,  9101,  8365,
	 5110,    45,  2400,  1921,  4377,  2720,  1695,    51,
	 2808,   650,  1896,  9997,  9971, 11980,  8098,  4833,
	 4135,  4257,  5838,  4765, 10985, 11532,   590, 12198,
	  482, 12173,  2006,  7064, 10018,  3912, 12016, 10519,
	11362,  6954,  2210,   284,  5413,  6601,  3865, 10339,
	11188,  6231,   517,  9564, 11281,  3863,  1210,  4604,
	 8160, 11447,   153,  7204,  5763,  5089,  9248, 12154,
	11748,  1354,  6672,   179,  5532,  2646,  5941, 12185,
	  862,  3158,   477,  7279,  5678,  7914,  4254,   302,
	 2893, 10114,  6890,  9560,  9647, 11905,  4098,  9824,
	10269,  1353, 10715,  5325,  6254,  3951,  1807,  6449,
	 5159,  1308,  8315,  3404,  1877,  1231,   112,  6398,
	11724, 12272,  7286,  1459, 12274,  9896,  3456,   800,
	 1397, 10678,   103,  7420,  7976,   936,   764,   632,
	 7996,  8223,  8445,  7758, 10870,  9571,  2508,  1946,
	 6524, 10158,  1044,  4338,  2457,  3641,  1659,  4139,
	 4688,  9733, 11148,  3946,  2082,  5261,  2036, 11850,
	 7636, 12236,  5366,  2380,  1399,  7720,  2100,  3217,
	10912,  8898,  7578, 11995,  2791,  1215,  3355,  2711,
	 2267,  2004,  8568, 10176,  3214,  2337,  1750,  4729,
	 4997,  7415,  6315, 12044,  4374,  7157,  4844,   211,
	 8003, 10159,  9290, 11481,  1735,  2336,  5793,  9875,
	 8192,   986,  7527,  1401,   870,  3615,  8465,  2756,
	 9770,  2034, 10168,  3264,  6132,    54,  2880,  4763,
	11805,  3074,  8286,  9428,  4881,  6933,  1090, 10038,
	 2567,   708,   893,  6465,  4962, 10024,  2090,  5718,
	10743,   780,  4733,  4623,  2134,  2087,  4802,   884,
	 5372,  5795,  5938,  4333,  6559,  7549,  5269, 10664,
	 4252,  3260,  5917, 10814,  5768,  9983,  8096,  7791,
	 6800,  7491,  6272,  1907, 10947,  6289, 11803,  6032,
	11449,  1171,  9201,  7933,  2479,  7970, 11337,  7062,
	 8911,  6728,  6542,  8114,  8828,  6595,  3545,  4348,
	 4610,  2205,  6999,  8106,  5560, 10390,  9321,  2499,
	 2413,  7272,  6881, 10582,  9308,  9437,  3554,  3326,
	 5991, 11969,  3415, 12283,  9838, 12063,  4332,  7830,
	11329,  6605, 12271,  2044, 11611,  7353, 11201, 11582,
	 3733,  8943,  9978,  1627,  7168,  3935,  5050,  2762,
	 7496, 10383,   755,  1654, 12053,  4952, 10134,  4394,
	 6592,  7898,  7497,  8904, 12029,  3581, 10748,  5674,
	10358,  4901,  7414,  8771,   710,  6764,  8462,  7193,
	 5371,  7274, 11084,   290,  7864,  6827, 11822,  2509,
	 6578,  4026,  5807,  1458,  5721,  5762,  4178,  2105,
	11621,  4852,  8897,  2856, 11510,  9264,  2520,  8776,
	 7011,  2647,  1898,  7039,  5950, 11163,  5488,  6277,
	 9182, 11456,   633, 10046, 11554,  5633,  9587,  2333,
	 7008,  7084,  5047,  7199,  9865,  8997,   569,  6390,
	10845,  9679,  8268, 11472,  4203,  1997,     2,  9331,
	  162,  6182,  2000,  3649,  9792,  6363,  7557,  6187,
	 8510,  9935,  5536,  9019,  3706, 12009,  1452,  3067,
	 5494,  9692,  4865,  6019,  7106,  9610,  4588, 10165,
	 6261,  5887,  2652, 10172,  1580, 10379,  4638,  9949
};

/*
 * Table for inverse NTT, binary case:
 *   iGMb[x] = R*((1/g)^rev(x)) mod q
 * Since g = 7, 1/g = 8778 mod 12289.
 */
static const uint16_t iGMb[] = {
	 4091,  4401,  1081,  1229,  2530,  6014,  7947,  5329,
	 2579,  4751,  6464, 11703,  7023,  2812,  5890, 10698,
	 3109,  2125,  1960, 10925, 10601, 10404,  4189,  1875,
	 5847,  8546,  4615,  5190, 11324, 10578,  5882, 11155,
	 8417, 12275, 10599,  7446,  5719,  3569,  5981, 10108,
	 4426,  8306, 10755,  4679, 11052,  1538, 11857,   100,
	 8247,  6625,  9725,  5145,  3412,  7858,  5831,  9460,
	 5217, 10740,  7882,  7506, 12172, 11292,  6049,    79,
	   13,  6938,  8886,  5453,  4586, 11455,  2903,  4676,
	 9843,  7621,  8822,  9109,  2083,  8507,  8685,  3110,
	 7015,  3269,  1367,  6397, 10259,  8435, 10527, 11559,
	11094,  2211,  1808,  7319,    48,  9547,  2560,  1228,
	 9438, 10787, 11800,  1820, 11406,  8966,  6159,  3012,
	 6109,  2796,  2203,  1652,   711,  7004,  1053,  8973,
	 5244,  1517,  9322, 11269,   900,  3888, 11133, 10736,
	 4949,  7616,  9974,  4746, 10270,   126,  2921,  6720,
	 6635,  6543,  1582,  4868,    42,   673,  2240,  7219,
	 1296, 11989,  7675,  8578, 11949,   989, 10541,  7687,
	 7085,  8487,  1004, 10236,  4703,   163,  9143,  4597,
	 6431, 12052,  2991, 11938,  4647,  3362,  2060, 11357,
	12011,  6664,  5655,  7225,  5914,  9327,  4092,  5880,
	 6932,  3402,  5133,  9394, 11229,  5252,  9008,  1556,
	 6908,  4773,  3853,  8780, 10325,  7737,  1758,  7103,
	11375, 12273,  8602,  3243,  6536,  7590,  8591, 11552,
	 6101,  3253,  9969,  9640,  4506,  3736,  6829, 10822,
	 9130,  9948,  3566,  2133,  3901,  6038,  7333,  6609,
	 3468,  4659,   625,  2700,  7738,  3443,  3060,  3388,
	 3526,  4418, 11911,  6232,  1730,  2558, 10340,  5344,
	 5286,  2190, 11562,  6199,  2482,  8756,  5387,  4101,
	 4609,  8605,  8226,   144,  5656,  8704,  2621,  5424,
	10812,  2959, 11346,  6249,  1715,  4951,  9540,  1888,
	 3764,    39,  8219,  2080,  2502,  1469, 10550,  8709,
	 5601,  1093,  3784,  5041,  2058,  8399, 11448,  9639,
	 2059,  9878,  7405,  2496,  7918, 11594,   371,  7993,
	 3073, 10326,    40, 10004,  9245,  7987,  5603,  4051,
	 7894,   676, 11380,  7379,  6501,  4981,  2628,  3488,
	10956,  7022,  6737,  9933,  7139,  2330,  3884,  5473,
	 7865,  6941,  5737,  5613,  9505, 11568, 11277,  2510,
	 6689,   386,  4462,   105,  2076, 10443,   119,  3955,
	 4370, 11505,  3672, 11439,   750,  3240,  3133,   754,
	 4013, 11929,  9210,  5378, 11881, 11018,  2818,  1851,
	 4966,  8181,  2688,  6205,  6814,   926,  2936,  4327,
	10175,  7089,  6047,  9410, 10492,  8950,  2472,  6255,
	  728,  7569,  6056, 10432, 11036,  2452,  2811,  3787,
	  945,  8998,  1244,  8815, 11017, 11218,  5894,  4325,
	 4639,  3819,  9826,  7056,  6786,  8670,  5539,  7707,
	 1361,  9812,  2949, 11265, 10301,  9108,   478,  6489,
	  101,  1911,  9483,  3608, 11997, 10536,   812,  8915,
	  637,  8159,  5299,  9128,  3512,  8290,  7068,  7922,
	 3036,  4759,  2163,  3937,  3755, 11306,  7739,  4922,
	11932,   424,  5538,  6228, 11131,  7778, 11974,  1097,
	 2890, 10027,  2569,  2250,  2352,   821,  2550, 11016,
	 7769,   136,   617,  3157,  5889,  9219,  6855,   120,
	 4405,  1825,  9635,  7214, 10261, 11393,  2441,  9562,
	11176,   599,  2085, 11465,  7233,  6177,  4801,  9926,
	 9010,  4514,  9455, 11352, 11670,  6174,  7950,  9766,
	 6896, 11603,  3213,  8473,  9873,  2835, 10422,  3732,
	 7961,  1457, 10857,  8069,   832,  1628,  3410,  4900,
	10855,  5111,  9543,  6325,  7431,  4083,  3072,  8847,
	 9853, 10122,  5259, 11413,  6556,   303,  1465,  3871,
	 4873,  5813, 10017,  6898,  3311,  5947,  8637,  5852,
	 3856,   928,  4933,  8530,  1871,  2184,  5571,  5879,
	 3481, 11597,  9511,  8153,    35,  2609,  5963,  8064,
	 1080, 12039,  8444,  3052,  3813, 11065,  6736,  8454,
	 2340,  7651,  1910, 10709,  2117,  9637,  6402,  6028,
	 2124,  7701,  2679,  5183,  6270,  7424,  2597,  6795,
	 9222, 10837,   280,  8583,  3270,  6753,  2354,  3779,
	 6102,  4732,  5926,  2497,  8640, 10289,  6107, 12127,
	 2958, 12287, 10292,  8086,   817,  4021,  2610,  1444,
	 5899, 11720,  3292,  2424,  5090,  7242,  5205,  5281,
	 9956,  2702,  6656,   735,  2243, 11656,   833,  3107,
	 6012,  6801,  1126,  6339,  5250, 10391,  9642,  5278,
	 3513,  9769,  3025,   779,  9433,  3392,  7437,   668,
	10184,  8111,  6527,  6568, 10831,  6482,  8263,  5711,
	 9780,   467,  5462,  4425, 11999,  1205,  5015,  6918,
	 5096,  3827,  5525, 11579,  3518,  4875,  7388,  1931,
	 6615,  1541,  8708,   260,  3385,  4792,  4391,  5697,
	 7895,  2155,  7337,   236, 10635, 11534,  1906,  4793,
	 9527,  7239,  8354,  5121, 10662,  2311,  3346,  8556,
	  707,  1088,  4936,   678, 10245,    18,  5684,   960,
	 4459,  7957,   226,  2451,     6,  8874,   320,  6298,
	 8963,  8735,  2852,  2981,  1707,  5408,  5017,  9876,
	 9790,  2968,  1899,  6729,  4183,  5290, 10084,  7679,
	 7941,  8744,  5694,  3461,  4175,  5747,  5561,  3378,
	 5227,   952,  4319,  9810,  4356,  3088, 11118,   840,
	 6257,   486,  6000,  1342, 10382,  6017,  4798,  5489,
	 4498,  4193,  2306,  6521,  1475,  6372,  9029,  8037,
	 1625,  7020,  4740,  5730,  7956,  6351,  6494,  6917,
	11405,  7487, 10202, 10155,  7666,  7556, 11509,  1546,
	 6571, 10199,  2265,  7327,  5824, 11396, 11581,  9722,
	 2251, 11199,  5356,  7408,  2861,  4003,  9215,   484,
	 7526,  9409, 12235,  6157,  9025,  2121, 10255,  2519,
	 9533,  3824,  8674, 11419, 10888,  4762, 11303,  4097,
	 2414,  6496,  9953, 10554,   808,  2999,  2130,  4286,
	12078,  7445,  5132,  7915,   245,  5974,  4874,  7292,
	 7560, 10539,  9952,  9075,  2113,  3721, 10285, 10022,
	 9578,  8934, 11074,  9498,   294,  4711,  3391,  1377,
	 9072, 10189,  4569, 10890,  9909,  6923,    53,  4653,
	  439, 10253,  7028, 10207,  8343,  1141,  2556,  7601,
	 8150, 10630,  8648,  9832,  7951, 11245,  2131,  5765,
	10343,  9781,  2718,  1419,  4531,  3844,  4066,  4293,
	11657, 11525, 11353,  4313,  4869, 12186,  1611, 10892,
	11489,  8833,  2393,    15, 10830,  5003,    17,   565,
	 5891, 12177, 11058, 10412,  8885,  3974, 10981,  7130,
	 5840, 10482,  8338,  6035,  6964,  1574, 10936,  2020,
	 2465,  8191,   384,  2642,  2729,  5399,  2175,  9396,
	11987,  8035,  4375,  6611,  5010, 11812,  9131, 11427,
	  104,  6348,  9643,  6757, 12110,  5617, 10935,   541,
	  135,  3041,  7200,  6526,  5085, 12136,   842,  4129,
	 7685, 11079,  8426,  1008,  2725, 11772,  6058,  1101,
	 1950,  8424,  5688,  6876, 12005, 10079,  5335,   927,
	 1770,   273,  8377,  2271,  5225, 10283,   116, 11807,
	   91, 11699,   757,  1304,  7524,  6451,  8032,  8154,
	 7456,  4191,   309,  2318,  2292, 10393, 11639,  9481,
	12238, 10594,  9569,  7912, 10368,  9889, 12244,  7179,
	 3924,  3188,   367,  2077,   336,  5384,  5631,  8596,
	 4621,  1775,  8866,   451,  6108,  1317,  6246,  8795,
	 5896,  7283,  3132, 11564,  4977, 12161,  7371,  1366,
	12130, 10619,  3809,  5149,  6300,  2638,  4197,  1418,
	10065,  4156,  8373,  8644, 10445,   882,  8158, 10173,
	 9763, 12191,   459,  2966,  3166,   405,  5000,  9311,
	 6404,  8986,  1551,  8175,  3630, 10766,  9265,   700,
	 8573,  9508,  6630, 11437, 11595,  5850,  3950,  4775,
	11941,  1446,  6018,  3386, 11470,  5310,  5476,   553,
	 9474,  2586,  1431,  2741,   473, 11383,  4745,   836,
	 4062, 10666,  7727, 11752,  5534,   312,  4307,  4351,
	 5764,  8679,  8381,  8187,     5,  7395,  4363,  1152,
	 5421,  5231,  6473,   436,  7567,  8603,  6229,  8230
};

/*
 * Reduce a small signed integer modulo q. The source integer MUST
 * be between -q/2 and +q/2.
 */
static inline uint32_t
mq_conv_small(int x)
{
	/*
	 * If x < 0, the cast to uint32_t will set the high bit to 1.
	 */
	uint32_t y;

	y = (uint32_t)x;
	y += Q & -(y >> 31);
	return y;
}

/*
 * Addition modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	/*
	 * We compute x + y - q. If the result is negative, then the
	 * high bit will be set, and 'd >> 31' will be equal to 1;
	 * thus '-(d >> 31)' will be an all-one pattern. Otherwise,
	 * it will be an all-zero pattern. In other words, this
	 * implements a conditional addition of q.
	 */
	uint32_t d;

	d = x + y - Q;
	d += Q & -(d >> 31);
	return d;
}

/*
 * Subtraction modulo q. Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/*
	 * As in mq_add(), we use a conditional addition to ensure the
	 * result is in the 0..q-1 range.
	 */
	uint32_t d;

	d = x - y;
	d += Q & -(d >> 31);
	return d;
}

/*
 * Division by 2 modulo q. Operand must be in the 0..q-1 range.
 */
static inline uint32_t
mq_rshift1(uint32_t x)
{
	x += Q & -(x & 1);
	return (x >> 1);
}

/*
 * Montgomery multiplication modulo q. If we set R = 2^16 mod q, then
 * this function computes: x * y / R mod q
 * Operands must be in the 0..q-1 range.
 */
static inline uint32_t
mq_montymul(uint32_t x, uint32_t y)
{
	uint32_t z, w;

	/*
	 * We compute x*y + k*q with a value of k chosen so that the 16
	 * low bits of the result are 0. We can then shift the value.
	 * After the shift, result may still be larger than q, but it
	 * will be lower than 2*q, so a conditional subtraction works.
	 */

	z = x * y;
	w = ((z * Q0I) & 0xFFFF) * Q;

	/*
	 * When adding z and w, the result will have its low 16 bits
	 * equal to 0. Since x, y and z are lower than q, the sum will
	 * be no more than (2^15 - 1) * q + (q - 1)^2, which will
	 * fit on 29 bits.
	 */
	z = (z + w) >> 16;

	/*
	 * After the shift, analysis shows that the value will be less
	 * than 2q. We do a subtraction then conditional subtraction to
	 * ensure the result is in the expected range.
	 */
	z -= Q;
	z += Q & -(z >> 31);
	return z;
}

#if 0
/*
 * Montgomery squaring (computes (x^2)/R).
 */
static inline uint32_t
mq_montysqr(uint32_t x)
{
	return mq_montymul(x, x);
}

/*
 * Divide x by y modulo q = 12289.
 */
static inline uint32_t
mq_div_12289(uint32_t x, uint32_t y)
{
	/*
	 * We invert y by computing y^(q-2) mod q.
	 *
	 * We use the following addition chain for exponent e = 12287:
	 *
	 *   e0 = 1
	 *   e1 = 2 * e0 = 2
	 *   e2 = e1 + e0 = 3
	 *   e3 = e2 + e1 = 5
	 *   e4 = 2 * e3 = 10
	 *   e5 = 2 * e4 = 20
	 *   e6 = 2 * e5 = 40
	 *   e7 = 2 * e6 = 80
	 *   e8 = 2 * e7 = 160
	 *   e9 = e8 + e2 = 163
	 *   e10 = e9 + e8 = 323
	 *   e11 = 2 * e10 = 646
	 *   e12 = 2 * e11 = 1292
	 *   e13 = e12 + e9 = 1455
	 *   e14 = 2 * e13 = 2910
	 *   e15 = 2 * e14 = 5820
	 *   e16 = e15 + e10 = 6143
	 *   e17 = 2 * e16 = 12286
	 *   e18 = e17 + e0 = 12287
	 *
	 * Additions on exponents are converted to Montgomery
	 * multiplications. We define all intermediate results as so
	 * many local variables, and let the C compiler work out which
	 * must be kept around.
	 */
	uint32_t y0, y1, y2, y3, y4, y5, y6, y7, y8, y9;
	uint32_t y10, y11, y12, y13, y14, y15, y16, y17, y18;

	y0 = mq_montymul(y, R2);
	y1 = mq_montysqr(y0);
	y2 = mq_montymul(y1, y0);
	y3 = mq_montymul(y2, y1);
	y4 = mq_montysqr(y3);
	y5 = mq_montysqr(y4);
	y6 = mq_montysqr(y5);
	y7 = mq_montysqr(y6);
	y8 = mq_montysqr(y7);
	y9 = mq_montymul(y8, y2);
	y10 = mq_montymul(y9, y8);
	y11 = mq_montysqr(y10);
	y12 = mq_montysqr(y11);
	y13 = mq_montymul(y12, y9);
	y14 = mq_montysqr(y13);
	y15 = mq_montysqr(y14);
	y16 = mq_montymul(y15, y10);
	y17 = mq_montysqr(y16);
	y18 = mq_montymul(y17, y0);

	/*
	 * Final multiplication with x, which is not in Montgomery
	 * representation, computes the correct division result.
	 */
	return mq_montymul(y18, x);
}
#endif

/*
 * Compute NTT on a ring element.
 */
static void
mq_NTT(uint16_t *a, unsigned logn)
{
	size_t n, t, m;

	n = (size_t)1 << logn;
	t = n;
	for (m = 1; m < n; m <<= 1) {
		size_t ht, i, j1;

		ht = t >> 1;
		for (i = 0, j1 = 0; i < m; i ++, j1 += t) {
			size_t j, j2;
			uint32_t s;

			s = GMb[m + i];
			j2 = j1 + ht;
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = a[j];
				v = mq_montymul(a[j + ht], s);
				a[j] = (uint16_t)mq_add(u, v);
				a[j + ht] = (uint16_t)mq_sub(u, v);
			}
		}
		t = ht;
	}
}

/*
 * Compute the inverse NTT on a ring element, binary case.
 */
static void
mq_iNTT(uint16_t *a, unsigned logn)
{
	size_t n, t, m;
	uint32_t ni;

	n = (size_t)1 << logn;
	t = 1;
	m = n;
	while (m > 1) {
		size_t hm, dt, i, j1;

		hm = m >> 1;
		dt = t << 1;
		for (i = 0, j1 = 0; i < hm; i ++, j1 += dt) {
			size_t j, j2;
			uint32_t s;

			j2 = j1 + t;
			s = iGMb[hm + i];
			for (j = j1; j < j2; j ++) {
				uint32_t u, v, w;

				u = a[j];
				v = a[j + t];
				a[j] = (uint16_t)mq_add(u, v);
				w = mq_sub(u, v);
				a[j + t] = (uint16_t)
					mq_montymul(w, s);
			}
		}
		t = dt;
		m = hm;
	}

	/*
	 * To complete the inverse NTT, we must now divide all values by
	 * n (the vector size). We thus need the inverse of n, i.e. we
	 * need to divide 1 by 2 logn times. But we also want it in
	 * Montgomery representation, i.e. we also want to multiply it
	 * by R = 2^16. In the common case, this should be a simple right
	 * shift. The loop below is generic and works also in corner cases;
	 * its computation time is negligible.
	 */
	ni = R;
	for (m = n; m > 1; m >>= 1) {
		ni = mq_rshift1(ni);
	}
	for (m = 0; m < n; m ++) {
		a[m] = (uint16_t)mq_montymul(a[m], ni);
	}
}

/*
 * Convert a polynomial (mod q) to Montgomery representation.
 */
static void
mq_poly_tomonty(uint16_t *f, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_montymul(f[u], R2);
	}
}

/*
 * Multiply two polynomials together (NTT representation, and using
 * a Montgomery multiplication). Result f*g is written over f.
 */
static void
mq_poly_montymul_ntt(uint16_t *f, const uint16_t *g, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_montymul(f[u], g[u]);
	}
}

/*
 * Subtract polynomial g from polynomial f.
 */
static void
mq_poly_sub(uint16_t *f, const uint16_t *g, unsigned logn)
{
	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		f[u] = (uint16_t)mq_sub(f[u], g[u]);
	}
}

/* ===================================================================== */

/* see inner.h */
void
Zf(to_ntt_monty)(uint16_t *h, unsigned logn)
{
	mq_NTT(h, logn);
	mq_poly_tomonty(h, logn);
}

#if 0
/* see inner.h */
int
Zf(verify_raw)(const uint16_t *c0, const int16_t *s2,
	const uint16_t *h, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;

	/*
	 * Reduce s2 elements modulo q ([0..q-1] range).
	 */
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;
	}

	/*
	 * Compute -s1 = s2*h - c0 mod phi mod q (in tt[]).
	 */
	mq_NTT(tt, logn);
	mq_poly_montymul_ntt(tt, h, logn);
	mq_iNTT(tt, logn);
	mq_poly_sub(tt, c0, logn);

	/*
	 * Normalize -s1 elements into the [-q/2..q/2] range.
	 */
	for (u = 0; u < n; u ++) {
		int32_t w;

		w = (int32_t)tt[u];
		w -= (int32_t)(Q & -(((Q >> 1) - (uint32_t)w) >> 31));
		((int16_t *)tt)[u] = (int16_t)w;
	}

	/*
	 * Signature is valid if and only if the aggregate (-s1,s2) vector
	 * is short enough.
	 */
	return Zf(is_short)((int16_t *)tt, s2, logn);
}

/* see inner.h */
int
Zf(compute_public)(uint16_t *h,
	const int8_t *f, const int8_t *g, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		tt[u] = (uint16_t)mq_conv_small(f[u]);
		h[u] = (uint16_t)mq_conv_small(g[u]);
	}
	mq_NTT(h, logn);
	mq_NTT(tt, logn);
	for (u = 0; u < n; u ++) {
		if (tt[u] == 0) {
			return 0;
		}
		h[u] = (uint16_t)mq_div_12289(h[u], tt[u]);
	}
	mq_iNTT(h, logn);
	return 1;
}

/* see inner.h */
int
Zf(complete_private)(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *t1, *t2;

	n = (size_t)1 << logn;
	t1 = (uint16_t *)tmp;
	t2 = t1 + n;
	for (u = 0; u < n; u ++) {
		t1[u] = (uint16_t)mq_conv_small(g[u]);
		t2[u] = (uint16_t)mq_conv_small(F[u]);
	}
	mq_NTT(t1, logn);
	mq_NTT(t2, logn);
	mq_poly_tomonty(t1, logn);
	mq_poly_montymul_ntt(t1, t2, logn);
	for (u = 0; u < n; u ++) {
		t2[u] = (uint16_t)mq_conv_small(f[u]);
	}
	mq_NTT(t2, logn);
	for (u = 0; u < n; u ++) {
		if (t2[u] == 0) {
			return 0;
		}
		t1[u] = (uint16_t)mq_div_12289(t1[u], t2[u]);
	}
	mq_iNTT(t1, logn);
	for (u = 0; u < n; u ++) {
		uint32_t w;
		int32_t gi;

		w = t1[u];
		w -= (Q & ~-((w - (Q >> 1)) >> 31));
		gi = *(int32_t *)&w;
		if (gi < -127 || gi > +127) {
			return 0;
		}
		G[u] = (int8_t)gi;
	}
	return 1;
}

/* see inner.h */
int
Zf(is_invertible)(
	const int16_t *s2, unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;
	uint32_t r;

	n = (size_t)1 << logn;
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;
	}
	mq_NTT(tt, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		r |= (uint32_t)(tt[u] - 1);
	}
	return (int)(1u - (r >> 31));
}

/* see inner.h */
int
Zf(verify_recover)(uint16_t *h,
	const uint16_t *c0, const int16_t *s1, const int16_t *s2,
	unsigned logn, uint8_t *tmp)
{
	size_t u, n;
	uint16_t *tt;
	uint32_t r;

	n = (size_t)1 << logn;

	/*
	 * Reduce elements of s1 and s2 modulo q; then write s2 into tt[]
	 * and c0 - s1 into h[].
	 */
	tt = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u];
		w += Q & -(w >> 31);
		tt[u] = (uint16_t)w;

		w = (uint32_t)s1[u];
		w += Q & -(w >> 31);
		w = mq_sub(c0[u], w);
		h[u] = (uint16_t)w;
	}

	/*
	 * Compute h = (c0 - s1) / s2. If one of the coefficients of s2
	 * is zero (in NTT representation) then the operation fails. We
	 * keep that information into a flag so that we do not deviate
	 * from strict constant-time processing; if all coefficients of
	 * s2 are non-zero, then the high bit of r will be zero.
	 */
	mq_NTT(tt, logn);
	mq_NTT(h, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		r |= (uint32_t)(tt[u] - 1);
		h[u] = (uint16_t)mq_div_12289(h[u], tt[u]);
	}
	mq_iNTT(h, logn);

	/*
	 * Signature is acceptable if and only if it is short enough,
	 * and s2 was invertible mod phi mod q. The caller must still
	 * check that the rebuilt public key matches the expected
	 * value (e.g. through a hash).
	 */
	r = ~r & (uint32_t)-Zf(is_short)(s1, s2, logn);
	return (int)(r >> 31);
}

/* see inner.h */
int
Zf(count_nttzero)(const int16_t *sig, unsigned logn, uint8_t *tmp)
{
	uint16_t *s2;
	size_t u, n;
	uint32_t r;

	n = (size_t)1 << logn;
	s2 = (uint16_t *)tmp;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)sig[u];
		w += Q & -(w >> 31);
		s2[u] = (uint16_t)w;
	}
	mq_NTT(s2, logn);
	r = 0;
	for (u = 0; u < n; u ++) {
		uint32_t w;

		w = (uint32_t)s2[u] - 1u;
		r += (w >> 31);
	}
	return (int)r;
}
#endif

#endif


#define LOGN 4

#ifndef LACK_OF_MEMORY
int16_t buf[1 << LOGN];
#endif
    
void testFalconNTT(void)
{
#ifndef LACK_OF_MEMORY
  for(int32_t i = 0; i < (1 << LOGN); i++)
    buf[i] = i/*montgomery_reduce(i)*/;

  mq_NTT(buf, LOGN);

  mq_iNTT(buf, LOGN);

  for(int32_t i = 0; i < (1 << LOGN); i++)
    ASSERT(buf[i] == i);
#endif
}

