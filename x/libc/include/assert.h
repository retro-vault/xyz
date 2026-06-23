/*
 * assert.h
 *
 * Standard C assertion support for the xcc Z80 target.
 *
 * This implementation preserves the failing expression text and source
 * location in internal globals, then halts forever. That gives us useful
 * debugger state today, even though stderr and abort() are not implemented in
 * this libc yet.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _ASSERT_H
#define _ASSERT_H

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define __STDC_VERSION_ASSERT_H__ 202311L
#endif

/* Internal failure sink used by the assert macro when checks are enabled. */
_Noreturn extern void __assert_fail(const char *expr,
                                    const char *file,
                                    int         line,
                                    const char *func);

/* Shared worker used by both the classic and C23 assert spellings. */
#define __assert_impl(condition_text, condition_value) \
    ((condition_value) ? (void)0 \
                       : __assert_fail((condition_text), __FILE__, __LINE__, __func__))

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  ifdef NDEBUG
#    define assert(...) ((void)0)
#  else
#    define __assert_dispatch(condition) __assert_impl(#condition, (condition))
#    define assert(...) __assert_dispatch((__VA_ARGS__))
#  endif
#else
#  ifdef NDEBUG
#    define assert(condition) ((void)0)
#  else
#    define assert(condition) __assert_impl(#condition, (condition))
#  endif
#endif

#endif /* _ASSERT_H */
