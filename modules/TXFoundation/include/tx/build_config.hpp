// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXFoundation

#pragma once

namespace tx::config {
#ifdef NDEBUG
inline constexpr const bool enabled_debug = false;
#else
inline constexpr const bool enabled_debug = true;
#endif

#ifdef __cpp_exceptions
inline constexpr const bool enabled_exception = true;
#else
inline constexpr const bool enabled_exception = false;
#endif
} // namespace tx::config