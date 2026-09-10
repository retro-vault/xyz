        ; RST 18h bridge for named-service lookup.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2021, 2026 tomaz stih

        .module svc_query_rst18
        .optsdcc -mz80 sdcccall(1)

        .globl  _svc_query_rst18
        .globl  __svc_query

        .area   _CODE

_svc_query_rst18::
        jp      __svc_query
