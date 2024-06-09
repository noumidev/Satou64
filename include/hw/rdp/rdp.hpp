/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::rdp {

void init();
void deinit();

void reset();

u64 processCommandList(const u64 startAddr, const u64 endAddr);

void cmdLoadTile(const u64 data);
void cmdLoadTLUT(const u64 data);
void cmdSetColorImage(const u64 data);
void cmdSetCombineMode(const u64 data);
void cmdSetOtherModes(const u64 data);
void cmdSetScissor(const u64 data);
void cmdSetTextureImage(const u64 data);
void cmdSetTile(const u64 data);
void cmdSyncFull(const u64 data);
void cmdSyncLoad(const u64 data);
void cmdSyncPipe(const u64 data);
void cmdSyncTile(const u64 data);
void cmdTextureRectangle(const u64 data, const u64 next);

}
