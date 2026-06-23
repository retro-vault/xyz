/*
 * iso646.h
 *
 * Alternative operator spellings for the xcc Z80 target.
 *
 * This header is unchanged in spirit from earlier C standards: it simply
 * provides macro aliases for the punctuation tokens that may be awkward to
 * type on restricted character-set keyboards.
 *
 * MIT License (see: LICENSE)
 * Copyright (C) 2026 tomaz stih
 */
#ifndef _ISO646_H
#define _ISO646_H

#define and     &&
#define and_eq  &=
#define bitand  &
#define bitor   |
#define compl   ~
#define not     !
#define not_eq  !=
#define or      ||
#define or_eq   |=
#define xor     ^
#define xor_eq  ^=

#endif /* _ISO646_H */
