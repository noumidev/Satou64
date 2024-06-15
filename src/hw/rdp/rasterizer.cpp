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

namespace CombinerRGBInputA {
    enum : u64 {
        Combined,
        Tex0,
        Tex1,
        Primitive,
        Shade,
        Environment,
        Fixed1,
        Noise,
        Fixed0,
        NumberOfCombinerInputs,
    };
}

constexpr const char *RGB_INPUT_A_NAMES[CombinerRGBInputA::NumberOfCombinerInputs] = {
    "COMBINED", "TEX0", "TEX1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "NOISE", "0",
};

namespace CombinerRGBInputB {
    enum : u64 {
        Combined,
        Tex0,
        Tex1,
        Primitive,
        Shade,
        Environment,
        Center,
        K4,
        Fixed0,
        NumberOfCombinerInputs,
    };
}

constexpr const char *RGB_INPUT_B_NAMES[CombinerRGBInputB::NumberOfCombinerInputs] = {
    "COMBINED", "TEX0", "TEX1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "CENTER", "K4", "0",
};

namespace CombinerRGBInputC {
    enum : u64 {
        Combined,
        Tex0,
        Tex1,
        Primitive,
        Shade,
        Environment,
        Center,
        CombinedAlpha,
        Tex0Alpha,
        Tex1Alpha,
        PrimitiveAlpha,
        ShadeAlpha,
        EnvironmentAlpha,
        LODFraction,
        PrimitiveLODFraction,
        K5,
        Fixed0,
        NumberOfCombinerInputs,
    };
}

constexpr const char *RGB_INPUT_C_NAMES[CombinerRGBInputC::NumberOfCombinerInputs] = {
    "COMBINED", "TEX0", "TEX1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "CENTER", "COMBINED_ALPHA",
    "TEX0_ALPHA", "TEX1_ALPHA", "PRIMITIVE_ALPHA", "SHADE_ALPHA", "ENVIRONMENT_ALPHA",
    "LOD_FRACTION", "PRIM_LOD_FRAC", "K5", "0",
};

namespace CombinerAlphaInputABD {
    enum : u64 {
        Combined,
        Tex0,
        Tex1,
        Primitive,
        Shade,
        Environment,
        Fixed1,
        Fixed0,
        NumberOfCombinerInputs,
    };
}

constexpr const char *ALPHA_INPUT_ABD_NAMES[CombinerAlphaInputABD::NumberOfCombinerInputs] = {
    "COMBINED", "TEX0", "TEX1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "0",
};

namespace CombinerRGBInputD {
    enum : u64 {
        Combined,
        Tex0,
        Tex1,
        Primitive,
        Shade,
        Environment,
        Fixed1,
        Fixed0,
        NumberOfCombinerInputs,
    };
}

constexpr const char *RGB_INPUT_D_NAMES[CombinerRGBInputD::NumberOfCombinerInputs] = {
    "COMBINED", "TEX0", "TEX1", "PRIMITIVE", "SHADE", "ENVIRONMENT", "1", "0",
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

    SetCombineModeHeader combineModes;

    TileDescriptor tileDescriptors[NUM_TILE_DESCRIPTORS];

    u32 fillColor;
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
    const u64 tmemIndex = width * y + (x / 16);

    return (tmem[tmemAddr + tmemIndex] >> (4 * (15 - (x & 15)))) & 0xF;
}

template<>
u64 readTMEM<Size::_8BPP>(const u64 tmemAddr, const u64 x, const u64 y, const u64 width) {
    const u64 tmemIndex = width * y + (x / 8);

    return (tmem[tmemAddr + tmemIndex] >> (8 * (7 - (x & 7)))) & 0xFF;
}

template<>
u64 readTMEM<Size::_16BPP>(const u64 tmemAddr, const u64 x, const u64 y, const u64 width) {
    const u64 tmemIndex = width * y + (x / 4);

    return (tmem[tmemAddr + tmemIndex] >> (16 * (3 - (x & 3)))) & 0xFFFF;
}

