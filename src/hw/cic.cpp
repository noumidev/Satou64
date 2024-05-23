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

constexpr u64 CIC_ID = 1;
constexpr u64 CIC_SEEDS = 0xBD393D;
constexpr u64 CIC_CHECKSUM = 0x3F57293E547CF590;

namespace Pin {
    enum : u8 {
        DIO = 1 << 0,
        DCLK = 1 << 1,
    };
}

namespace State {
    enum {
        SendID = 0,
        SendSeeds = 1,
        RandomEntropy = 2,
        SendChecksum = 3,
        NumberOfStates,
    };
}

struct CICData {
    u64 data, length;
} CIC_DATA[State::NumberOfStates] = {
    {.data = CIC_ID, 4}, {.data = CIC_SEEDS, .length = 24}, {.data = 0, .length = 0}, {.data = CIC_CHECKSUM, .length = 64},
};

int state;

CICData dataOut;

void init() {}

void deinit() {}

void reset() {
    state = State::SendID;

    setNextData(state);
}

void setNextData(const int idx) {
    dataOut = CIC_DATA[idx];
}

u8 read() {
    PLOG_VERBOSE << "CIC read";

    dataOut.length--;
    const u8 data = (dataOut.data >> dataOut.length) & 1;

    if (dataOut.length == 0) {
        // Load new data
        switch (state) {
            case State::SendID:
                state = State::SendSeeds;

                setNextData(state);
                break;
            case State::SendSeeds:
                state = State::RandomEntropy;
                break;
            case State::RandomEntropy:
                exit(0);
            case State::SendChecksum:
                break;
        }
    }

    return data << 3;
}

void write(const u8 data) {
    if ((data & Pin::DCLK) == 0) {
        PLOG_VERBOSE << "DCLK is low";

        if (state == State::RandomEntropy) {
            state = State::SendChecksum;
        }

        return;
    }

    switch (state) {
        case State::SendID:
        case State::SendSeeds:
        case State::RandomEntropy:
        case State::SendChecksum:
            PLOG_VERBOSE << "DIO ignored";
            break;
        default:
            PLOG_FATAL << "Unhandled CIC write (data = " << std::hex << (u16)data << ")";

            exit(0);
    }
}

}
