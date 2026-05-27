/*
   bug-3920.c
   A bug in the interaction of the generation for temporary variables for postfix operators with inlining.
   And a related one affecting compiund iterals in inlined functions.
 */
 
#include <testfwk.h>

char *p;

extern void ABSORB(unsigned char* s, const unsigned char* d);

inline void ABSORB(unsigned char* s, const unsigned char* d) {
  *s++ ^= *d;
}

void ascon_update(unsigned char* m) {
  ABSORB(p, m);
}

void
testBug(void)
{
  char a = 0x5a;
  p = &a;
  ascon_update(p);
  ASSERT(a == 0x00);
}

typedef union {
  unsigned char x[1];
} ascon_state_t;

void P(ascon_state_t* s, int nr);

extern void ascon_inithash(ascon_state_t* s);

inline void ascon_inithash(ascon_state_t* s) {
  *s = (ascon_state_t){{0x9b}};
}

int ascon_xof(void) {
  ascon_state_t s;
  ascon_inithash(&s);
  P(&s, 12);
  return 0;
}

void
testBug2(void)
{
  ASSERT (!ascon_xof());
}

void P(ascon_state_t* s, int nr)
{
  ASSERT(s->x[nr - 12] == 0x9b);
}

