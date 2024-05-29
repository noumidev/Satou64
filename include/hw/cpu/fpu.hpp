/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::fpu {

void init();
void deinit();

void reset();

bool getCondition();

f32 makeSingle(const u32 data);
f64 makeDouble(const u64 data); 
u32 makeWord(const f32 data);
u64 makeLong(const f64 data);

// Returns the value of an FPU register
template<typename T>
T get(const u32 idx) requires std::is_unsigned_v<T>;

// Returns the value of an FPU control register
u32 getControl(const u32 idx);

// Sets the value of an FPU register
template<typename T>
void set(const u32 idx, const T data) requires std::is_unsigned_v<T>;

// Sets the value of an FPU control register
void setControl(const u32 idx, const u32 data);

void doSingle(const Instruction instr);
void doDouble(const Instruction instr);
void doWord(const Instruction instr);

}
