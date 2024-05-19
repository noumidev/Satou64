/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pif/memory.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::pif::memory {

std::array<u8, MemorySize::RAM> ram;
std::array<u8, MemorySize::ROM> rom;

void init(const char *pifPath) {
    // Read PIF-NUS ROM
    FILE *file = std::fopen(pifPath, "rb");
    if (file == NULL) {
        PLOG_FATAL << "Unable to open PIF-NUS ROM file";

        exit(0);
    }

    // Read file
    std::fread(rom.data(), sizeof(u8), MemorySize::ROM, file);
    std::fclose(file);
}

void deinit() {}

void reset() {}

u8 read(const u16 paddr) {
    if (paddr < MemorySize::ROM) {
        return rom[paddr];
    }

    PLOG_FATAL << "Unrecognized read (address = " << std::hex << paddr << ")";

    exit(0);
}

u8 readRAM(const u8 paddr) {
    PLOG_VERBOSE << "PIF RAM read (address = " << std::hex << (u16)paddr << ")";

    return ram[paddr];
}

void write(const u16 paddr, const u8 data) {
    PLOG_FATAL << "Unrecognized write (address = " << std::hex << paddr << ", data = " << (u16)data << ")";

    exit(0);
}

void writeRAM(const u8 paddr, const u8 data) {
    PLOG_VERBOSE << "PIF RAM write (address = " << std::hex << (u16)paddr << ", data = " << (u16)data << ")";

    ram[paddr] = data;
}

}
