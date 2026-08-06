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

struct address_constant {
    std::string symbol;
    int64_t     byte_offset = 0;
};

class const_expr_evaluator {
public:
    //
    // Try to fold e to a compile-time integer constant.
    // Returns the value on success, std::nullopt if e is not a constant.
    //
    static std::optional<int64_t> evaluate(const expr *e);

    //
    // Try to fold e to a compile-time floating-point constant.
    // Returns the value on success, std::nullopt if e is not a constant.
    //
    static std::optional<double> evaluate_float(const expr *e);

    // Evaluate an arithmetic constant expression as if converted to the
    // supplied integer type.  Unlike evaluate(), this deliberately accepts
    // floating constants used by static integer initializers.
    static std::optional<int64_t> evaluate_integer_conversion(
        const expr *e, type_ptr target);

    // Evaluate a C address constant used by an object with static storage.
    // The result remains symbolic so the backend can emit a relocation.
    static std::optional<address_constant> evaluate_address(const expr *e);
};

} // namespace xcc
