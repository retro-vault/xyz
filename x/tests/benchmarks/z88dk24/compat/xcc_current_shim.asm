; Current z88dk already exports XCC's double-underscore HL trampoline and
; rewrites ___printf_sd itself.  Only the IY spelling still needs an alias.
PUBLIC __sdcc_call_iy
EXTERN ___sdcc_call_iy
defc __sdcc_call_iy = ___sdcc_call_iy
