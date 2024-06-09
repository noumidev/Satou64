/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#pragma once

#include "common/types.hpp"

namespace hw::pif::joybus {

void init();
void deinit();

void reset();
void resetTXBuffer();

void prepareReceiveData(const u8 length);

void setActiveChannel(const u8 channel);

u8 calculateCRC(const u8 *data);

void doCommand();

void cmdControllerState();
void cmdInfo();
void cmdReadControllerAccessory();
void cmdWriteControllerAccessory();

u8 readChannel();
u8 readError();
u8 readReceive();
u8 readStatus();

void writeChannel(const u8 data);
void writeControl(const u8 data);
void writeError(const u8 data);
void writeTransmit(const u8 data);

}
