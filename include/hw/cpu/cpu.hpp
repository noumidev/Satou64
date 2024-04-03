/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace hw::cpu {

void init();
void deinit();

void run();

void reset(const bool isFastBoot);

// Returns true if register index is valid
bool isValidRegisterIndex(const u32 idx);

// Returns true if address is aligned
template<typename T>
bool isAlignedAddress(const u64 addr) requires std::is_unsigned_v<T> {
    return (addr & (sizeof(T) - 1)) == 0;
}

// Returns the value of a GPR
u64 get(const u32 idx);

// Returns the program counter
template<bool isCurrentPC>
u64 getPC();

// Sets the value of a GPR
template<typename T>
void set(const u32 idx, const T data) requires std::is_unsigned_v<T>;

// Sets the program counter
template<bool isBranch>
void setPC(const u64 addr);

void advancePC();

u64 translateAddress(const u64 vaddr);

// Reads data from memory. Does virtual address translation
template<typename T>
T read(const u64 vaddr) requires std::is_unsigned_v<T>;

// Fetches an instruction word, increments PC
u32 fetch();

// Writes data to memory. Does virtual address translation
template<typename T>
void write(const u64 vaddr, const T data) requires std::is_unsigned_v<T>;

void doInstruction();

void run(const i64 cycles);

}
