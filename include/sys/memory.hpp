/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include <type_traits>

#include "common/types.hpp"

namespace sys::memory {

// Constants for software fastmem

constexpr u32 PAGE_SHIFT = 12;
constexpr u32 PAGE_SIZE = 1 << PAGE_SHIFT;
constexpr u32 PAGE_MASK = PAGE_SIZE - 1;

// Memory region base addresses
namespace MemoryBase {
    enum : u64 {
        RDRAM = 0,
        RSP_DMEM = 0x4000000,
        RSP_IMEM = 0x4001000,
        CART_DOM1_A2 = 0x10000000,
        PIF_ROM = 0x1FC00000,
        PIF_RAM = 0x1FC007C0,
    };
}

// Memory region sizes
namespace MemorySize {
    enum : u64 {
        RDRAM = 0x800000,
        RSP_DMEM = 0x1000,
        RSP_IMEM = 0x1000,
        PIF_ROM = 0x7C0,
        PIF_RAM = 0x40,
        AddressSpace = 0x80000000,
    };
}

void init(const char *bootPath, const char *romPath);
void deinit();

void reset();

u64 addressToPage(const u64 addr);
constexpr u64 addressToIOPage(const u64 addr);
u64 pageToAddress(const u64 page);

// Returns true if address is a valid physical address
bool isValidPhysicalAddress(const u64 paddr);

// Maps memory into software fastmem page table
void map(const u64 paddr, const u64 size, u8 *mem);

u8 *getPointer(const u64 paddr);

// Reads data from system memory
template<typename T>
T read(const u64 paddr) requires std::is_unsigned_v<T>;

// Reads data from the I/O bus
u32 readIO(const u64 ioaddr);

// Writes data to system memory
template<typename T>
void write(const u64 paddr, const T data) requires std::is_unsigned_v<T>;

// Writes data to the I/O bus
void writeIO(const u64 ioaddr, const u32 data);

}
