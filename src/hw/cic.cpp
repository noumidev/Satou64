/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/cic.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::cic {

constexpr u64 RAM_SIZE = 32;

// Taken from https://github.com/jago85/UltraCIC_C/blob/3450b4403a1df190b9abb2dbe071ce07a546179b/cic_c.c#L73
constexpr u8 INITIAL_RAM[] = {
    0xE, 0x0, 0x9, 0xA, 0x1, 0x8, 0x5, 0xA, 0x1, 0x3, 0xE, 0x1, 0x0, 0xD, 0xE, 0xC,
    0x0, 0xB, 0x1, 0x4, 0xF, 0x8, 0xB, 0x5, 0x7, 0xC, 0xD, 0x6, 0x1, 0xE, 0x9, 0x8,
};

constexpr void swap(u8 *a, u8 *b) {
    const u8 temp = *a;

    *a = *b;
    *b = temp;
}

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
constexpr u64 CIC_SEEDS = 0xB53F3F;
constexpr u64 CIC_CHECKSUM = 0xA536C0F1D859;

namespace Command {
    enum : u64 {
        Compare = 0,
        Die = 1,
        Challenge = 2,
        Reset = 3,
        NumberOfCommands,
    };
}

constexpr const char *COMMAND_NAMES[Command::NumberOfCommands] = {
    "Compare",
    "Die",
    "Challenge",
    "Reset",
};

namespace DataLength {
    enum : u64 {
        ID = 4,
        Seeds = 24,
        Checksum = 64,
        InitialState = 8,
        Command = 2,
    };
}

namespace Pin {
    enum : u8 {
        DIO = 1 << 0,
        DCLK = 1 << 1,
        DOUT = 1 << 3,
    };
}

enum class State {
    SendID,
    SendSeeds,
    RandomEntropy,
    SendChecksum,
    ReceiveInitialState,
    ReceiveCommand,
    CommandCompare,
};

struct CICData {
    u64 data, length;
};

CICData dataIn, dataOut;

State state;

std::array<u8, RAM_SIZE> ram;

void init() {
    PLOG_INFO << "IPL2/3 seed is " << std::hex << CIC_SEEDS;
    PLOG_INFO << "IPL3 checksum is " << std::hex << CIC_CHECKSUM;
}

void deinit() {}

void reset() {
    std::memcpy(ram.data(), INITIAL_RAM, RAM_SIZE);

    state = State::SendID;

    setDataOut(CIC_ID, DataLength::ID);
}

void setDataIn(const u64 length) {
    dataIn.data = 0;
    dataIn.length = length;
}

void setDataOut(const u64 data, const u64 length) {
    dataOut.data = data;
    dataOut.length = length;
}

u8 read() {
    // PLOG_VERBOSE << "CIC read";

    dataOut.length--;
    const u8 data = (dataOut.data >> dataOut.length) & 1;

    if (dataOut.length == 0) {
        // Load new data
        switch (state) {
            case State::SendID:
                state = State::SendSeeds;

                setDataOut(CIC_SEEDS, DataLength::Seeds);

                for (int i = 0; i < 2; i++) {
                    dataOut.data = SCRAMBLE(dataOut.data, dataOut.length);
                }
                break;
            case State::SendSeeds:
                state = State::RandomEntropy;
                break;
            case State::SendChecksum:
                state = State::ReceiveInitialState;

                setDataIn(DataLength::InitialState);
                break;
            case State::CommandCompare:
                state = State::ReceiveCommand;

                setDataIn(DataLength::Command);
                break;
            case State::RandomEntropy:
            case State::ReceiveInitialState:
            case State::ReceiveCommand:
                // Shouldn't happen
                break;
        }
    }

    return data * Pin::DOUT;
}

void write(const u8 data) {
    if ((data & Pin::DCLK) == 0) {
        // PLOG_VERBOSE << "DCLK is low";

        if (state == State::RandomEntropy) {
            state = State::SendChecksum;

            setDataOut(CIC_CHECKSUM, DataLength::Checksum);

            for (int i = 0; i < 4; i++) {
                dataOut.data = SCRAMBLE(dataOut.data, dataOut.length);
            }
        }

        return;
    }
    
    const u64 bit = data & Pin::DIO;

    // PLOG_VERBOSE << "CIC write (data = " << std::hex << bit << ")";

    switch (state) {
        case State::SendID:
        case State::SendSeeds:
        case State::RandomEntropy:
        case State::SendChecksum:
        case State::CommandCompare:
            // PLOG_VERBOSE << "DIO ignored";
            return;
        case State::ReceiveInitialState:
        case State::ReceiveCommand:
            break;
        default:
            PLOG_FATAL << "Unhandled CIC write (data = " << std::hex << (u16)data << ")";

            exit(0);
    }

    dataIn.length--;
    dataIn.data |= bit << dataIn.length;

    if (dataIn.length == 0) {
        // PLOG_VERBOSE << "Data in = " << std::hex << dataIn.data;

        switch (state) {
            case State::ReceiveInitialState:
                ram[0x01] = (dataIn.data >> 4) & 0xF;
                ram[0x11] = (dataIn.data >> 0) & 0xF;

                state = State::ReceiveCommand;

                setDataIn(DataLength::Command);
                break;
            case State::ReceiveCommand:
                {
                    const u64 command = dataIn.data;
                    switch (command) {
                        case Command::Compare:
                            doCompare();
                            break;
                        default:
                            PLOG_FATAL << "Unrecognized CIC command " << COMMAND_NAMES[command];

                            exit(0);
                    }
                }
                break;
            default:
                PLOG_FATAL << "Unhandled CIC write";

                exit(0);
        }
    }
}

// Taken from https://github.com/jago85/UltraCIC_C/blob/3450b4403a1df190b9abb2dbe071ce07a546179b/cic_c.c#L240
void compareScramble(const u64 addr) {
    u8 a, b, x;
    u8 *m = &ram[addr];

    a = x = m[15];

    do {
        b = 1;
        a += m[b] + 1;
        m[b] = a;
        b++;
        a += m[b] + 1;

        swap(&a, &m[b]);

        m[b] = ~m[b];
        b++;
        a &= 0xF;
        a += (m[b] & 0xF) + 1;

        if (a < 16) {
            swap(&a, &m[b]);

            b++;
        }

        a += m[b];
        m[b] = a;
        b++;
        a += m[b];

        swap(&a, &m[b]);

        b++;
        a &= 0xF;
        a += 8;

        if (a < 16) {
            a += m[b];
        }

        swap(&a, &m[b]);

        b++;

        do {
            a += m[b] + 1;
            m[b] = a;
            b = (b + 1) & 0xF;
        } while (b != 0);

        a = x + 0xF;
        x = a & 0xF;
    } while (x != 15);
}

// Taken from https://github.com/jago85/UltraCIC_C/blob/3450b4403a1df190b9abb2dbe071ce07a546179b/cic_c.c#L332
void doCompare() {
    PLOG_VERBOSE << "Compare";

    state = State::CommandCompare;

    for (int i = 0; i < 3; i++) {
        compareScramble(0x10);
    }

    u64 addr = ram[0x17] & 0xF;
    if (addr == 0) {
        addr = 1;
    }

    addr |= 0x10;

    setDataOut(0, 0);

    for (; (addr & 0xF) != 0; addr++) {
        dataIn.data <<= 1;
        dataIn.data |= ram[addr] & 1;

        dataIn.length++;
    }
}

}
