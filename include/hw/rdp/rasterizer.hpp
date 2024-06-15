/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::rdp::rasterizer {

union SetCombineModeHeader {
    u64 raw;
    struct {
        u64 alphaD2ndCycle : 3;
        u64 alphaB2ndCycle : 3;
        u64 rgbD2ndCycle : 3;
        u64 alphaD1stCycle : 3;
        u64 alphaB1stCycle : 3;
        u64 rgbD1stCycle : 3;
        u64 alphaC2ndCycle : 3;
        u64 alphaA2ndCycle : 3;
        u64 rgbB2ndCycle : 4;
        u64 rgbB1stCycle : 4;
        u64 rgbC2ndCycle : 5;
        u64 rgbA2ndCycle : 4;
        u64 alphaC1stCycle : 3;
        u64 alphaA1stCycle : 3;
        u64 rgbC1stCycle : 5;
        u64 rgbA1stCycle : 4;
        u64 : 8;
    };
};

union SetTileHeader {
    u64 raw;
    struct {
        u64 shiftS : 4;
        u64 maskS : 4;
        u64 mirrorS : 1;
        u64 clampS : 1;
        u64 shiftT : 4;
        u64 maskT : 4;
        u64 mirrorT : 1;
        u64 clampT : 1;
        u64 palette : 4;
        u64 index : 3;
        u64 : 5;
        u64 address : 9;
        u64 line : 9;
        u64 : 1;
        u64 size : 2;
        u64 format : 3;
        u64 : 8;
    };
};

union TextureRectangleHeader {
    u64 raw;
    struct {
        u64 y0 : 12;
        u64 x0 : 12;
        u64 tile : 3;
        u64 : 5;
        u64 y1 : 12;
        u64 x1 : 12;
        u64 : 8;
    };
};

union TextureRectangleParameters {
    u64 raw;
    struct {
        u64 dtdy : 16;
        u64 dsdx : 16;
        u64 t : 16;
        u64 s : 16;
    };
};

void init();
void deinit();

void reset();

template<u64 size>
u64 readTMEM(const u64 tmemAddr, const u64 x, const u64 y, const u64 width);

template<u64 format, u64 size, bool isTLUT>
void loadTMEM(const u64 dramaddr, const u64 tmemAddr, const u64 x0, const u64 y0, const u64 x1, const u64 y1, const u64 dramWidth, const u64 tmemWidth);

u64 getCombinerRGBInputA(const u64 mode, const u64 texel0);
u64 getCombinerRGBInputB(const u64 mode, const u64 texel0);
u64 getCombinerRGBInputC(const u64 mode, const u64 texel0);
u64 getCombinerRGBInputD(const u64 mode, const u64 texel0);

u64 getCombinerAlphaInputA(const u64 mode, const u64 texel0);
u64 getCombinerAlphaInputB(const u64 mode, const u64 texel0);
u64 getCombinerAlphaInputC(const u64 mode, const u64 texel0);
u64 getCombinerAlphaInputD(const u64 mode, const u64 texel0);

template<u64 size>
u64 combine2ndCycle(const u64 texel0);

void fillRectangle(const u64 x0, const u64 y0, const u64 x1, const u64 y1);
void textureRectangle(TextureRectangleHeader header, TextureRectangleParameters params);

void loadTile(const u64 tileIndex, const u64 x0, const u64 y0, const u64 x1, const u64 y1);
void loadTLUT(const u64 tileIndex, const u64 x0, const u64 y0, const u64 x1, const u64 y1);
void setColorImage(const u64 dramaddr, const u64 width, const u64 size, const u64 format);
void setCombineMode(const SetCombineModeHeader header);
void setFillColor(const u32 fillColor);
void setScissor(const u64 x0, const u64 y0, const u64 x1, const u64 y1);
void setTextureImage(const u64 dramaddr, const u64 width, const u64 size, const u64 format);
void setTile(const SetTileHeader header);

}
