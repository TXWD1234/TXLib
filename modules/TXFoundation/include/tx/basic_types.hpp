// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXFoundation

#pragma once
#include <cstdint>

namespace tx {
// clang-format off

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;
using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8  = int8_t;
using f32 = float;
using f64 = double;

constexpr u64 InvalidU64 = UINT64_MAX;
constexpr u32 InvalidU32 = UINT32_MAX;
constexpr u16 InvalidU16 = UINT16_MAX;
constexpr u8  InvalidU8 = UINT8_MAX;

constexpr inline bool valid(u64 val) { return val != InvalidU64; }
constexpr inline bool valid(u32 val) { return val != InvalidU32; }
constexpr inline bool valid(u16 val) { return val != InvalidU16; }
constexpr inline bool valid(u8 val)  { return val != InvalidU8; }

// clang-format on
} // namespace tx