#include <fcntl.h>
#include <unistd.h>

/* Upstream predates prototypes and uses POSIX open(path, flags, mode).
 * The X emulation backend does not use permission modes. */
#define open(path, flags, mode) open((path), (flags))
#include "../upstream/md5/md5sum.c"

