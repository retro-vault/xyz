        ;; free_aligned_sized.s
        ;; C23 free_aligned_sized(ptr, alignment, size). The current heap keeps
        ;; allocation metadata, so alignment/size are validation hints only.

        .module free_aligned_sized
        .optsdcc -mz80 sdcccall(1)

        .globl  _free_aligned_sized
        .globl  _free

        .area   _CODE
_free_aligned_sized::
        ld      a,h
        or      l
        ret     z
        jp      _free
