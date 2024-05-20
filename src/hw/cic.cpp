/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cic.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::cic {

constexpr u8 CIC_ID = 0x8;

namespace Pin {
    enum : u8 {
        DIO = 1 << 0,
        DCLK = 1 << 1,
    };
}

enum class State {
    SendID,
};

State state;

u8 dataOut;

int remainingBits;

void init() {}

void deinit() {}

void reset() {
    state = State::SendID;

    setNextData(CIC_ID);
}

void setNextData(const u8 data) {
    dataOut = data;

    remainingBits = 4;
}

u8 read() {
    PLOG_VERBOSE << "CIC read";

    const u8 data = dataOut & 1;

    dataOut >>= 1;
    remainingBits--;

    if (remainingBits == 0) {
        // Load new data
        switch (state) {
            case State::SendID:
                // TODO: add seed
                break;
        }
    }

    return data << 3;
}

void write(const u8 data) {
    if ((data & Pin::DCLK) == 0) {
        PLOG_VERBOSE << "DCLK is low";

        return;
    }

    switch (state) {
        case State::SendID:
            PLOG_VERBOSE << "DIO ignored";
            break;
        default:
            PLOG_FATAL << "Unhandled CIC write (data = " << std::hex << (u16)data << ")";

            exit(0);
    }
}

}
