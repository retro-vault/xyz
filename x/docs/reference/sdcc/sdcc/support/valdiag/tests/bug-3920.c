/* bug-3889.c

   An issue about compound literals in inlined functions resulting in compilation failures and invalid warnings. 
 */

#ifdef TEST1
typedef union {
  unsigned char x[1];
} ascon_state_t;

void P(ascon_state_t* s, int nr);

inline void ascon_inithash(ascon_state_t* s) {
  *s = (ascon_state_t){{0x9b}};
}

int ascon_xof(void) {
  ascon_state_t s;
  ascon_inithash(&s);
  P(&s, 12);
  return 0;
}
#endif

