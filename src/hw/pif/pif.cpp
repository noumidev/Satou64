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
    switch (paddr) {
        default:
            PLOG_FATAL << "Unrecognized read (address = " << std::hex << paddr << ")";

            exit(0);
    }
}

void write(const u64 paddr, const u32 data) {
    switch (paddr) {
        default:
            PLOG_FATAL << "Unrecognized write (address = " << std::hex << paddr << ", data = " << data << ")";

            exit(0);
    }
}

void run(const i64 cycles) {
    pifNUS.run(cycles);
}

}
