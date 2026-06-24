// C23: enum with explicit underlying type.
enum Status : unsigned char { OK = 0, ERR = 1, BUSY = 2 };
enum Flags  : unsigned int  { NONE = 0, READY = 1, DONE = 2 };

enum Status get_status(void) { return OK; }
int is_ready(enum Flags f) { return (int)(f & READY); }
