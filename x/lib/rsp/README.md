# librsp

`librsp` is the reusable GDB Remote Serial Protocol transport library used by
the host-side debugger tools.

Public headers live in [lib/rsp/include/rsp](/home/tstih/data/retro-vault/xyz/lib/rsp/include/rsp):

- [rsp.h](/home/tstih/data/retro-vault/xyz/lib/rsp/include/rsp/rsp.h)

Typical use:

```cpp
#include <rsp/rsp.h>
```

The build produces a static archive at `bin/lib/librsp.a`.
