        ;; putwc.s
        ;;
        ;; putwc is the stream-parameter spelling of fputwc. Both signatures
        ;; pass the wide code unit in HL and the FILE* in DE under sdcccall(1),
        ;; so the implementation can tail-call the shared worker directly.

        .module putwc
        .optsdcc -mz80 sdcccall(1)

        .globl  _putwc
        .globl  _fputwc

        .area   _CODE

_putwc::
        jp      _fputwc