template<>
void loadTMEM<Format::RGBA, Size::_8BPP, false>(const u64 dramaddr, const u64 tmemAddr, const u64 x0, const u64 y0, const u64 x1, const u64 y1, const u64 dramWidth, const u64 tmemWidth) {
    for (u64 y = y0; y <= y1; y++) {
        for (u64 x = x0; x < ((x1 + 1) / 4); x += 8) {
            // Load 4 texels at once
            const u64 texels = sys::memory::read<u64>(dramaddr + dramWidth * y + 8 * (x / 8));

            const u64 addr = tmemAddr + tmemWidth * (y - y0) + ((x - x0) / 8);

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
void loadTMEM<Format::RGBA, Size::_16BPP, false>(const u64 dramaddr, const u64 tmemAddr, const u64 x0, const u64 y0, const u64 x1, const u64 y1, const u64 dramWidth, const u64 tmemWidth) {
    for (u64 y = y0; y <= y1; y++) {
        for (u64 x = x0; x < ((x1 + 1) / 4); x += 4) {
            // Load 4 texels at once
            const u64 texels = sys::memory::read<u64>(dramaddr + 2 * dramWidth * y + 8 * (x / 4));

            const u64 addr = tmemAddr + tmemWidth * (y - y0) + ((x - x0) / 4);

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

            const u64 addr = tmemAddr + tmemWidth * (y - y0) + (x - x0);

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

u64 getCombinerRGBInputA(const u64 mode, const u64 texel0) {
    if (mode >= CombinerRGBInputA::Fixed0) {
        return 0;
    }

    switch (mode) {
        case CombinerRGBInputA::Tex0:
            return texel0;
        default:
            PLOG_FATAL << "Unrecognized combiner RGB A input " << RGB_INPUT_A_NAMES[mode];

            exit(0);
    }
}

u64 getCombinerRGBInputB(const u64 mode, const u64 texel0) {
    if (mode >= CombinerRGBInputB::Fixed0) {
        return 0;
    }

    switch (mode) {
        default:
            PLOG_FATAL << "Unrecognized combiner RGB B input " << RGB_INPUT_B_NAMES[mode];

            exit(0);
    }
}

u64 getCombinerRGBInputC(const u64 mode, const u64 texel0) {
    if (mode >= CombinerRGBInputC::Fixed0) {
        return 0;
    }

    switch (mode) {
        case CombinerRGBInputC::CombinedAlpha:
            return 1;
        default:
            PLOG_FATAL << "Unrecognized combiner RGB C input " << RGB_INPUT_C_NAMES[mode];

            exit(0);
    }
}

u64 getCombinerRGBInputD(const u64 mode, const u64 texel0) {
    switch (mode) {
        case CombinerRGBInputD::Fixed0:
            return 0;
        default:
            PLOG_FATAL << "Unrecognized combiner RGB D input " << RGB_INPUT_D_NAMES[mode];

            exit(0);
    }
}

template<>
u64 combine2ndCycle<Size::_16BPP>(const u64 texel0) {
    const SetCombineModeHeader &combineModes = ctx.combineModes;

    // Get combiner inputs
    const u64 a = getCombinerRGBInputA(combineModes.rgbA2ndCycle, texel0);
    const u64 b = getCombinerRGBInputB(combineModes.rgbB2ndCycle, texel0);
    const u64 c = getCombinerRGBInputC(combineModes.rgbC2ndCycle, texel0);
    const u64 d = getCombinerRGBInputD(combineModes.rgbD2ndCycle, texel0);

    return texel0;
}

void fillRectangle(const u64 x0, const u64 y0, const u64 x1, const u64 y1) {
    PLOG_VERBOSE << "Fill Rectangle (x0 = " << (x0 >> 2) << ", y0 = " << (y0 >> 2) << ", x1 = " << (x1 >> 2) << ", y1 = " << (y1 >> 2) << ")";

    const u16 fillColor = (u16)ctx.fillColor;

    for (u64 y = (y0 >> 2); y < (y1 >> 2); y++) {
        for (u64 x = (x0 >> 2); x < (x1 >> 2); x++) {
            sys::memory::write(ctx.colorImage.dramaddr + 2 * (ctx.colorImage.width * y + x), fillColor);
        }
    }
}

void textureRectangle(TextureRectangleHeader header, TextureRectangleParameters params) {
    const i64 s = ((i64)params.s << 48) >> 48;
    const i64 t = ((i64)params.t << 48) >> 48;
    i64 dsdx = ((i64)params.dsdx << 48) >> 48;
    const i64 dtdy = ((i64)params.dtdy << 48) >> 48;

    if ((dsdx >> 10) == 4) dsdx = 1 << 10;

    PLOG_VERBOSE << "Texture Rectangle (tile index = " << header.tile << ", x0 = " << (header.x0 >> 2) << ", y0 = " << (header.y0 >> 2) << ", x1 = " << (header.x1 >> 2) << ", y1 = " << (header.y1 >> 2) << ", s = " << (s >> 5) << ", t = " << (t >> 5) << ", dsdx = " << (dsdx >> 10) << ", dtdy = " << (dtdy >> 10) << ")";

    TileDescriptor &tile = ctx.tileDescriptors[header.tile];

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
                        case Size::_8BPP:
                            texel = readTMEM<Size::_8BPP>(tile.tmemAddr, u >> 5, v >> 5, tile.lineLength);
                            texel = readTMEM<Size::_16BPP>(0x100, 4 * texel, 0, 1);
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

            // Major hack to get a demo running, I'll implement the blender soon...
            if (tile.paletteIndex != 0xD) {
                const u64 oldColor = sys::memory::read<u16>(ctx.colorImage.dramaddr + 2 * (ctx.colorImage.width * y + x));

                u64 b = (texel >>  1) & 0x1F;
                u64 g = (texel >>  6) & 0x1F;
                u64 r = (texel >> 11) & 0x1F;

                const u64 oldB = (oldColor >>  1) & 0x1F;
                const u64 oldG = (oldColor >>  6) & 0x1F;
                const u64 oldR = (oldColor >> 11) & 0x1F;

                b += oldB;
                g += oldG;
                r += oldR;

                if (b > 0x1F) {
                    b = 0x1F;
                }

                if (g > 0x1F) {
                    g = 0x1F;
                }

                if (r > 0x1F) {
                    r = 0x1F;
                }

                u64 newColor = texel & 1;
                newColor |= b <<  1;
                newColor |= g <<  6;
                newColor |= r << 11;

                sys::memory::write(ctx.colorImage.dramaddr + 2 * (ctx.colorImage.width * y + x), (u16)newColor);
            } else {
                sys::memory::write(ctx.colorImage.dramaddr + 2 * (ctx.colorImage.width * y + x), (u16)texel);
            }

            

            u = ((u << 5) + dsdx) >> 5;
        }

        v = ((v << 5) + dtdy) >> 5;
    }
}

void loadTile(const u64 tileIndex, const u64 x0, const u64 y0, const u64 x1, const u64 y1) {
    const TileDescriptor &tile = ctx.tileDescriptors[tileIndex];

    PLOG_VERBOSE << "Load Tile (tile index = " << tileIndex << ", x0 = " << (x0 >> 2) << ", y0 = " << (y0 >> 2) << ", x1 = " << (x1 >> 2) << ", y1 = " << (y1 >> 2) << ")";

    const u64 dramaddr = ctx.textureImage.dramaddr;

    switch (tile.format) {
        case Format::RGBA:
        case Format::ColorIndexed:
            switch (tile.size) {
                case Size::_8BPP:
                    loadTMEM<Format::RGBA, Size::_8BPP, false>(dramaddr, tile.tmemAddr, x0 >> 2, y0 >> 2, x1 >> 2, y1 >> 2, ctx.textureImage.width, tile.lineLength);
                    break;
                case Size::_16BPP:
                    loadTMEM<Format::RGBA, Size::_16BPP, false>(dramaddr, tile.tmemAddr, x0 >> 2, y0 >> 2, x1 >> 2, y1 >> 2, ctx.textureImage.width, tile.lineLength);
                    break;
                default:
                    PLOG_FATAL << "Unrecognized tile size " << SIZE_NAMES[tile.size];

                    exit(0);
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized tile format " << FORMAT_NAMES[tile.format];

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
                    PLOG_FATAL << "Unrecognized TLUT size " << SIZE_NAMES[size];

                    exit(0);
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized TLUT format " << FORMAT_NAMES[tile.format];

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

void setCombineMode(const SetCombineModeHeader header) {
    std::memcpy(&ctx.combineModes, &header, sizeof(SetCombineModeHeader));
}

void setFillColor(const u32 fillColor) {
    ctx.fillColor = fillColor;

    PLOG_VERBOSE << "Fill color = " << std::hex << fillColor;
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
