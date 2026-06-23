//
// debug_info_adapter.cpp — Translates xcc IR types to xdi calls.
//
// The only file in xcc that knows both xcc's IR AND xdi.
//
#include "backend/z80/debug_info.h"
#include "ir/icode.h"
#include "frontend/types.h"
#include <xbfd/xbfd.h>
#include <map>

namespace xcc {

// ---------------------------------------------------------------------------
// Type translation: xcc type* → xbfd::type_ref
// ---------------------------------------------------------------------------

xbfd::type_ref xdi_adapter::to_xdi(const type* t) {
    if (!t) return xbfd::type_ref::void_();
    xbfd::type_ref r;
    r.size_bytes = t->size() > 0 ? t->size() : 2;
    switch (t->kind) {
    case type_kind::VOID:    r.base = xbfd::type_ref::kind::void_;    break;
    case type_kind::BOOL:    r.base = xbfd::type_ref::kind::uchar;    break;
    case type_kind::CHAR:    r.base = xbfd::type_ref::kind::char_;    break;
    case type_kind::UCHAR:
    case type_kind::CHAR8T:  r.base = xbfd::type_ref::kind::uchar;    break;
    case type_kind::SHORT:   r.base = xbfd::type_ref::kind::short_;   break;
    case type_kind::USHORT:  r.base = xbfd::type_ref::kind::ushort;   break;
    case type_kind::INT:
    case type_kind::ENUM:    r.base = xbfd::type_ref::kind::int_;     break;
    case type_kind::UINT:    r.base = xbfd::type_ref::kind::uint_;    break;
    case type_kind::LONG:    r.base = xbfd::type_ref::kind::long_;    break;
    case type_kind::ULONG:   r.base = xbfd::type_ref::kind::ulong;    break;
    case type_kind::LLONG:   r.base = xbfd::type_ref::kind::llong;    break;
    case type_kind::ULLONG:  r.base = xbfd::type_ref::kind::ullong;   break;
    case type_kind::FLOAT:
    case type_kind::DOUBLE:
    case type_kind::COMPLEX: r.base = xbfd::type_ref::kind::float_;   break;
    case type_kind::POINTER: r.base = xbfd::type_ref::kind::pointer;  break;
    case type_kind::ARRAY:
        r.base = xbfd::type_ref::kind::array;
        r.array_count = (t->base && t->base->size() > 0)
                        ? t->array_size / t->base->size() : 1;
        break;
    case type_kind::FUNCTION: r.base = xbfd::type_ref::kind::function; break;
    case type_kind::STRUCT:
        r.base = xbfd::type_ref::kind::struct_;
        r.tag  = t->tag;
        break;
    case type_kind::UNION:
        r.base = xbfd::type_ref::kind::union_;
        r.tag  = t->tag;
        break;
    default:
        r.base = xbfd::type_ref::kind::int_;
        break;
    }
    return r;
}

xbfd::calling_convention xdi_adapter::to_xdi(call_abi abi) {
    switch (abi) {
    case call_abi::DEFAULT:         return xbfd::calling_convention::normal;
    case call_abi::SDCCCALL0:       return xbfd::calling_convention::xcc_sdcccall0;
    case call_abi::SDCCCALL1:       return xbfd::calling_convention::xcc_sdcccall1;
    case call_abi::Z88DK_FASTCALL:  return xbfd::calling_convention::xcc_z88dk_fastcall;
    case call_abi::Z88DK_CALLEE:    return xbfd::calling_convention::xcc_z88dk_callee;
    case call_abi::NAKED:           return xbfd::calling_convention::xcc_naked;
    case call_abi::INTERRUPT:       return xbfd::calling_convention::xcc_interrupt;
    case call_abi::CRITICAL:        return xbfd::calling_convention::xcc_critical;
    default:                        return xbfd::calling_convention::unknown;
    }
}

// ---------------------------------------------------------------------------
// begin_function: scan icodes, emit locals and params
// ---------------------------------------------------------------------------

void xdi_adapter::begin_function(const ir_function& fn,
                                  const std::string& mangled) {
    writer_->begin_function(fn.name, mangled, fn.is_global,
                             to_xdi(fn.ret_type.get()),
                             to_xdi(fn.abi));

    // Collect unique local variables (non-global, non-param)
    std::map<std::string, const operand*> seen;
    auto collect = [&](const operand& op) {
        if (op.kind != operand_kind::SYMBOL) return;
        if (op.is_global || op.is_param)     return;
        if (op.name.empty())                 return;
        seen.emplace(op.name, &op);
    };
    for (const auto& ic : fn.icodes) {
        collect(ic.result);
        collect(ic.left);
        collect(ic.right);
    }
    for (const auto& [name, op] : seen)
        writer_->add_local(name, to_xdi(op->type.get()),
                            xbfd::storage::stack, op->stack_offset);

    // Parameters from RECEIVE icodes
    for (const auto& ic : fn.icodes) {
        if (ic.op != icode_op::RECEIVE) continue;
        const operand& p = ic.result;
        if (p.name.empty()) continue;
        writer_->add_local(p.name, to_xdi(p.type.get()),
                            xbfd::storage::stack, 4 + p.stack_offset);
    }
}

// ---------------------------------------------------------------------------
// emit_global
// ---------------------------------------------------------------------------

void xdi_adapter::emit_global(const std::string& name,
                               const type* t, bool is_static) {
    writer_->add_global(name, to_xdi(t), is_static);
}

} // namespace xcc
