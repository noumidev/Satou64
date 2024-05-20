/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pif/pif.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/sm5.hpp"
#include "hw/pif/memory.hpp"

#include "sys/memory.hpp"

namespace hw::pif {

using sm5::SM5;

SM5 pifNUS;

void init() {
    pifNUS.read = &memory::read;
    pifNUS.readRAM = &memory::readRAM;
    pifNUS.write = &memory::write;
    pifNUS.writeRAM = &memory::writeRAM;
}

void deinit() {}

void reset() {
    pifNUS.reset();
}

u32 read(const u64 paddr) {
    u32 data;
    std::memcpy(&data, memory::getRAMPointer(paddr - sys::memory::MemoryBase::PIF_RAM + sys::memory::MemorySize::PIF_RAM), sizeof(u32));

    // PLOG_VERBOSE << "PIF RAM read (address = " << std::hex << paddr << ", data = " << data << ")";

    return data;
}

void write(const u64 paddr, const u32 data) {
    // PLOG_VERBOSE << "PIF RAM write (address = " << std::hex << paddr << ", data = " << data << ")";

    std::memcpy(memory::getRAMPointer(paddr - sys::memory::MemoryBase::PIF_RAM + sys::memory::MemorySize::PIF_RAM), &data, sizeof(u32));
}

void run(const i64 cycles) {
    pifNUS.run(cycles);
}

}
