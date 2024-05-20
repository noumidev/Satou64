/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::cic {

void init();
void deinit();

void reset();

void setNextData(const u8 data);

u8 read();

void write(const u8 data);

}
