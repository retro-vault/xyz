        ;; Core YOS process bindings.

        .module yos_system
        .optsdcc -mz80 sdcccall(1)

        .globl  _query_service
        .globl  _yos_get_api
        .globl  _yos_api_table
        .globl  __exit
        .globl  ___sdcc_call_bc

        .area   _BSS
_yos_api_table::
        .ds     2

        .area   _CODE

        ;; void *query_service(const char *name)
        ;;   HL = name, DE = interface or NULL.
_query_service::
        rst     0x18
        ret

        ;; yos_t *yos_get_api(void)
_yos_get_api::
        ld      de,(_yos_api_table)
        ret

        ;; _exit/exit process termination. Status is currently advisory: ABI 1
        ;; exposes process_exit(void), so there is no status channel yet.
__exit::
        ld      hl,(_yos_api_table)
        ld      a,h
        or      l
        jr      z,.halt
        ld      de,#32                 ; yos_t.exit_process
        add     hl,de
        ld      c,(hl)
        inc     hl
        ld      b,(hl)
        jp      ___sdcc_call_bc
.halt:
        di
.halt_loop:
        halt
        jr      .halt_loop
