#ifndef _STDATOMIC_H
#define _STDATOMIC_H

#define ATOMIC_BOOL_LOCK_FREE      0
#define ATOMIC_CHAR_LOCK_FREE      0
#define ATOMIC_CHAR16_T_LOCK_FREE  0
#define ATOMIC_CHAR32_T_LOCK_FREE  0
#define ATOMIC_WCHAR_T_LOCK_FREE   0
#define ATOMIC_SHORT_LOCK_FREE     0
#define ATOMIC_INT_LOCK_FREE       0
#define ATOMIC_LONG_LOCK_FREE      0
#define ATOMIC_LLONG_LOCK_FREE     0
#define ATOMIC_POINTER_LOCK_FREE   0

#define ATOMIC_VAR_INIT(val) (val)
#define atomic_init(obj, val) (*(obj) = (val))

#endif
