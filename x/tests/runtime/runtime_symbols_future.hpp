// runtime_symbols_future.hpp — addresses for long long / double runtime
// functions.
//
// These now alias the auto-generated runtime_symbols.hpp (rt_sym::) which
// is regenerated from the linked binary on every build. As long as the
// corresponding .s file is present in src/xc/xcc/lib/runtime/ and exports
// the symbol, its address is resolved automatically — no manual updates.
//
// MIT License (see: LICENSE)
// copyright (C) 2026 tomaz stih
#pragma once
#include <cstdint>
#include "runtime_symbols.hpp"

namespace rt_sym_future {

// ---------------------------------------------------------------------------
// 64-bit integer (long long)
// ---------------------------------------------------------------------------
static constexpr uint16_t mulll    = rt_sym::mulll;
static constexpr uint16_t divull   = rt_sym::divull;
static constexpr uint16_t divsll   = rt_sym::divsll;
static constexpr uint16_t modull   = rt_sym::modull;
static constexpr uint16_t modsll   = rt_sym::modsll;

static constexpr uint16_t shl64    = rt_sym::shl64;
static constexpr uint16_t shr64u   = rt_sym::shr64u;
static constexpr uint16_t shr64s   = rt_sym::shr64s;

static constexpr uint16_t sint2ll  = rt_sym::sint2ll;
static constexpr uint16_t uint2ll  = rt_sym::uint2ll;
static constexpr uint16_t slong2ll = rt_sym::slong2ll;
static constexpr uint16_t ulong2ll = rt_sym::ulong2ll;

static constexpr uint16_t ll2sint  = rt_sym::ll2sint;
static constexpr uint16_t ll2uint  = rt_sym::ll2uint;
static constexpr uint16_t ll2slong = rt_sym::ll2slong;
static constexpr uint16_t ll2ulong = rt_sym::ll2ulong;

// ---------------------------------------------------------------------------
// double (IEEE-754 64-bit float)
// ---------------------------------------------------------------------------
static constexpr uint16_t dbadd    = rt_sym::dbadd;
static constexpr uint16_t dbsub    = rt_sym::dbsub;
static constexpr uint16_t dbmul    = rt_sym::dbmul;
static constexpr uint16_t dbdiv    = rt_sym::dbdiv;
static constexpr uint16_t dbneg    = rt_sym::dbneg;
static constexpr uint16_t dbsqrt   = rt_sym::dbsqrt;

static constexpr uint16_t dbcmp    = rt_sym::dbcmp;
static constexpr uint16_t dbeq     = rt_sym::dbeq;
static constexpr uint16_t dblt     = rt_sym::dblt;

static constexpr uint16_t sint2db  = rt_sym::sint2db;
static constexpr uint16_t uint2db  = rt_sym::uint2db;
static constexpr uint16_t slong2db = rt_sym::slong2db;
static constexpr uint16_t ulong2db = rt_sym::ulong2db;
static constexpr uint16_t sll2db   = rt_sym::sll2db;
static constexpr uint16_t ull2db   = rt_sym::ull2db;
static constexpr uint16_t fs2db    = rt_sym::fs2db;

static constexpr uint16_t db2sint  = rt_sym::db2sint;
static constexpr uint16_t db2uint  = rt_sym::db2uint;
static constexpr uint16_t db2slong = rt_sym::db2slong;
static constexpr uint16_t db2ulong = rt_sym::db2ulong;
static constexpr uint16_t db2sll   = rt_sym::db2sll;
static constexpr uint16_t db2ull   = rt_sym::db2ull;
static constexpr uint16_t db2fs    = rt_sym::db2fs;

} // namespace rt_sym_future
