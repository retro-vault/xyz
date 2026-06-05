//
// debug_info.h — Thin adapter: xcc IR events → xbfd::debug_writer calls.
//
// xcc's Z80 backend calls this interface; it translates xcc-specific
// types (ir_function, type*) into xbfd::type_ref / xbfd::storage and
// forwards them to an xbfd::debug_writer.
//
// MIT License (see: LICENSE)
// Copyright (C) 2026 tomaz stih
//
#pragma once
#include "ir/icode.h"
#include "frontend/types.h"
#include <xbfd/xbfd.h>
#include <memory>
#include <string>

namespace xcc {

// Abstract interface the Z80 backend calls — matches xdi events 1:1
// but uses xcc IR types so xcc code stays clean.
class debug_info_emitter {
public:
    virtual ~debug_info_emitter() = default;

    virtual void begin_module() = 0;
    virtual void emit_location(int line) = 0;
    virtual void begin_function(const ir_function& fn,
                                const std::string& mangled) = 0;
    virtual void end_function  (const ir_function& fn) = 0;
    virtual void emit_global   (const std::string& name,
                                 const type*       t,
                                 bool              is_static) {}
    virtual void end_module(const std::string& producer) = 0;
};

// ---------------------------------------------------------------------------
// xdi_adapter — bridges xcc IR types to xbfd::debug_writer
// ---------------------------------------------------------------------------

class xdi_adapter final : public debug_info_emitter {
public:
    explicit xdi_adapter(std::unique_ptr<xbfd::debug_writer> writer,
                         const std::string& source_file)
        : owned_(std::move(writer))
        , writer_(owned_.get())
        , source_file_(source_file) {}

    void begin_module() override {
        writer_->begin_module();
    }
    void emit_location(int line) override {
        writer_->source_line(line);
    }
    void begin_function(const ir_function& fn, const std::string& mangled) override;
    void end_function  (const ir_function& fn) override {
        writer_->end_function(fn.name);
    }
    void emit_global(const std::string& name, const type* t, bool is_static) override;
    void end_module(const std::string& producer) override {
        writer_->end_module(producer);
    }

private:
    static xbfd::type_ref to_xdi(const type* t);

    std::unique_ptr<xbfd::debug_writer> owned_;
    xbfd::debug_writer*                 writer_;
    std::string                        source_file_;
};

} // namespace xcc
