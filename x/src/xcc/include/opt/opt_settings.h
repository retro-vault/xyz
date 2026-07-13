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
    O1 = 1, // late target cleanup only (peephole + tiny backend fusions)
    O2 = 2, // smart optimizer baseline (IR + backend + O1 cleanup)
    O3 = 3, // separately routed experimental speed profile
    Of = 4, // O2-based speed profile with validated speed hooks
    Os = 5, // size-biased smart optimization
};

struct optimization_settings {
    opt_level level = opt_level::O0;

    // Late assembly passes. These always run after IR optimization and
    // target code generation; they never replace the IR pipeline.
    bool peephole = false;

    // Module-level IR passes.
    bool dead_static_functions = false;
    bool const_arg_propagation = false;
    bool const_call_eval = false;
    bool function_const_eval = false;
    bool dead_params = false;
    bool merge_identical_functions = false;
    bool inline_trivial_internal_functions = false;
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
    bool tail_recursion_elim = false;
    bool short_circuit_bool_ifx = false;
    bool branch_bool_arithmetic = false;
    bool countdown_dead_loops = false;
    bool block_fill_loops = false;
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
            s.compare_ifx_fusion = true;
            break;

        case opt_level::O2:
            s.peephole = true;
            s.dead_static_functions = true;
            s.const_arg_propagation = true;
            s.const_call_eval = true;
            s.function_const_eval = true;
            s.dead_params = true;
            s.merge_identical_functions = true;
            s.inline_trivial_internal_functions = true;
            s.inline_static_functions = false;
            s.cfg_cleanup = true;
            s.jump_threading = true;
            // Keep the global value/memory propagation family opt-in until its
            // alias and memory-version tracking is strong enough for pointer-
            // heavy parser/formatter code (jsmn, optparse, nanoprintf).
            s.value_propagation = false;
            s.constant_folding = true;
            s.algebraic_simplify = true;
            s.local_cse = true;
            s.loop_licm = true;
            s.loop_induction = true;
            s.strength_reduction = true;
            s.dead_code_elim = true;
            // Keep scalar promotion opt-in for now; it can change byte-local
            // lifetimes across pointer-heavy loops.
            s.scalar_local_promotion = false;
            s.reg_param_promotion = true;
            s.tail_recursion_elim = true;
            s.short_circuit_bool_ifx = true;
            s.narrow_counted_byte_loops = true;
            s.loop_pointer_walk = true;
            s.promoted_byte_compare = true;
            // Byte-op promotion is still useful, but the current family can
            // narrow full-width parser state incorrectly (picohttpparser).
            s.promoted_byte_ops = false;
            s.rotate_combine = true;
            s.duplicate_block_merge = true;
            s.merge_tails = false;
            s.local_frame_compaction = true;
            // Keep the allocator opt-in until its Z80 lowering is proven on
            // larger stack-heavy kernels and pointer-heavy control flow.
            s.regalloc = true;
            s.compare_ifx_fusion = true;
            s.frame_omit = true;
            s.prealloc_temp_frame = true;
            s.switch_jump_tables = true;
            break;

        case opt_level::Of:
            s = for_level(opt_level::O2);
            s.level = level;
            // -Of starts from the broad-tested O2 pipeline. Static inlining is
            // restricted by the module pass to small leaf helpers, avoiding
            // broad loop/control-flow flattening until the allocator can
            // safely handle those larger regions.
            s.inline_static_functions = true;
            s.block_fill_loops = true;
            // Keep promoted byte expressions narrow when they provably flow
            // back to byte sinks; this avoids Z80 stack-heavy 16-bit lowering
            // for ordinary unsigned/signed char arithmetic.
            s.promoted_byte_ops = true;
            // Speed mode can use the conservative Z80 temp/register allocator:
            // the allocator itself rejects calls, clobbers, address-taking,
            // deep-frame hazards, and unsafe pointer rematerialization.
            // -Os enables the same allocator below after separate size/cycle
            // validation.
            s.regalloc = true;
            // Promote simple scalar locals to temps before backend allocation.
            // This lets leaf helpers keep values in registers instead of
            // manufacturing stack locals solely for reloads.
            s.scalar_local_promotion = true;
            s.value_propagation = false;
            break;

        case opt_level::O3:
            s = for_level(opt_level::Of);
            s.level = level;
            break;

        case opt_level::Os:
            s = for_level(opt_level::O2);
            s.level = level;
            // Size mode uses the same restricted static helper inliner as
            // speed mode: the module pass accepts only small leaf helpers and
            // rejects broad loop/control-flow flattening.
            s.inline_static_functions = true;
            // These two backend families are not benchmark recognizers:
            // byte-op promotion only keeps already-provable byte expressions
            // narrow, and the Z80 allocator rejects unsafe functions. In the
            // z88dk kernels they reduce both generated size and cycles for
            // stack-heavy byte/switch code, so they belong in -Os as well.
            s.promoted_byte_ops = true;
            s.regalloc = true;
            s.scalar_local_promotion = true;
            s.branch_bool_arithmetic = true;
            s.countdown_dead_loops = true;
            s.block_fill_loops = true;
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
               inline_trivial_internal_functions ||
               inline_static_functions ||
               tail_recursion_elim;
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
               tail_recursion_elim ||
               short_circuit_bool_ifx ||
               branch_bool_arithmetic ||
               countdown_dead_loops ||
               block_fill_loops ||
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
