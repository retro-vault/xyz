        ;; string_fold_lower.s
        ;; Split from string_common.s to keep archive granularity
        ;; one public routine per module (prevents overlinking).

        .module string_fold_lower
        .optsdcc -mz80 sdcccall(1)

        .globl  __string_fold_lower

        .area   _CODE
__string_fold_lower::
        cp      #0x41                   ; < 'A' ?
        jr      c,__string_fold_lower_done
        cp      #0x5b                   ; > 'Z' ?  ('Z'+1 = 0x5B)
        jr      nc,__string_fold_lower_done
        add     a,#0x20                 ; uppercase -> lowercase
__string_fold_lower_done:
        ret
