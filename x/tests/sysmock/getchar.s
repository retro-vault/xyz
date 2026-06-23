        ;; getchar.s  (sys backend: sim)
        ;;
        ;; Test-facing public getchar wrapper.  The buffered implementation is
        ;; kept in __sys_getchar so read() and getchar() share the same stream.

        .module getchar
        .optsdcc -mz80 sdcccall(1)

        .globl  _getchar
        .globl  __sys_getchar

        .area   _CODE
_getchar::
        jp      __sys_getchar
