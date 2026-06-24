        ;; putchar.s  (sys backend: sim)
        ;;
        ;; Test-facing public putchar wrapper.  The capture implementation is
        ;; kept in __sys_putchar so read/write can share it; libc entry points
        ;; that call _putchar directly should still hit the same buffer.

        .module putchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _putchar
        .globl  __sys_putchar

        .area   _CODE
_putchar::
        jp      __sys_putchar
