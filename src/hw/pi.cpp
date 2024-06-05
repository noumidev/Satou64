/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pi.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/mi.hpp"

#include "sys/memory.hpp"

namespace hw::pi {

union BSDLAT {
    u32 raw;
    struct {
        u32 latch : 8;
        u32 : 24;
    };
};

union BSDPWD {
    u32 raw;
    struct {
        u32 pulseWidth : 8;
        u32 : 24;
    };
};

union BSDPGS {
    u32 raw;
    struct {
        u32 pageSize : 4;
        u32 : 28;
    };
};

union BSDRLS {
    u32 raw;
    struct {
        u32 release : 2;
        u32 : 30;
    };
};

struct Domain {
    BSDLAT bsdlat;
    BSDPWD bsdpwd;
    BSDPGS bsdpgs;
    BSDRLS bsdrls;
};

union DRAMADDR {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

struct CARTADDR {
    u32 addr;
};

union WRLEN {
    u32 raw;
    struct {
        u32 len : 24;
        u32 : 8;
    };
};

union STATUS {
    u32 raw;
    struct {
        u32 dmaBusy : 1;
        u32 ioBusy : 1;
        u32 error : 1;
        u32 : 29;
    };
};

struct Registers {
    DRAMADDR dramaddr;
    CARTADDR cartaddr;
    WRLEN wrlen;
    STATUS status;

    Domain dom[2];
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

void doDMAToRAM() {
    const u32 cartaddr = regs.cartaddr.addr;
    const u32 dramaddr = regs.dramaddr.addr;
    const u32 len = regs.wrlen.len + 1;

    PLOG_VERBOSE << "DMA to RAM (cart address = " << std::hex << cartaddr << ", DRAM address = " << dramaddr << ", length = " << len << ")";

    const u8 *src = sys::memory::getPointer(cartaddr);
    u8 *dst = sys::memory::getPointer(dramaddr);

    std::memcpy(dst, src, len);

    regs.status.dmaBusy = 0;

    mi::requestInterrupt(mi::InterruptSource::PI);
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::STATUS:
            PLOG_INFO << "STATUS read";

            return regs.status.raw;
        case IORegister::BSDDOM1LAT:
            PLOG_INFO << "BSDDOM1LAT read";

            return regs.dom[0].bsdlat.raw;
        case IORegister::BSDDOM1PWD:
            PLOG_INFO << "BSDDOM1PWD read";

            return regs.dom[0].bsdpwd.raw;
        case IORegister::BSDDOM1PGS:
            PLOG_INFO << "BSDDOM1PGS read";

            return regs.dom[0].bsdpgs.raw;
        case IORegister::BSDDOM1RLS:
            PLOG_INFO << "BSDDOM1RLS read";

            return regs.dom[0].bsdrls.raw;
        case IORegister::BSDDOM2LAT:
            PLOG_INFO << "BSDDOM2LAT read";

            return regs.dom[1].bsdlat.raw;
        case IORegister::BSDDOM2PWD:
            PLOG_INFO << "BSDDOM2PWD read";

            return regs.dom[1].bsdpwd.raw;
        case IORegister::BSDDOM2PGS:
            PLOG_INFO << "BSDDOM2PGS read";

            return regs.dom[1].bsdpgs.raw;
        case IORegister::BSDDOM2RLS:
            PLOG_INFO << "BSDDOM2RLS read";

            return regs.dom[1].bsdrls.raw;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::DRAMADDR:
            PLOG_INFO << "DRAMADDR write (data = " << std::hex << data << ")";

            regs.dramaddr.raw = data;
            break;
        case IORegister::CARTADDR:
            PLOG_INFO << "CARTADDR write (data = " << std::hex << data << ")";

            regs.cartaddr.addr = data;
            break;
        case IORegister::WRLEN:
            PLOG_INFO << "WRLEN write (data = " << std::hex << data << ")";

            regs.wrlen.len = data;

            doDMAToRAM();
            break;
        case IORegister::STATUS:
            PLOG_INFO << "STATUS write (data = " << std::hex << data << ")";

            if ((data & 1) != 0) {
                PLOG_VERBOSE << "DMA controller reset";
            }

            if ((data & 2) != 0) {
                PLOG_INFO << "Interrupt flag cleared";

                mi::clearInterrupt(mi::InterruptSource::PI);
            }
            break;
        case IORegister::BSDDOM1LAT:
            PLOG_INFO << "BSDDOM1LAT write (data = " << std::hex << data << ")";

            regs.dom[0].bsdlat.raw = data;
            break;
        case IORegister::BSDDOM1PWD:
            PLOG_INFO << "BSDDOM1PWD write (data = " << std::hex << data << ")";

            regs.dom[0].bsdpwd.raw = data;
            break;
        case IORegister::BSDDOM1PGS:
            PLOG_INFO << "BSDDOM1PGS write (data = " << std::hex << data << ")";

            regs.dom[0].bsdpgs.raw = data;
            break;
        case IORegister::BSDDOM1RLS:
            PLOG_INFO << "BSDDOM1RLS write (data = " << std::hex << data << ")";

            regs.dom[0].bsdrls.raw = data;
            break;
        case IORegister::BSDDOM2LAT:
            PLOG_INFO << "BSDDOM2LAT write (data = " << std::hex << data << ")";

            regs.dom[1].bsdlat.raw = data;
            break;
        case IORegister::BSDDOM2PWD:
            PLOG_INFO << "BSDDOM2PWD write (data = " << std::hex << data << ")";

            regs.dom[1].bsdpwd.raw = data;
            break;
        case IORegister::BSDDOM2PGS:
            PLOG_INFO << "BSDDOM2PGS write (data = " << std::hex << data << ")";

            regs.dom[1].bsdpgs.raw = data;
            break;
        case IORegister::BSDDOM2RLS:
            PLOG_INFO << "BSDDOM2RLS write (data = " << std::hex << data << ")";

            regs.dom[1].bsdrls.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
