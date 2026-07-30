        ; tls_template.s
        ;
        ; Link-time TLS template supplied by the compiler in ordinary C
        ; programs.  The direct runtime harness force-links every runtime
        ; object, so it provides one byte here to exercise __tls_base.

        .module runtime_test_tls_template
        .area   _DATA
        .globl  __tls_template

__tls_template::
        .db     0x5a
