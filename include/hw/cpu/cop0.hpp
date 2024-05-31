/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

#include "hw/cpu/cpu.hpp"

namespace hw::cpu::cop0 {

namespace InterruptNumber {
    enum : u32 {
        External = 2,
        Compare = 7,
    };
}

void init();
void deinit();

void reset();

bool isCoprocessorUsable(const u32 coprocessor);
bool isLargeFPURegisterFile();

// Returns the value of a COP0 register
template<typename T>
T get(const u32 idx) requires std::is_unsigned_v<T>;

// Sets the value of a COP0 register
template<typename T>
void set(const u32 idx, const T data) requires std::is_unsigned_v<T>;

void setInterruptPending(const u32 interruptNumber);
void clearInterruptPending(const u32 interruptNumber);

void checkInterruptPending();

void clearBranchDelay();

bool getBootExceptionVectors();
bool getExceptionLevel();

void setBranchDelay();
void setExceptionCode(const u32 exceptionCode);
void setExceptionLevel();
void setExceptionPC(const u64 epc);

void doInstruction(const Instruction instr);

void incrementCount();

}
