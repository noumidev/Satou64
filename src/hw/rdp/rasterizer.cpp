/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/rdp/rasterizer.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "sys/memory.hpp"

namespace hw::rdp::rasterizer {

constexpr u64 NUM_TILE_DESCRIPTORS = 8;
constexpr u64 NUM_TMEM_WORDS = 0x200;

namespace Format {
    enum : u64 {
        RGBA = 0,
        YUV = 1,
        ColorIndexed = 2,
        IntensityAlpha = 3,
        Intensity = 4,
        NumberOfFormats,
    };
}

constexpr const char *FORMAT_NAMES[Format::NumberOfFormats] = {
    "RGBA", "YUV", "Color Indexed", "Intensity Alpha", "Intensity",
};

namespace Size {
    enum : u64 {
        _4BPP = 0,
        _8BPP = 1,
        _16BPP = 2,
        _32BPP = 3,
        NumberOfSizes,
    };
}

constexpr const char *SIZE_NAMES[Size::NumberOfSizes] = {
    "4 BPP", "8 BPP", "16 BPP", "32 BPP",
};

struct Image {
    u64 dramaddr;
    u64 width;
    u64 size;
    u64 format;
};

// Scissor area
struct Scissor {
    u64 x0, y0;
    u64 x1, y1;
};

struct TileDescriptor {
    struct {
        u64 shift, mask;

        bool mirrorEnable, clampEnable;
    } s, t;

    u64 paletteIndex;
    u64 tmemAddr;
    u64 lineLength;
    u64 size;
    u64 format;
};

struct Context {
    Image colorImage, textureImage;
    Scissor scissor;

