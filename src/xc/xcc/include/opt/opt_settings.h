//
// opt_settings.h — optimization presets and fine-grained pass controls.
//
// xcc exposes GCC-style -O presets for normal use, but expert users can
// also toggle individual optimization families with -f... / -fno-...
// switches.  optimization_settings is the normalized internal form used by
// the driver, IR optimizers, backend, and late assembly passes.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once

namespace xcc {

enum class opt_level {
    O0 = 0, // no optimization
    O1 = 1, // peephole rules
    O2 = 2, // general optimization
    O3 = 3, // single-translation-unit experimental optimization
    Of = 4, // speed optimization
    Os = 5, // size optimization
};

struct optimization_settings {
    opt_level level = opt_level::O0;

    // Late assembly passes.
    bool peephole = false;

    // Module-level IR passes.
    bool dead_static_functions = false;
    bool const_arg_propagation = false;
    bool const_call_eval = false;
    bool function_const_eval = false;
    bool dead_params = false;
    bool merge_identical_functions = false;
    bool inline_static_functions = false;

    // Per-function IR passes.
    bool cfg_cleanup = false;
    bool jump_threading = false;
    bool address_deref_fold = false;
    bool value_propagation = false;
    bool constant_folding = false;
    bool algebraic_simplify = false;
    bool local_cse = false;
    bool loop_licm = false;
    bool loop_induction = false;
    bool strength_reduction = false;
    bool dead_code_elim = false;
    bool scalar_local_promotion = false;
    bool reg_param_promotion = false;
    bool short_circuit_bool_ifx = false;
    bool narrow_counted_byte_loops = false;
    bool loop_pointer_walk = false;
    bool promoted_byte_compare = false;
    bool promoted_byte_ops = false;
    bool rotate_combine = false;
    bool duplicate_block_merge = false;
    bool merge_tails = false;
    bool local_frame_compaction = false;

    // Backend/codegen passes.
    bool regalloc = false;
    bool compare_ifx_fusion = false;
    bool frame_omit = false;
    bool prealloc_temp_frame = false;
    bool switch_jump_tables = false;

    static optimization_settings for_level(opt_level level) {
        optimization_settings s;
        s.level = level;

        switch (level) {
        case opt_level::O0:
            break;

        case opt_level::O1:
            s.peephole = true;
            break;

        case opt_level::O2:
            s.peephole = true;
            s.dead_static_functions = true;
            s.const_arg_propagation = true;
            s.const_call_eval = true;
            s.function_const_eval = true;
            s.dead_params = true;
            s.merge_identical_functions = true;
            s.inline_static_functions = false;
            s.cfg_cleanup = true;
            s.jump_threading = true;
            s.address_deref_fold = false;
            s.value_propagation = true;
            s.constant_folding = true;
            s.algebraic_simplify = true;
            s.local_cse = true;
            s.loop_licm = true;
            s.loop_induction = true;
            s.strength_reduction = true;
            s.dead_code_elim = true;
            s.scalar_local_promotion = true;
            s.reg_param_promotion = true;
            s.short_circuit_bool_ifx = true;
            s.narrow_counted_byte_loops = true;
            s.loop_pointer_walk = true;
            s.promoted_byte_compare = true;
            s.promoted_byte_ops = true;
            s.rotate_combine = true;
            s.duplicate_block_merge = false;
            s.merge_tails = false;
            s.local_frame_compaction = true;
            s.regalloc = true;
            s.compare_ifx_fusion = true;
            s.frame_omit = true;
            s.switch_jump_tables = true;
            break;

        case opt_level::Of:
            s = for_level(opt_level::O3);
            s.level = level;
            break;

        case opt_level::O3:
            s = for_level(opt_level::O2);
            s.level = level;
            s.inline_static_functions = true;
            break;

        case opt_level::Os:
            s = for_level(opt_level::O3);
            s.level = level;
            break;

        }

        return s;
    }

    bool has_module_passes() const {
        return dead_static_functions ||
               const_arg_propagation ||
               const_call_eval ||
               function_const_eval ||
               dead_params ||
               merge_identical_functions ||
               inline_static_functions;
    }

    bool has_function_ir_passes() const {
        return cfg_cleanup ||
               jump_threading ||
               address_deref_fold ||
               value_propagation ||
               constant_folding ||
               algebraic_simplify ||
               local_cse ||
               loop_licm ||
               loop_induction ||
               strength_reduction ||
               dead_code_elim ||
               scalar_local_promotion ||
               reg_param_promotion ||
               short_circuit_bool_ifx ||
               narrow_counted_byte_loops ||
               loop_pointer_walk ||
               promoted_byte_compare ||
               promoted_byte_ops ||
               rotate_combine ||
               duplicate_block_merge ||
               merge_tails ||
               local_frame_compaction;
    }
};

} // namespace xcc
