/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

namespace sys::emulator {

void init(const char *bootPath, const char *pifPath, const char *romPath);
void deinit();

void run();

void reset();

void finishFrame();

}
