/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/vi.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "renderer/renderer.hpp"

namespace hw::vi {

union CONTROL {
    u32 raw;
    struct {
        u32 type : 2;
        u32 gammaDitherEnable : 1;
        u32 gammaEnable : 1;
        u32 divotEnable : 1;
        u32 : 1;
        u32 serrate : 1;
        u32 : 1;
        u32 aaMode : 2;
        u32 : 22;
    };
};

union ORIGIN {
    u32 raw;
    struct {
        u32 addr : 24;
        u32 : 8;
    };
};

union WIDTH {
    u32 raw;
    struct {
        u32 width : 12;
        u32 : 20;
    };
};

union INTR {
    u32 raw;
    struct {
        u32 line : 10;
        u32 : 22;
    };
};

union CURRENT {
    u32 raw;
    struct {
        u32 currentHalfline : 10;
        u32 : 22;
    };
};

union BURST {
    u32 raw;
    struct {
        u32 hsyncWidth : 8;
        u32 colorBurstWidth : 8;
        u32 vsyncWidth : 4;
        u32 colorBurstStart : 10;
        u32 : 2;
    };
};

union VSYNC {
    u32 raw;
    struct {
        u32 halflines : 10;
        u32 : 22;
    };
};

union HSYNC {
    u32 raw;
    struct {
        u32 lineDuration : 12;
        u32 : 4;
        u32 leapPattern : 5;
        u32 : 11;
    };
};

union LEAP {
    u32 raw;
    struct {
        u32 hsyncPeriod0 : 12;
        u32 : 4;
        u32 hsyncPeriod1 : 12;
        u32 : 4;
    };
};

union HSTART {
    u32 raw;
    struct {
        u32 end : 10;
        u32 : 6;
        u32 start : 12;
        u32 : 4;
    };
};

union VSTART {
    u32 raw;
    struct {
        u32 end : 10;
        u32 : 6;
        u32 start : 12;
        u32 : 4;
    };
};

union VBURST {
    u32 raw;
    struct {
        u32 end : 10;
        u32 : 6;
        u32 start : 12;
        u32 : 4;
    };
};

union XSCALE {
    u32 raw;
    struct {
        u32 scaleUpFactor : 12;
        u32 : 4;
        u32 subpixelOffset : 12;
        u32 : 4;
    };
};

union YSCALE {
    u32 raw;
    struct {
        u32 scaleUpFactor : 12;
        u32 : 4;
        u32 subpixelOffset : 12;
        u32 : 4;
    };
};

struct Registers {
    CONTROL control;
    ORIGIN origin;
    WIDTH width;
    INTR intr;
    CURRENT current;
    BURST burst;
    VSYNC vsync;
    HSYNC hsync;
    LEAP leap;
    HSTART hstart;
    VSTART vstart;
    VBURST vburst;
    XSCALE xscale;
    YSCALE yscale;
};

Registers regs;

void init() {}

void deinit() {}

void reset() {
    std::memset(&regs, 0, sizeof(Registers));
}

u32 readIO(const u64 ioaddr) {
    switch (ioaddr) {
        case IORegister::CURRENT:
            PLOG_INFO << "CURRENT read";

            return regs.current.currentHalfline;
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

u32 getFormat() {
    return regs.control.type;
}

u32 getOrigin() {
    return regs.origin.addr;
}

void writeIO(const u64 ioaddr, const u32 data) {
    switch (ioaddr) {
        case IORegister::CONTROL:
            PLOG_INFO << "CONTROL write (data = " << std::hex << data << ")";

            regs.control.raw = data;
            break;
        case IORegister::ORIGIN:
            PLOG_INFO << "ORIGIN write (data = " << std::hex << data << ")";

            regs.origin.raw = data;
            break;
        case IORegister::WIDTH:
            PLOG_INFO << "WIDTH write (data = " << std::hex << data << ")";

            regs.width.raw = data;

            renderer::changeResolution(regs.width.width);
            break;
        case IORegister::INTR:
            PLOG_INFO << "INTR write (data = " << std::hex << data << ")";

            regs.intr.raw = data;
            break;
        case IORegister::CURRENT:
            PLOG_INFO << "CURRENT write (data = " << std::hex << data << ")";

            // TODO: clear VI interrupt
            break;
        case IORegister::BURST:
            PLOG_INFO << "BURST write (data = " << std::hex << data << ")";

            regs.burst.raw = data;
            break;
        case IORegister::VSYNC:
            PLOG_INFO << "VSYNC write (data = " << std::hex << data << ")";

            regs.vsync.raw = data;
            break;
        case IORegister::HSYNC:
            PLOG_INFO << "HSYNC write (data = " << std::hex << data << ")";

            regs.hsync.raw = data;
            break;
        case IORegister::LEAP:
            PLOG_INFO << "LEAP write (data = " << std::hex << data << ")";

            regs.leap.raw = data;
            break;
        case IORegister::HSTART:
            PLOG_INFO << "HSTART write (data = " << std::hex << data << ")";

            regs.hstart.raw = data;
            break;
        case IORegister::VSTART:
            PLOG_INFO << "VSTART write (data = " << std::hex << data << ")";

            regs.vstart.raw = data;
            break;
        case IORegister::VBURST:
            PLOG_INFO << "VBURST write (data = " << std::hex << data << ")";

            regs.vburst.raw = data;
            break;
        case IORegister::XSCALE:
            PLOG_INFO << "XSCALE write (data = " << std::hex << data << ")";

            regs.xscale.raw = data;
            break;
        case IORegister::YSCALE:
            PLOG_INFO << "YSCALE write (data = " << std::hex << data << ")";

            regs.yscale.raw = data;
            break;
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
