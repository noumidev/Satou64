/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <cinttypes>
#include <cstddef>
#include <type_traits>

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using f32 = float;
using f64 = double;

template<typename T>
inline T byteswap(const T data) requires std::is_unsigned_v<T>;

template<>
inline u16 byteswap(const u16 data) {
    return (data >> 8) | (data << 8);
}

template <>
inline u32 byteswap(const u32 data) {
    return (data >> 24) | ((data & 0xFF0000) >> 8) | ((data & 0xFF00) << 8) | (data << 24);
}

template<>
inline u64 byteswap(const u64 data) {
    return (data >> 56) | ((data & 0xFF000000000000) >> 40) | ((data & 0xFF0000000000) >> 24) | ((data & 0xFF00000000) >> 8) | ((data & 0xFF000000) << 8) | ((data & 0xFF0000) << 24) | ((data & 0xFF00) << 40) | (data << 56);
}
