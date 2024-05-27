/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/ri.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

namespace hw::ri {

constexpr u64 MODULE_NUM = 2;

constexpr u64 ADDRESS_MASK = 0x3F003FF;

union DeviceID {
    u32 raw;
    struct {
        u32 : 7;
        u32 idHi : 1;
        u32 idMidHi : 8;
        u32 : 7;
        u32 idMidLo : 1;
        u32 : 2;
        u32 idLo : 6;
    };

    u64 getID() {
        return idLo | (idMidLo << 6) | (idMidHi << 7) | (idHi << 15);
    }
};

// RDRAM delay register
union Delay {
    u32 raw;
    struct {
        u32 writeBits : 3;
        u32 writeDelay : 3;
        u32 : 2;
        u32 ackBits : 3;
        u32 ackDelay : 2;
        u32 : 3;
        u32 readBits : 3;
        u32 readDelay : 3;
        u32 : 2;
        u32 ackWinBits : 3;
        u32 ackWinDelay : 3;
        u32 : 2;
    };
};

union Mode {
    u32 raw;
    struct {
        u32 : 6;
        u32 c0 : 1;
        u32 c3 : 1;
        u32 : 6;
        u32 c1 : 1;
        u32 c4 : 1;
        u32 : 6;
        u32 c2 : 1;
        u32 c5 : 1;
        u32 lowPowerEnable : 1;
        u32 deviceEnable : 1;
        u32 autoSkip : 1;
        u32 skip : 1;
        u32 skipValue : 1;
        u32 powerDownLatency : 1;
        u32 x2 : 1;
        u32 currentControlEnable : 1;
    };
};

union RefRow {
    u32 raw;
    struct {
        u32 : 8;
        u32 rowFieldHi : 2;
        u32 : 9;
        u32 bankField : 1;
        u32 : 5;
        u32 rowFieldLo : 7;
    };
};

// RDRAM module
struct Module {
    DeviceID devID;
    Delay delay;
    Mode mode;
    RefRow refRow;
};

union MODE {
    u32 raw;
    struct {
        u32 operationMode : 2;
        u32 stopTransmit : 1;
        u32 stopReceive : 1;
        u32 : 28;
    };
};

union CONFIG {
    u32 raw;
    struct {
        u32 currentControl : 6;
        u32 autoCurrentControl : 1;
        u32 : 25;
    };
};

union SELECT {
    u32 raw;
    struct {
        u32 rsel : 4;
        u32 tsel : 4;
        u32 : 24;
    };
};

union REFRESH {
    u32 raw;
    struct {
        u32 cleanRefreshDelay : 8;
        u32 dirtyRefreshDelay : 8;
        u32 bank : 1;
        u32 refreshEnable : 1;
        u32 optimize : 1;
        u32 multiBank : 4;
        u32 : 9;
    };
};

struct Registers {
    MODE mode;
    CONFIG config;
    SELECT select;
    REFRESH refresh;
};

Module modules[2];
Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&modules, 0, sizeof(modules));
    std::memset(&regs, 0, sizeof(Registers));
}

