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
    O3 = 3, // empty experimental alias of -Of; reserved for the next speed pass
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
    bool internal_call_abi_promotion = false;
    bool internal_arg_packing = false;

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
    bool ctype_builtins = false;

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
            // Keep scalar promotion opt-in. Promoted locals can acquire
            // incorrect live ranges when they cross complex CFG joins and
            // are subsequently assigned by the Z80 register allocator.
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
            // Keep the physical-home allocator opt-in. Independent corpus
            // cases still expose incorrect values in otherwise ordinary
            // search, sort, hash, and 64-bit-helper control flow.
            s.regalloc = false;
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
            s.internal_call_abi_promotion = true;
            s.internal_arg_packing = true;
            s.address_deref_fold = true;
            s.block_fill_loops = true;
            // Keep promoted-byte rewriting opt-in.  It still narrows
            // full-width induction state when a byte array sink appears in a
            // large frame, so it is not release-grade for the speed profile.
            s.promoted_byte_ops = false;
            // Prefer branch-form updates such as `count -= !predicate` when
            // the Boolean is consumed only by adjacent arithmetic.  Besides
            // avoiding a materialized 0/1 value, this shortens the taken hot
            // path on the Z80, so it belongs in the speed profile too.
            s.branch_bool_arithmetic = true;
            // Pointer-walk lowering can make an induction variable control
            // only.  Rewriting that newly dead upward counter as a countdown
            // saves a full-width compare on the Z80 hot path.
            s.countdown_dead_loops = true;
            s.value_propagation = false;
            // Graduated from the 2026-08 O3 experiment after the complete
            // compiler matrix and the independent bare, numeric, portable,
            // and full-program benchmark corpora remained correct.  Dense
            // dispatch retains its source-independent profitability guard in
            // the IR driver.
            s.regalloc = true;
            s.scalar_local_promotion = true;
            // The bundled libc exposes only the built-in C locale. Inline
            // its ASCII ctype operations at hot parser sites; users can
            // retain interposable calls with -fno-ctype-builtins.
            s.ctype_builtins = true;
            break;

        case opt_level::O3:
            s = for_level(opt_level::Of);
            // Deliberately empty. Retain a distinct level identity so the next
            // speed experiment can be staged here without changing -Of.
            s.level = level;
            break;

        case opt_level::Os:
            s = for_level(opt_level::O2);
            s.level = level;
            // Fold constant-offset dereferences of known objects back into
            // direct object accesses.  Besides deleting address arithmetic,
            // this frequently lets the backend keep register parameters
            // frameless when they are stored into struct fields or arrays.
            s.address_deref_fold = true;
            // Size mode uses the same restricted static helper inliner as
            // speed mode: the module pass accepts only small leaf helpers and
            // rejects broad loop/control-flow flattening.
            s.inline_static_functions = true;
            s.internal_call_abi_promotion = true;
            // Promoted-byte rewriting remains opt-in for the same reason as
            // the speed profile: byte-array sinks do not prove that every
            // related induction value is narrow.
            s.promoted_byte_ops = false;
            s.branch_bool_arithmetic = true;
            s.countdown_dead_loops = true;
            s.block_fill_loops = true;
            // A register-resident temp is usually smaller as well as faster
            // on the Z80 (it skips a load/store pair entirely), so the -Of
            // physical-register allocator and scalar-local promotion belong
            // in the size profile too rather than leaving every temp in a
            // stack slot. 2026-08-07: enabling both here pending full
            // validation (matrix + benchmark corpora), mirroring the same
            // graduation bar -Of's `regalloc = true` was held to.
            s.regalloc = true;
            s.scalar_local_promotion = true;
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
               internal_call_abi_promotion ||
               internal_arg_packing ||
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
