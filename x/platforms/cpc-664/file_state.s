        ; AMSDOS Cassette Manager channel state and two 2 KiB buffers.
        ; The firmware exposes one input and one output stream at a time.
        ;
        ; MIT License (see: LICENSE)
        ; Copyright (C) 2026 tomaz stih

        .module file_state

        .globl  __cpc_input_open
        .globl  __cpc_output_open
        .globl  __cpc_input_length
        .globl  __cpc_input_length_known
        .globl  __cpc_input_position
        .globl  __cpc_input_name_length
        .globl  __cpc_input_name
        .globl  __cpc_input_buffer
        .globl  __cpc_output_buffer

        .area   _BSS
__cpc_input_open::
        .ds     1
__cpc_output_open::
        .ds     1
__cpc_input_length::
        .ds     2
__cpc_input_length_known::
        .ds     1
__cpc_input_position::
        .ds     2
__cpc_input_name_length::
        .ds     1
__cpc_input_name::
        .ds     16
__cpc_input_buffer::
        .ds     2048
__cpc_output_buffer::
        .ds     2048
