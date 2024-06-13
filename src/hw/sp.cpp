/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/sp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/mi.hpp"
#include "hw/rsp/rsp.hpp"

#include "sys/memory.hpp"

namespace hw::sp {

constexpr u32 SIG_NUM = 8;

namespace RegisterMask {
    enum : u32 {
        SPADDR = 0x00001FF8,
        RAMADDR = 0x00FFFFF8,
        RDLEN = 0xFF8FFFF8,
    };
}

union SPADDR {
    u32 raw;
    struct {
        u32 : 3;
        u32 addr : 9;
        u32 isIMEM : 1;
        u32 : 19;
    };
};

union RAMADDR {
    u32 raw;
    struct {
        u32 : 3;
        u32 addr : 21;
        u32 : 8;
    };
};

union LEN {
    u32 raw;
    struct {
        u32 : 3;
        u32 rdlen : 9;
        u32 count : 8;
        u32 : 3;
        u32 skip : 9;
    };
};

union STATUS {
    u32 raw;
    struct {
        u32 halted : 1;
        u32 broke : 1;
        u32 dmaBusy : 1;
        u32 dmaFull : 1;
        u32 ioBusy : 1;
        u32 singleStep : 1;
        u32 interruptOnBreak : 1;
        u32 sig : 8;
        u32 : 17;
    };
};

struct Registers {
    SPADDR spaddr;
    RAMADDR ramaddr;
    LEN rdlen;
    LEN wrlen;
    STATUS status;

    bool semaphore;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));

    regs.status.halted = 1;
}

void BREAK() {
    STATUS &status = regs.status;

    status.halted = 1;
    status.broke = 1;

    if (status.interruptOnBreak) {
        mi::requestInterrupt(mi::InterruptSource::SP);
    }
}

bool isHalted() {
    return regs.status.halted != 0;
}

void doDMAToRAM() {
    const u64 dramaddr = regs.ramaddr.addr << 3;
    u64 rspAddr = regs.spaddr.addr;

    const u64 length = regs.wrlen.rdlen + 1;
    const u64 count = regs.wrlen.count + 1;
    const u64 skip = regs.wrlen.skip;

    // Casting this to u64 * is fine because the address is guaranteed
    // to be 64-bit aligned
    u64 *dram = (u64 *)sys::memory::getPointer(dramaddr);

    u64 *spmem;
    if (regs.spaddr.isIMEM) {
        PLOG_VERBOSE << "DMA from RSP IMEM (RSP address = " << std::hex << rspAddr << ", DRAM address = " << dramaddr << ", length = " << std::dec << length << ", count = " << count << ", skip = " << skip << ")";

        spmem = (u64 *)sys::memory::getPointer(sys::memory::MemoryBase::RSP_IMEM);
    } else {
        PLOG_VERBOSE << "DMA from RSP DMEM (RSP address = " << std::hex << rspAddr << ", DRAM address = " << dramaddr << ", length = " << std::dec << length << ", count = " << count << ", skip = " << skip << ")";

        spmem = (u64 *)sys::memory::getPointer(sys::memory::MemoryBase::RSP_DMEM);
    }

    for (u64 c = 0; c < count; c++) {
        for (u64 i = 0; i < length; i++) {
            *dram++ = spmem[rspAddr++];

            rspAddr &= 0x1FF;
        }

        dram += skip;
    }

    // Write final register values
    regs.ramaddr.addr = (dramaddr >> 3) + (count - 1) * (length + skip) + length;
    regs.spaddr.addr = rspAddr;

    regs.wrlen.rdlen = 0xFF8 >> 3;
    regs.wrlen.count = 0;
}

