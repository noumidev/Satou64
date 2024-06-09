/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace hw::pif {

void init();
void deinit();

void reset();

void setInterruptAPending();

void setRCPPort(const bool isRead, const bool is64B);

// Reads data from PIF RAM
template<typename T>
T read(const u64 paddr) requires std::is_unsigned_v<T>;

// Writes data to PIF RAM
void write(const u64 paddr, const u32 data);

void run(const i64 cycles);

}
