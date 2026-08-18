/*
 * conio.h
 *
 * ZX Spectrum console extensions.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef XCC_CONIO_H
#define XCC_CONIO_H

/* Return zero when no key is available, nonzero otherwise. */
[[sdcc::sdccall(1)]] extern int kbhit(void);

#endif /* XCC_CONIO_H */
