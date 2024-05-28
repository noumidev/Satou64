/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::cop0 {

void init();
void deinit();

void reset();

bool isCoprocessorUsable(const u32 coprocessor);

// Returns the value of a COP0 register
template<typename T>
T get(const u32 idx) requires std::is_unsigned_v<T>;

// Sets the value of a COP0 register
template<typename T>
void set(const u32 idx, const T data) requires std::is_unsigned_v<T>;

void doInstruction(const Instruction instr);

}