u64 getRDRAMAddress(const u64 ioaddr) {
    const u64 addrLo = ioaddr & 0x3FF;
    const u64 addrHi = (ioaddr >> 10) & 0x1FF;

    return (addrHi << 20) | (addrHi << 11) | addrLo;
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::SELECT:
            PLOG_INFO << "SELECT read";

            return regs.select.raw;
        case IORegister::REFRESH:
            PLOG_INFO << "REFRESH read";

            return regs.refresh.raw;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

u32 readRDRAM(const u64 ioaddr) {
    const u64 moduleAddr = getRDRAMAddress(ioaddr);

    // Find RDRAM module
    Module *module = NULL;

    u64 idx = 0;
    for (; idx < MODULE_NUM; idx++) {
        if ((moduleAddr >> 20) == modules[idx].devID.getID()) {
            module = &modules[idx];

            break;
        }
    }

    if (module == NULL) {
        PLOG_ERROR << "No module responded to read (address = " << std::hex << ioaddr << ", module address = " << moduleAddr << ")";

        return 0;
    }

    switch (ioaddr & ADDRESS_MASK) {
        case RDRAMRegister::Mode:
            PLOG_INFO << "Mode read";
            PLOG_VERBOSE << "Module " << idx << " mode read";

            return module->mode.raw;
        default:
            PLOG_FATAL << "Unrecognized RDRAM IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::MODE:
            PLOG_INFO << "MODE write (data = " << std::hex << data << ")";

            regs.mode.raw = data;
            break;
        case IORegister::CONFIG:
            PLOG_INFO << "CONFIG write (data = " << std::hex << data << ")";

            regs.config.raw = data;
            break;
        case IORegister::CURRENTLOAD:
            PLOG_INFO << "CURRENTLOAD write (data = " << std::hex << data << ")";
            break;
        case IORegister::SELECT:
            PLOG_INFO << "SELECT write (data = " << std::hex << data << ")";

            regs.select.raw = data;
            break;
        case IORegister::REFRESH:
            PLOG_INFO << "REFRESH write (data = " << std::hex << data << ")";

            regs.refresh.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

void writeRDRAM(const u64 ioaddr, const u32 data) {
    const u64 moduleAddr = getRDRAMAddress(ioaddr);

    // Find RDRAM module
    Module *module = NULL;

    u64 idx = 0;
    for (; idx < MODULE_NUM; idx++) {
        if ((moduleAddr >> 20) == modules[idx].devID.getID()) {
            module = &modules[idx];

            break;
        }
    }

    if (module == NULL) {
        PLOG_ERROR << "No module responded to write (address = " << std::hex << ioaddr << ", module address = " << moduleAddr << ", data = " << data << ")";

        return;
    }

    switch (ioaddr & ADDRESS_MASK) {
        case RDRAMRegister::DeviceID:
            {
                PLOG_INFO << "DeviceID write (data = " << std::hex << data << ")";

                DeviceID &devID = module->devID;

                devID.raw = data;

                PLOG_VERBOSE << "Module " << idx << " device ID = " << std::hex << devID.getID();
            }
            break;
        case RDRAMRegister::Mode:
            {
                PLOG_INFO << "Mode write (data = " << std::hex << data << ")";

                module->mode.raw = data;

                PLOG_VERBOSE << "Module " << idx << " mode = " << std::hex << data;
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized RDRAM IO write (address = " << std::hex << ioaddr << ", module address = " << moduleAddr << ", data = " << data << ")";

            exit(0);
    }
}

void writeRDRAMBroadcast(const u64 ioaddr, const u32 data) {
    switch (ioaddr & ~(1 << 19)) {
        case RDRAMRegister::DeviceID:
            {
                PLOG_INFO << "Broadcast DeviceID write (data = " << std::hex << data << ")";

                for (u64 i = 0; i < MODULE_NUM; i++) {
                    DeviceID &devID = modules[i].devID;

                    devID.raw = data;

                    PLOG_VERBOSE << "Module " << i << " device ID = " << std::hex << devID.getID();
                }
            }
            break;
        case RDRAMRegister::Delay:
            {
                PLOG_INFO << "Broadcast Delay write (data = " << std::hex << data << ")";

                for (u64 i = 0; i < MODULE_NUM; i++) {
                    Delay &delay = modules[i].delay;

                    if (delay.raw == 0) {
                        // IPL3 rotates this value by 16 during boot, dirty RDRAM hack
                        delay.raw = (data << 16) | (data >> 16);
                    } else {
                        delay.raw = data;
                    }

                    PLOG_VERBOSE << "Module " << i << " delays (write = " << delay.writeDelay << ", ACK = " << delay.ackDelay << ", read = " << delay.readDelay << ", ACK window = " << delay.ackWinDelay << ")";
                }
            }
            break;
        case RDRAMRegister::Mode:
            {
                PLOG_INFO << "Broadcast Mode write (data = " << std::hex << data << ")";

                for (u64 i = 0; i < MODULE_NUM; i++) {
                    for (u64 i = 0; i < MODULE_NUM; i++) {
                        modules[i].mode.raw = data;
                    }
                }
            }
            break;
        case RDRAMRegister::RefRow:
            {
                PLOG_INFO << "Broadcast RefRow write (data = " << std::hex << data << ")";

                for (u64 i = 0; i < MODULE_NUM; i++) {
                    modules[i].refRow.raw = data;
                }
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized broadcast RDRAM IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
