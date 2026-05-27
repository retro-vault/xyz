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

namespace xcc {

symbol_table::symbol_table() {
    // Global scope is always present.
    scopes_.emplace_back(0);
    tag_scopes_.emplace_back();
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
