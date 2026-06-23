//
// const_eval.h — compile-time constant expression evaluator for xcc.
//
// Evaluates C integer constant expressions at parse time.  Used by:
//   - _Static_assert conditions
//   - enum initializer values
//   - switch case values
//   - bit-field widths
//   - array dimension expressions (when constant)
//
// Only handles pure integer arithmetic; returns std::nullopt for anything
// that cannot be resolved at compile time (function calls, non-const
// variables, etc.).
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//

#pragma once
#include "ast.h"
#include <cstdint>
#include <optional>

namespace xcc {

class const_expr_evaluator {
public:
    //
    // Try to fold e to a compile-time integer constant.
    // Returns the value on success, std::nullopt if e is not a constant.
    //
    static std::optional<int64_t> evaluate(const expr *e);
};

} // namespace xcc
