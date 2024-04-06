/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace renderer {

void init();
void deinit();

void reset();

void drawFrameBuffer(const u64 paddr, const u32 format);

}
