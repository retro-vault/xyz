//
// symtab.cpp — scoped symbol table implementation.
//
// push_scope / pop_scope maintain a stack of scope objects for the
// ordinary identifier namespace.  A parallel stack of tag_scopes_
// maps serves the separate C tag namespace (struct/union/enum tags).
// Symbol lookup walks the scope stack from innermost outward.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#include "frontend/symtab.h"
#include <unordered_set>

namespace xcc {

namespace {

void break_type_cycles(const type_ptr &ty, std::unordered_set<const type *> &seen) {
    if (!ty || !seen.insert(ty.get()).second)
        return;

    break_type_cycles(ty->base, seen);
    break_type_cycles(ty->ret, seen);
    for (const auto &param : ty->params)
        break_type_cycles(param, seen);
    for (const auto &field : ty->fields)
        break_type_cycles(field.type, seen);

    ty->base.reset();
    ty->ret.reset();
    ty->params.clear();
    ty->fields.clear();
}

} // namespace

symbol_table::symbol_table() {
    // Global scope is always present.
    scopes_.emplace_back(0);
    tag_scopes_.emplace_back();
}

symbol_table::~symbol_table() {
    std::unordered_set<const type *> seen;

    for (const auto &scope : scopes_) {
        for (const auto &entry : scope.entries()) {
            const auto &sym = entry.second;
            if (!sym)
                continue;
            break_type_cycles(sym->type, seen);
            if (sym->vla_size_sym)
                sym->vla_size_sym.reset();
        }
    }

    for (const auto &tags : tag_scopes_) {
        for (const auto &entry : tags)
            break_type_cycles(entry.second, seen);
    }

    for (auto &scope : scopes_)
        scope.clear();
    tag_scopes_.clear();
}

void symbol_table::push_scope() {
    scopes_.emplace_back(static_cast<int>(scopes_.size()));
    tag_scopes_.emplace_back();
}

void symbol_table::pop_scope() {
    scopes_.pop_back();
    tag_scopes_.pop_back();
}

bool symbol_table::insert(sym_ptr sym) {
    auto &scope = scopes_.back();
    if (scope.lookup(sym->name)) return false;
    sym->scope_depth = scope.depth();
    scope.insert(sym->name, sym);
    return true;
}

sym_ptr symbol_table::lookup(const std::string &name) const {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        if (auto s = scopes_[i].lookup(name)) return s;
    }
    return nullptr;
}

sym_ptr symbol_table::lookup_current(const std::string &name) const {
    return scopes_.back().lookup(name);
}

bool symbol_table::insert_tag(const std::string &name, type_ptr ty) {
    auto &tags = tag_scopes_.back();
    if (tags.count(name)) return false;
    tags[name] = std::move(ty);
    return true;
}

type_ptr symbol_table::lookup_tag(const std::string &name) const {
    for (int i = static_cast<int>(tag_scopes_.size()) - 1; i >= 0; --i) {
        auto it = tag_scopes_[i].find(name);
        if (it != tag_scopes_[i].end()) return it->second;
    }
    return nullptr;
}

} // namespace xcc