    TileDescriptor tileDescriptors[NUM_TILE_DESCRIPTORS];
};

Context ctx;

std::array<u64, NUM_TMEM_WORDS> tmem;

void init() {}

void deinit() {}

void reset() {
    std::memset(&ctx, 0, sizeof(Context));
}

template<>
u64 readTMEM<Size::_4BPP>(const u64 tmemAddr, const u64 x, const u64 y, const u64 width) {
    const u64 tmemIndex = (width * y / 16) + (x / 16);

    return (tmem[tmemAddr + tmemIndex] >> (4 * (15 - (x & 15)))) & 0xF;
}

template<>
u64 readTMEM<Size::_16BPP>(const u64 tmemAddr, const u64 x, const u64 y, const u64 width) {
    const u64 tmemIndex = (width * y / 4) + (x / 4);

    return (tmem[tmemAddr + tmemIndex] >> (16 * (3 - (x & 3)))) & 0xFFFF;
}

template<>
void loadTMEM<Format::RGBA, Size::_16BPP, false>(const u64 dramaddr, const u64 tmemAddr, const u64 x0, const u64 y0, const u64 x1, const u64 y1, const u64 dramWidth, const u64 tmemWidth) {
    for (u64 y = y0; y <= y1; y++) {
        for (u64 x = x0; x <= (x1 >> 2); x += 4) {
            // Load 4 texels at once
            const u64 texels = sys::memory::read<u64>(dramaddr + 8 * (dramWidth * y + (x / 4)));

            const u64 addr = tmemAddr + tmemWidth * y + (x / 4);

            if (addr >= NUM_TMEM_WORDS) {
                PLOG_FATAL << "Invalid TMEM address " << std::hex << addr;

                exit(0);
            }

            // PLOG_DEBUG << "TMEM[" << std::hex << addr << "] = " << texels << " (x = " << std::dec << x << ", y = " << y << ")";

            tmem[addr] = texels;
        }
    }
}

template<>
void loadTMEM<Format::RGBA, Size::_16BPP, true>(const u64 dramaddr, const u64 tmemAddr, const u64 x0, const u64 y0, const u64 x1, const u64 y1, const u64 dramWidth, const u64 tmemWidth) {
    for (u64 y = y0; y <= y1; y++) {
        for (u64 x = x0; x <= x1; x++) {
            const u64 texel = sys::memory::read<u16>(dramaddr + 2 * (dramWidth * y + x));

            const u64 addr = tmemAddr + tmemWidth * y + x;

            if (addr >= NUM_TMEM_WORDS) {
                PLOG_FATAL << "Invalid TMEM address " << std::hex << addr;

                exit(0);
            }

            // Quadruple texel
            const u64 data = 0x1000100010001 * texel;

            // PLOG_DEBUG << "TMEM[" << std::hex << addr << "] = " << data << " (x = " << std::dec << x << ", y = " << y << ")";

            tmem[addr] = data;
        }
    }
}

void textureRectangle(TextureRectangleHeader header, TextureRectangleParameters params) {
    const i64 s = ((i64)params.s << 48) >> 48;
    const i64 t = ((i64)params.t << 48) >> 48;
    const i64 dsdx = ((i64)params.dsdx << 48) >> 48;
    const i64 dtdy = ((i64)params.dtdy << 48) >> 48;

    PLOG_VERBOSE << "Texture Rectangle (tile index = " << header.tile << ", x0 = " << (header.x0 >> 2) << ", y0 = " << (header.y0 >> 2) << ", x1 = " << (header.x1 >> 2) << ", y1 = " << (header.y1 >> 2) << ", s = " << (s >> 5) << ", t = " << (t >> 5) << ", dsdx = " << (dsdx >> 10) << ", dtdy = " << (dtdy >> 10) << ")";

    TileDescriptor &tile = ctx.tileDescriptors[header.tile];

    if (tile.paletteIndex != 0xD) return;

    i64 v = t;
    for (u64 y = (header.y0 >> 2); y < (header.y1 >> 2); y++) {
        i64 u = s;
        for (u64 x = (header.x0 >> 2); x < (header.x1 >> 2); x++) {
            // Fetch texel
            u64 texel;
            switch (tile.format) {
                case Format::ColorIndexed:
                    switch (tile.size) {
                        case Size::_4BPP:
                            texel = readTMEM<Size::_4BPP>(tile.tmemAddr, u >> 5, v >> 5, tile.lineLength);
                            texel = readTMEM<Size::_16BPP>(0x100 + 16 * tile.paletteIndex, 4 * texel, 0, 1);
                            break;
                        default:
                            PLOG_FATAL << "Unrecognized texture size " << SIZE_NAMES[tile.size];

                            exit(0);
                    }
                    break;
                default:
                    PLOG_FATAL << "Unrecognized texture format " << FORMAT_NAMES[tile.format];

                    exit(0);
            }

            // Write to frame buffer
            if ((ctx.colorImage.format != Format::RGBA) || (ctx.colorImage.size != Size::_16BPP)) {
                PLOG_FATAL << "Unhandled frame buffer configuration";

                exit(0);
            }

            sys::memory::write(ctx.colorImage.dramaddr + 2 * (ctx.colorImage.width * y + x), (u16)texel);

            u = ((u << 5) + dsdx) >> 5;
        }

        v = ((v << 5) + dtdy) >> 5;
    } 
}

void loadTile(const u64 tileIndex, const u64 x0, const u64 y0, const u64 x1, const u64 y1) {
    const TileDescriptor &tile = ctx.tileDescriptors[tileIndex];

    PLOG_VERBOSE << "Load Tile (tile index = " << tileIndex << ", x0 = " << (x0 >> 2) << ", y0 = " << (y0 >> 2) << ", x1 = " << (x1 >> 2) << ", y1 = " << (y1 >> 2) << ")";

    const u64 dramaddr = ctx.textureImage.dramaddr;
    const u64 width = ((x1 - x0) >> 2) + 1;

    switch (tile.format) {
        case Format::RGBA:
            switch (tile.size) {
                case Size::_16BPP:
                    loadTMEM<Format::RGBA, Size::_16BPP, false>(dramaddr, tile.tmemAddr, x0 >> 2, y0 >> 2, x1 >> 2, y1 >> 2, width, tile.lineLength);
                    break;
                default:
                    PLOG_FATAL << "Unrecognized TLUT size " << SIZE_NAMES[tile.size];

                    exit(0);
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized TLUT format " << FORMAT_NAMES[tile.format];

            exit(0);
    }
}

void loadTLUT(const u64 tileIndex, const u64 x0, const u64 y0, const u64 x1, const u64 y1) {
    const TileDescriptor &tile = ctx.tileDescriptors[tileIndex];

    PLOG_VERBOSE << "Load TLUT (tile index = " << tileIndex << ", x0 = " << (x0 >> 2) << ", y0 = " << (y0 >> 2) << ", x1 = " << (x1 >> 2) << ", y1 = " << (y1 >> 2) << ")";

    const u64 dramaddr = ctx.textureImage.dramaddr;
    const u64 width = ((x1 - x0) >> 2) + 1;

    const u64 size = ctx.textureImage.size;
    switch (tile.format) {
        case Format::RGBA:
            switch (size) {
                case Size::_16BPP:
                    loadTMEM<Format::RGBA, Size::_16BPP, true>(dramaddr, tile.tmemAddr, x0 >> 2, y0 >> 2, x1 >> 2, y1 >> 2, width, width);
                    break;
                default:
                    PLOG_FATAL << "Unrecognized tile size " << SIZE_NAMES[size];

                    exit(0);
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized tile format " << FORMAT_NAMES[tile.format];

            exit(0);
    }
}

void setColorImage(const u64 dramaddr, const u64 width, const u64 size, const u64 format) {
    Image &colorImage = ctx.colorImage;

    colorImage.dramaddr = dramaddr;
    colorImage.width = width + 1;
    colorImage.size = size;
    colorImage.format = format;

    PLOG_VERBOSE << "Color image (DRAM address = " << std::hex << dramaddr << ", width = " << std::dec << (width + 1) << ", size = " << (4 << size) << " BPP, format = " << FORMAT_NAMES[format] << ")";
}

void setScissor(const u64 x0, const u64 y0, const u64 x1, const u64 y1) {
    Scissor &scissor = ctx.scissor;

    scissor.x0 = x0;
    scissor.y0 = y0;
    scissor.x1 = x1;
    scissor.y1 = y1;

    PLOG_VERBOSE << "Scissor area (x0 = " << (x0 >> 2) << ", y0 = " << (y0 >> 2) << ", x1 = " << (x1 >> 2) << ", y1 = " << (y1 >> 2) << ")";
}

void setTextureImage(const u64 dramaddr, const u64 width, const u64 size, const u64 format) {
    Image &textureImage = ctx.textureImage;

    textureImage.dramaddr = dramaddr;
    textureImage.width = width + 1;
    textureImage.size = size;
    textureImage.format = format;

    PLOG_VERBOSE << "Texture image (DRAM address = " << std::hex << dramaddr << ", width = " << std::dec << (width + 1) << ", size = " << (4 << size) << " BPP, format = " << FORMAT_NAMES[format] << ")";
}

void setTile(const SetTileHeader header) {
    const u64 tileIndex = header.index;

    TileDescriptor &tile = ctx.tileDescriptors[tileIndex];

    tile.s.shift = header.shiftS;
    tile.s.mask = header.maskS;
    tile.s.clampEnable = header.clampS != 0;
    tile.s.mirrorEnable = header.mirrorS != 0;

    tile.t.shift = header.shiftT;
    tile.t.mask = header.maskT;
    tile.t.clampEnable = header.clampT != 0;
    tile.t.mirrorEnable = header.mirrorT != 0;

    tile.paletteIndex = header.palette;
    tile.tmemAddr = header.address;
    tile.lineLength = header.line;
    tile.size = header.size;
    tile.format = header.format;

    PLOG_VERBOSE << "Tile " << tileIndex << " (S shift = " << header.shiftS << ", S mask = " << header.maskS << ", S mirror = " << header.mirrorS << ", S clamp = " << header.clampS << ", T shift = " << header.shiftT << ", T mask = " << header.maskT << ", T mirror = " << header.mirrorT << ", T clamp = " << header.clampT << ", palette = " << header.palette << ", TMEM address = " << std::hex << header.address << ", line = " << std::dec << header.line << ", size = " << (4 << header.size) << ", format = " << FORMAT_NAMES[header.format] << ")";
}

}