void doDMAToRSP() {
    const u64 dramaddr = regs.ramaddr.addr << 3;
    u64 rspAddr = regs.spaddr.addr;

    const u64 length = regs.rdlen.rdlen + 1;
    const u64 count = regs.rdlen.count + 1;
    const u64 skip = regs.rdlen.skip;

    // Casting this to u64 * is fine because the address is guaranteed
    // to be 64-bit aligned
    u64 *dram = (u64 *)sys::memory::getPointer(dramaddr);

    u64 *spmem;
    if (regs.spaddr.isIMEM) {
        PLOG_VERBOSE << "DMA to RSP IMEM (RSP address = " << std::hex << rspAddr << ", DRAM address = " << dramaddr << ", length = " << std::dec << length << ", count = " << count << ", skip = " << skip << ")";

        spmem = (u64 *)sys::memory::getPointer(sys::memory::MemoryBase::RSP_IMEM);
    } else {
        PLOG_VERBOSE << "DMA to RSP DMEM (RSP address = " << std::hex << rspAddr << ", DRAM address = " << dramaddr << ", length = " << std::dec << length << ", count = " << count << ", skip = " << skip << ")";

        spmem = (u64 *)sys::memory::getPointer(sys::memory::MemoryBase::RSP_DMEM);
    }

    for (u64 c = 0; c < count; c++) {
        for (u64 i = 0; i < length; i++) {
            spmem[rspAddr++] = *dram++;

            rspAddr &= 0x1FF;
        }

        dram += skip;
    }

    // Write final register values
    regs.ramaddr.addr = (dramaddr >> 3) + (count - 1) * (length + skip) + length;
    regs.spaddr.addr = rspAddr;

    regs.rdlen.rdlen = 0xFF8 >> 3;
    regs.rdlen.count = 0;
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::SPADDR:
            PLOG_INFO << "SPADDR read";

            return regs.spaddr.raw;
        case IORegister::RAMADDR:
            PLOG_INFO << "RAMADDR read";

            return regs.ramaddr.raw;
        case IORegister::RDLEN:
            PLOG_INFO << "RDLEN read";

            return regs.rdlen.raw;
        case IORegister::WRLEN:
            PLOG_INFO << "WRLEN read";

            return regs.wrlen.raw;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS read";

            return regs.status.raw;
        case IORegister::DMAFULL:
            PLOG_INFO << "DMAFULL read";

            return regs.status.dmaFull;
        case IORegister::DMABUSY:
            PLOG_INFO << "DMABUSY read";

            return regs.status.dmaBusy;
        case IORegister::SEMAPHORE:
            {
                PLOG_INFO << "SEMAPHORE read";

                const u32 data = (u32)regs.semaphore;

                regs.semaphore = true;

                return data;
            }
        case IORegister::PC:
            PLOG_WARNING << "PC read";

            return 0;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::SPADDR:
            PLOG_INFO << "SPADDR write (data = " << std::hex << data << ")";

            regs.spaddr.raw = data & RegisterMask::SPADDR;
            break;
        case IORegister::RAMADDR:
            PLOG_INFO << "RAMADDR write (data = " << std::hex << data << ")";

            regs.ramaddr.raw = data & RegisterMask::RAMADDR;
            break;
        case IORegister::RDLEN:
            PLOG_INFO << "RDLEN write (data = " << std::hex << data << ")";

            regs.rdlen.raw = data & RegisterMask::RDLEN;

            doDMAToRSP();
            break;
        case IORegister::WRLEN:
            PLOG_INFO << "WRLEN write (data = " << std::hex << data << ")";

            regs.wrlen.raw = data & RegisterMask::RDLEN;

            doDMAToRAM();
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";

            switch (data & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "RSP running";

                    regs.status.halted = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "RSP halted";

                    regs.status.halted = 1;
                    break;
            }

            if ((data & (1 << 2)) != 0) {
                PLOG_VERBOSE << "BREAK flag cleared";

                regs.status.broke = 0;
            }

            // TODO: implement RSP interrupt flag
            switch ((data >> 3) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "Interrupt flag cleared";

                    mi::clearInterrupt(mi::InterruptSource::SP);
                    break;
                case 2:
                    mi::requestInterrupt(mi::InterruptSource::SP);
                    break;
            }

            switch ((data >> 5) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "Single step disabled";

                    regs.status.singleStep = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "Single step enabled";

                    regs.status.singleStep = 1;
                    break;
            }

            switch ((data >> 7) & 3) {
                // No change
                case 0:
                case 3:
                    break;
                case 1:
                    PLOG_VERBOSE << "Interrupt on BREAK disabled";

                    regs.status.interruptOnBreak = 0;
                    break;
                case 2:
                    PLOG_VERBOSE << "Interrupt on BREAK enabled";

                    regs.status.interruptOnBreak = 1;
                    break;
            }

            // Signal
            for (u32 signal = 0; signal < SIG_NUM; signal++) {
                switch ((data >> (9 + 2 * signal)) & 3) {
                    // No change
                    case 0:
                    case 3:
                        break;
                    case 1:
                        PLOG_VERBOSE << "Signal " << signal << " disabled";

                        regs.status.sig &= ~(1 << signal);
                        break;
                    case 2:
                        PLOG_VERBOSE << "Signal " << signal << " enabled";

                        regs.status.sig |= 1 << signal;
                        break;
                }
            }
            break;
        case IORegister::SEMAPHORE:
            PLOG_INFO << "SEMAPHORE write (data = " << std::hex << data << ")";

            regs.semaphore = (data & 1) != 0;
            break;
        case IORegister::PC:
            PLOG_INFO << "PC write (data = " << std::hex << data << ")";

            rsp::setPC(data);
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
