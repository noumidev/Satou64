/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace hw::cpu {

namespace ExceptionCode {
    enum : u32 {
        Interrupt = 0x00,
    };
}

// CPU instruction
union Instruction {
    u32 raw;

    // Immediate type
    struct {
        u32 immediate : 16;
        u32 rt : 5;
        u32 rs : 5;
        u32 op : 6;
    } iType;

    // Jump type
    struct {
        u32 target : 26;
        u32 op : 6;
    } jType;

    // Register type
    struct {
        u32 funct : 6;
        u32 sa : 5;
        u32 rd : 5;
        u32 rt : 5;
        u32 rs : 5;
        u32 op : 6;
    } rType;

    // FPU instruction
    struct {
        u32 funct : 6;
        u32 fd : 5;
        u32 fs : 5;
        u32 ft : 5;
        u32 fmt : 5;
        u32 op : 6;
    } fType;
};

void init();
void deinit();

void reset();

void raiseException(const u32 exceptionCode);

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
u64 getPC();
u64 getCurrentPC();

// Sets the value of a GPR
template<typename T>
void set(const u32 idx, const T data) requires std::is_unsigned_v<T>;

// Sets the program counter
void setPC(const u64 addr);
void setBranchPC(const u64 addr);

void branch(const u64 target, const bool condition, const u32 linkReg, const bool isLikely);

void advanceDelaySlot();
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
