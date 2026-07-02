/*
 * stdnoreturn.h
 *
 * Legacy noreturn convenience macro for the xcc Z80 target.
 *
 * This header remains part of C23, although it is deprecated there. It is
 * still useful for portable code that wants the older noreturn spelling while
 * mapping directly to the core _Noreturn keyword.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _STDNORETURN_H
#define _STDNORETURN_H

#define noreturn _Noreturn

#endif /* _STDNORETURN_H */
