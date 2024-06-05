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

void setDataIn(const u64 length);
void setDataOut(const u64 data, const u64 length);

u8 read();

void write(const u8 data);

void doCompare();

}
