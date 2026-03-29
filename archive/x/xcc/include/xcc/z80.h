/**
 * Common definitions and configuration options for the Z80 backend.
 *
 * Defines target configuration for the Z80 CPU and utility macros
 * used to adapt code generation logic based on backend options.
 */

#pragma once

#include <xcc/options.h> /* for global compiler options */

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        SUB_Z80
    } Z80_SUB_PORT;

    typedef struct
    {
        Z80_SUB_PORT sub;
        int calleeSavesBC;
        int port_mode;
        int port_back;
        int reserveIY;
        int noOmitFramePtr;
        int legacyBanking;
        int nmosZ80;
    } Z80_OPTS;

    extern Z80_OPTS z80_opts;

    enum
    {
        ACCUSE_A = 1,
        ACCUSE_SCRATCH,
        ACCUSE_IY
    };

#ifdef __cplusplus
}
#endif

#define IS_Z80 (z80_opts.sub == SUB_Z80)
#define HAS_IYL_INST (IS_Z80 && options.allow_undoc_inst)
#define IY_RESERVED (z80_opts.reserveIY)
#define OPTRALLOC_IY !(IY_RESERVED)
