#ifndef AC_KBARENA_H
#define AC_KBARENA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize a blocked arena
 *
 * @param blen      block length in bytes
 *
 * @return pointer to a blocked arena
 */
void *kba_init(unsigned blen);

/** Destroy a blocked arena */
void kba_destroy(void *kba);

/**
 * Allocate memory from a blocked arena
 *
 * @param len       size of the memory to allocate
 * @param aln       alignment; must be power of 2
 *
 * @return pointer to the memory on success; NULL on insufficient memory or
 * when len is longer than block length
 */
void *kba_alloc(void *kba, unsigned len, unsigned aln);

/** Save the state */
int kba_save(void *kba);

/** Restore the state (effectively free all memory allocated after the pairing save) */
int kba_restore(void *kba);

/** Get the capacity in byte */
size_t kba_capacity(const void *ba_);

#ifdef __cplusplus
}
#endif

#endif
