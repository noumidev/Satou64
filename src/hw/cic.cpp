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

constexpr u64 SCRAMBLE(const u64 data, const int length) {
    u64 result = data & (0xFULL << (length - 4));
    for (int i = length - 8; i >= 0; i -= 4) {
        const u64 prev = (result >> (i + 4)) & 0xF;
        const u64 curr = (data   >> (i + 0)) & 0xF;

        result |= ((prev + curr + 1) & 0xF) << i;
    }

    return result;
}

constexpr u64 CIC_ID = 1;
constexpr u64 CIC_SEEDS = SCRAMBLE(SCRAMBLE(0xB53F3F, 24), 24);
constexpr u64 CIC_CHECKSUM = SCRAMBLE(SCRAMBLE(SCRAMBLE(SCRAMBLE(0x0000A536C0F1D859, 64), 64), 64), 64);

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

void init() {
    PLOG_INFO << "IPL2/3 seed is " << std::hex << CIC_SEEDS;
    PLOG_INFO << "IPL3 checksum is " << std::hex << CIC_CHECKSUM;
}

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
                break;
            case State::RandomEntropy:
                exit(0);
            case State::SendChecksum:
                PLOG_ERROR << "SendChecksum";
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

            setNextData(state);
        }

        return;
    } else {
        if ((state == State::SendSeeds) && (dataOut.length == 0)) {
            state = State::RandomEntropy;
        }
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
