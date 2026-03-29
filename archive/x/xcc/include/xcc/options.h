/**
 * Compiler configuration options used across the frontend, core, and backend.
 *
 * This header defines the full xcc_options structure, which contains all
 * command-line flags, tuning parameters, and build options that affect
 * code generation, optimization, output format, and target memory layout.
 *
 * It also includes enums representing the memory model and dependency file
 * output mode.
 *
 * These options are typically initialized at the frontend, passed through
 * to the backend, and referenced throughout the compiler pipeline.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Build model.
        Used in options.model.A bit each as port.supported_models is an OR
        of these.
    */
    enum xcc_model
    {
        XCC_MODEL_NONE = 0,
        XCC_MODEL_SMALL = 1,
        XCC_MODEL_COMPACT = 2,
        XCC_MODEL_MEDIUM = 4,
        XCC_MODEL_LARGE = 8,
        XCC_MODEL_FLAT24 = 16,
        XCC_MODEL_HUGE = 64
    };

    enum xcc_dependency_file_opt
    {
        XCC_DEP_NONE = 0,
        XCC_DEP_SYSTEM = 1,
        XCC_DEP_USER = 2
    };

    struct xcc_options
    {
        int model;
        int stackAuto;
        int useXstack;
        int stack10bit;
        int dump_ast;
        int dump_i_code;
        int dump_graphs;
        int syntax_only;
        int no_assemble;
        int cc_only;
        int c1mode;
        int intlong_rent;
        int float_rent;
        int out_fmt;
        int cyclomatic;
        int noOverlay;
        int xram_movc;
        int nopeep;
        int asmpeep;
        int peepReturn;
        int debug;
        char *peep_file;
        int nostdlib;
        int nostdinc;
        int noRegParams;
        int verbose;
        int lessPedantic;
        int omitFramePtr;
        int useAccelerator;
        int noiv;
        int all_callee_saves;
        int stack_probe;
        int tini_libid;
        int protect_sp_update;
        int parms_in_bank1;
        int stack_size;
        int acall_ajmp;
        int no_ret_without_call;
        int use_non_free;
        int xstack_loc;
        int stack_loc;
        int xdata_loc;
        int data_loc;
        int idata_loc;
        int code_loc;
        int iram_size;
        int xram_size;
        bool xram_size_set;
        int code_size;
        int verboseExec;
        int noXinitOpt;
        int noCcodeInAsm;
        int iCodeInAsm;
        int noPeepComments;
        int verboseAsm;
        int printSearchDirs;
        int vc_err_style;
        int use_stdout;
        int no_std_crt0;
        int std_c95;
        int std_c99;
        int std_c11;
        int std_c23;
        int std_c2y;
        int std_sdcc;
        int dollars_in_ident;
        int signed_char;
        char *code_seg;
        char *const_seg;
        char *data_seg;
        int dependencyFileOpt;
        void *calleeSavesSet; /* Replace `set*` with your own type later */
        void *excludeRegsSet;
        int max_allocs_per_node;
        bool noOptsdccInAsm;
        bool oldralloc;
        int sdcccall;
        bool allow_undoc_inst;
        int xdata_spill;
    };

    extern struct xcc_options options;

#ifdef __cplusplus
}
#endif
