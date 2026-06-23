#include <stdio.h>
#include "kbarena.h"

int main(void)
{
	int i;
	void *ba;
	ba = kba_init(1000);
	for (i = 0; i < 100; ++i)
		kba_alloc(ba, 23, 1);
	kba_save(ba);
	for (i = 0; i < 100; ++i)
		kba_alloc(ba, 29, 1);
	kba_restore(ba);
	for (i = 0; i < 100; ++i)
		kba_alloc(ba, 29, 1);
	kba_destroy(ba);
	return 0;
}
