/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/rdp/rdp.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "hw/mi.hpp"
#include "hw/rdp/rasterizer.hpp"

#include "sys/memory.hpp"

namespace hw::rdp {

namespace Command {
    enum : u64 {
        TextureRectangle = 0x24,
        SyncLoad = 0x26,
        SyncPipe = 0x27,
        SyncTile = 0x28,
        SyncFull = 0x29,
        SetScissor = 0x2D,
        SetOtherModes = 0x2F,
        LoadTLUT = 0x30,
        LoadTile = 0x34,
        SetTile = 0x35,
        SetCombineMode = 0x3C,
        SetTextureImage = 0x3D,
        SetColorImage = 0x3F,
    };
}

union LoadTLUTHeader {
    u64 raw;
    struct {
        u64 y1 : 12;
        u64 x1 : 12;
        u64 tile : 3;
        u64 : 5;
        u64 y0 : 12;
        u64 x0 : 12;
        u64 : 8;
    };
};

union SetImageHeader {
    u64 raw;
    struct {
        u64 dramaddr : 24;
        u64 : 8;
        u64 width : 10;
        u64 : 9;
        u64 size : 2;
        u64 format : 3;
        u64 : 8;
    };
};

union SetScissorHeader {
    u64 raw;
    struct {
        u64 y1 : 12;
        u64 x1 : 12;
        u64 y0 : 12;
        u64 x0 : 12;
        u64 : 8;
    };
};

void init() {}

void deinit() {}

void reset() {}

u64 processCommandList(const u64 startAddr, const u64 endAddr) {
    PLOG_VERBOSE << "RDP command list (start address = " << std::hex << startAddr << ", end address = " << endAddr << ")";

    if (startAddr >= endAddr) {
        PLOG_WARNING << "Empty command list";

        return startAddr;
    }

    u64 addr = startAddr;

    for (; addr < endAddr; addr += 8) {
        // Read first command word
        const u64 data = sys::memory::read<u64>(addr);

        const u64 command = (data >> 56) & 0x3F;
        switch (command) {
            case Command::TextureRectangle:
                addr += 8;

                cmdTextureRectangle(data, sys::memory::read<u64>(addr));
                break;
            case Command::SyncLoad:
                cmdSyncLoad(data);
                break;
            case Command::SyncPipe:
                cmdSyncPipe(data);
                break;
            case Command::SyncTile:
                cmdSyncTile(data);
                break;
            case Command::SyncFull:
                cmdSyncFull(data);
                break;
            case Command::SetScissor:
                cmdSetScissor(data);
                break;
            case Command::SetOtherModes:
                cmdSetOtherModes(data);
                break;
            case Command::LoadTLUT:
                cmdLoadTLUT(data);
                break;
            case Command::LoadTile:
                cmdLoadTile(data);
                break;
            case Command::SetTile:
                cmdSetTile(data);
                break;
            case Command::SetCombineMode:
                cmdSetCombineMode(data);
                break;
            case Command::SetTextureImage:
                cmdSetTextureImage(data);
                break;
            case Command::SetColorImage:
                cmdSetColorImage(data);
                break;
            default:
                PLOG_FATAL << "Unrecognized command " << std::hex << command << " (command word = " << data << ", address = " << addr << ")";

                exit(0);
        }
    }

    return addr;
}

void cmdLoadTile(const u64 data) {
    PLOG_VERBOSE << "Load Tile (command word = " << std::hex << data << ")";

    // Load Tile uses the same command header
    LoadTLUTHeader header{.raw = data};

    rasterizer::loadTile(header.tile, header.x0, header.y0, header.x1, header.y1);
}

void cmdLoadTLUT(const u64 data) {
    PLOG_VERBOSE << "Load TLUT (command word = " << std::hex << data << ")";

    LoadTLUTHeader header{.raw = data};

    rasterizer::loadTLUT(header.tile, header.x0, header.y0, header.x1, header.y1);
}

void cmdSetColorImage(const u64 data) {
    PLOG_VERBOSE << "Set Color Image (command word = " << std::hex << data << ")";

    SetImageHeader header{.raw = data};

    rasterizer::setColorImage(header.dramaddr, header.width, header.size, header.format);
}

void cmdSetCombineMode(const u64 data) {
    PLOG_VERBOSE << "Set Combine Mode (command word = " << std::hex << data << ")";
}

void cmdSetOtherModes(const u64 data) {
    PLOG_VERBOSE << "Set Other Modes (command word = " << std::hex << data << ")";
}

void cmdSetScissor(const u64 data) {
    PLOG_VERBOSE << "Set Scissor (command word = " << std::hex << data << ")";

    SetScissorHeader header{.raw = data};

    rasterizer::setScissor(header.x0, header.y0, header.x1, header.y1);
}

void cmdSetTextureImage(const u64 data) {
    PLOG_VERBOSE << "Set Texture Image (command word = " << std::hex << data << ")";

    SetImageHeader header{.raw = data};

    rasterizer::setTextureImage(header.dramaddr, header.width, header.size, header.format);
}

void cmdSetTile(const u64 data) {
    PLOG_VERBOSE << "Set Tile (command word = " << std::hex << data << ")";

    rasterizer::SetTileHeader header{.raw = data};

    rasterizer::setTile(header);
}

void cmdSyncFull(const u64 data) {
    PLOG_VERBOSE << "Sync Full (command word = " << std::hex << data << ")";
    
    mi::requestInterrupt(mi::InterruptSource::DP);
}

void cmdSyncLoad(const u64 data) {
    PLOG_VERBOSE << "Sync Load (command word = " << std::hex << data << ")";
}

void cmdSyncPipe(const u64 data) {
    PLOG_VERBOSE << "Sync Pipe (command word = " << std::hex << data << ")";
}

void cmdSyncTile(const u64 data) {
    PLOG_VERBOSE << "Sync Tile (command word = " << std::hex << data << ")";
}

void cmdTextureRectangle(const u64 data, const u64 next) {
    PLOG_VERBOSE << "Texture Rectangle (command words = " << std::hex << data << ", " << next << ")";

    rasterizer::TextureRectangleHeader header{.raw = data};
    rasterizer::TextureRectangleParameters params{.raw = next};

    rasterizer::textureRectangle(header, params);
}

}
