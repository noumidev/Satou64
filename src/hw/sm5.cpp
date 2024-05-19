/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/sm5.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::sm5 {

SM5::SM5() {}

SM5::~SM5() {}

void SM5::reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u8 SM5::fetchImmediate() {
    const u8 data = read(regs.pc.raw);

    regs.pc.pl++;

    return data;
}

void SM5::doInstruction() {
    const u16 oldPC = regs.pc.raw;

    const u8 instr = fetchImmediate();

    PLOG_FATAL << "Unrecognized instruction " << std::hex << (u16)instr << " (PC = " << oldPC << ")";

    exit(0);
}

void SM5::run(const i64 cycles) {
    for (i64 i = 0; i < cycles; i++) {
        doInstruction();
    }
}

}
