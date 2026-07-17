/* Keep the upstream program intact while giving its interactive main a
 * private name.  The execution harness supplies a deterministic main. */
#define main static inline umchess_interactive_main
#include "upstream/umchess.c"
#undef main
