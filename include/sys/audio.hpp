/*
 * ChiSP is a PlayStation Portable emulator written in C++.
 * Copyright (C) 2023  noumidev
 */

#pragma once

#include "../common/types.hpp"

namespace sys::audio {

void init();
void deinit();

void reset();

void audioCallback(void *userData, u8 *buffer, int length);
void pushSamples(const i16 left, const i16 right);

void doSample();

}
