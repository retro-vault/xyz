#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "kbarena.h"

#define kom_malloc(type, cnt)       ((type*)malloc((cnt) * sizeof(type)))
#define kom_calloc(type, cnt)       ((type*)calloc((cnt), sizeof(type)))
#define kom_realloc(type, ptr, cnt) ((type*)realloc((ptr), (cnt) * sizeof(type)))

#define kom_grow(type, ptr, __i, __m, __succ) do { /* make enough room for ptr[__i] */ \
		*(__succ) = 1; \
		if ((__i) >= (__m)) { \
			size_t new_m = (__i) + 16ULL + (((__i) + 1ULL) >> 1); \
			type *new_p; \
			new_p = kom_realloc(type, (ptr), new_m); \
			if (new_p) (__m) = new_m, (ptr) = new_p; \
			else *(__succ) = 0; \
		} \
	} while (0)

typedef struct {
	uint32_t blen, max_blk; // block length and max number of blocks
	uint32_t bi, bo; // current block index and block offset
	uint8_t **b; // blocks
	uint32_t n_stack, m_stack; // stack size and capacity
	uint64_t *stack; // saved states (two uint32_t packed together)
} kbarena_t;

void *kba_init(uint32_t blen)
{
	kbarena_t *ba;
	if (blen == 0) return 0;
	ba = kom_calloc(kbarena_t, 1);
	if (ba == 0) return 0;
	ba->blen = (blen + 7) / 8 * 8; // round up to 8 bytes
	ba->max_blk = 4;
	ba->b = kom_calloc(uint8_t*, ba->max_blk);
	ba->b[0] = kom_malloc(uint8_t, ba->blen); // TODO: use aligned alloc
	return ba;
}

void kba_destroy(void *ba_)
{
	kbarena_t *ba = (kbarena_t*)ba_;
	uint32_t i;
	if (ba == 0) return;
	for (i = 0; i < ba->max_blk; ++i)
		if (ba->b[i]) free(ba->b[i]);
	free(ba->b); free(ba->stack); free(ba);
}

size_t kba_capacity(const void *ba_)
{
	const kbarena_t *ba = (const kbarena_t*)ba_;
	uint32_t i;
	size_t cap = 0;
	for (i = 0; i < ba->max_blk; ++i)
		if (ba->b[i]) cap += ba->blen;
	return cap;
}

void *kba_alloc(void *ba_, uint32_t sz, uint32_t aln)
{
	kbarena_t *ba = (kbarena_t*)ba_;
	uint32_t pad = -(int64_t)ba->bo & (aln - 1);
	void *p;
	if (sz == 0 || sz > ba->blen || (aln & (aln - 1)) != 0) return 0;
	if ((uint64_t)ba->bo + sz + pad > ba->blen) { // no room in the current block
		uint32_t old_max = ba->max_blk;
		int succ;
		kom_grow(uint8_t*, ba->b, ba->bi + 1, ba->max_blk, &succ);
		if (!succ) return 0;
		if (ba->max_blk > old_max)
			memset(&ba->b[old_max], 0, (ba->max_blk - old_max) * sizeof(void*));
		if (ba->b[ba->bi + 1] == 0) {
			ba->b[ba->bi + 1] = kom_malloc(uint8_t, ba->blen); // TODO: use aligned malloc
			if (ba->b[ba->bi + 1] == 0) return 0;
		}
		++ba->bi, ba->bo = 0, pad = 0;
	}
	p = (void*)&ba->b[ba->bi][ba->bo + pad];
	ba->bo += sz + pad;
	return p;
}

int kba_save(void *ba_)
{
	kbarena_t *ba = (kbarena_t*)ba_;
	int succ;
	kom_grow(uint64_t, ba->stack, ba->n_stack, ba->m_stack, &succ);
	if (!succ) return -1;
	ba->stack[ba->n_stack++] = (uint64_t)ba->bi << 32 | ba->bo;
	return 0;
}

int kba_restore(void *ba_)
{
	uint64_t x;
	kbarena_t *ba = (kbarena_t*)ba_;
	if (ba->n_stack == 0) return -1;
	x = ba->stack[--ba->n_stack];
	ba->bi = x>>32, ba->bo = (uint32_t)x;
	return 0;
}
