/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "hw/pif/joybus.hpp"

#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include "sys/emulator.hpp"

namespace hw::pif::joybus {

constexpr u8 NUM_CHANNELS = 5;

constexpr u64 TX_BUFFER_SIZE = 64; // Arbitrary number

enum class JoybusDevice {
    None,
    Controller,
};

enum class JoybusState {
    ReceiveCommand,
    ReceiveData,
};

// Port 4 bits
namespace JoybusError {
    enum : u8 {
        Error = 1 << 3,
    };
}

// Port 3 bits
namespace JoybusStatus {
    enum : u8 {
        DevicePresent = 1 << 2,
        Clock = 1 << 3,
    };
}

namespace JoybusCommand {
    enum : u8 {
        Info = 0x00,
        ControllerState = 0x01,
        WriteControllerAccessory = 0x03,
    };
}

namespace ControllerIdentifier {
    enum : u16 {
        Controller = 0x0500,
    };
}

namespace ControllerStatus {
    enum : u8 {
        NoControllerPak = 1 << 1,
    };
}

struct JoybusChannel {
    JoybusDevice device;
};

// 5 Joybus channels
JoybusChannel channels[NUM_CHANNELS];
JoybusChannel *activeChannel;

u8 currentChannel;

u8 txPointer, dataSize;
u8 txBuffer[TX_BUFFER_SIZE];

bool isFirstAccess;

JoybusState state;

void init() {}

void deinit() {}

void reset() {
    for (JoybusChannel &channel : channels) {
        channel.device = JoybusDevice::None;
    }

    // Add one controller
    channels[0].device = JoybusDevice::Controller;
    
    activeChannel = NULL;

    isFirstAccess = true;

    resetTXBuffer();

    state = JoybusState::ReceiveCommand;
}

void resetTXBuffer() {
    txPointer = dataSize = 0;

    std::memset(txBuffer, 0, TX_BUFFER_SIZE);
}

void prepareReceiveData(const u8 length) {
    dataSize = txPointer + length;

    state = JoybusState::ReceiveData;
}

void setActiveChannel(const u8 channel) {
    if (channel >= NUM_CHANNELS) {
        PLOG_FATAL << "Invalid Joybus channel " << channel;

        exit(0);
    }

    activeChannel = &channels[channel];

    currentChannel = channel;

    resetTXBuffer();

    state = JoybusState::ReceiveCommand;
}

u8 calculateCRC(const u8 *data) {
    constexpr u8 POLYNOMIAL = 0x85;

    u8 crc = 0;
    for (int i = 0; i <= 32; i++) {
        for (int j = 7; j >= 0; j--) {
            u8 mask = 0;
            if ((crc & (1 << 7)) != 0) {
                mask = POLYNOMIAL;
            }

            crc <<= 1;

            if (i != 32) {
                if ((data[i] & (1 << j)) != 0) {
                    crc |= 1;
                }
            }

            crc ^= mask;
        }
    }

    return crc;
}

void doCommand() {
    const u8 command = txBuffer[0];

    switch (command) {
        case JoybusCommand::Info:
            cmdInfo();
            break;
        case JoybusCommand::ControllerState:
            cmdControllerState();
            break;
        case JoybusCommand::WriteControllerAccessory:
            prepareReceiveData(34); // Two address bytes, 32 data bytes
            break;
        default:
            PLOG_FATAL << "Unrecognized Joybus command " << std::hex << (u16)command << " (channel = " << (u16)currentChannel << ")";

            exit(0);
    }
}

void cmdControllerState() {
    PLOG_VERBOSE << "Controller State (channel = " << (u16)currentChannel << ")";

    resetTXBuffer();

    switch (activeChannel->device) {
        case JoybusDevice::Controller:
            {
                PLOG_DEBUG << "Channel " << (u16)currentChannel << " is standard controller";

                const u32 buttonState = sys::emulator::getButtonState();

                std::memcpy(txBuffer, &buttonState, sizeof(u32));
            }
            break;
        default:
            PLOG_FATAL << "Unrecognized Joybus device";

            exit(0);
    }
}

void cmdInfo() {
    PLOG_VERBOSE << "Info (channel = " << (u16)currentChannel << ")";

    resetTXBuffer();

    u16 id;
    u8 status;

    switch (activeChannel->device) {
        case JoybusDevice::Controller:
            PLOG_DEBUG << "Channel " << (u16)currentChannel << " is standard controller";

            id = ControllerIdentifier::Controller;
            status = ControllerStatus::NoControllerPak;
            break;
        default:
            PLOG_FATAL << "Unrecognized Joybus device";

            exit(0);
    }

    std::memcpy(txBuffer, &id, sizeof(u16));

    txBuffer[2] = status;
}

void cmdWriteControllerAccessory() {
    PLOG_VERBOSE << "Write Controller Accessory (channel = " << (u16)currentChannel << ")";

    const u8 crc = calculateCRC(&txBuffer[3]);

    resetTXBuffer();

    switch (activeChannel->device) {
        case JoybusDevice::Controller:
            PLOG_DEBUG << "Channel " << (u16)currentChannel << " is standard controller";
            PLOG_WARNING << "No Controller Pak inserted";
            break;
        default:
            PLOG_FATAL << "Unrecognized Joybus device";

            exit(0);
    }

    txBuffer[0] = crc;
}

u8 readChannel() {
    PLOG_VERBOSE << "Read from Joybus Channel";

    return currentChannel;
}

u8 readError() {
    PLOG_VERBOSE << "Read from Joybus Error";

    return 0;
}

u8 readReceive() {
    PLOG_VERBOSE << "Read from Joybus Receive";

    u8 data;
    if (isFirstAccess) {
        data = txBuffer[txPointer] >> 4;
    } else {
        data = txBuffer[txPointer++] & 0xF;
    }

    isFirstAccess = !isFirstAccess;

    return data;
}

u8 readStatus() {
    PLOG_VERBOSE << "Read from Joybus Status";

    u8 isDevicePresent = 0;
    if (activeChannel->device != JoybusDevice::None) {
        isDevicePresent |= JoybusStatus::DevicePresent;
    }

    return JoybusStatus::Clock | isDevicePresent;
}

void writeChannel(const u8 data) {
    PLOG_VERBOSE << "Write to Joybus Channel (data = " << std::hex << (u16)data << ")";

    setActiveChannel(data);
}

void writeControl(const u8 data) {
    PLOG_WARNING << "Write to Joybus Control (data = " << std::hex << (u16)data << ")";
}

void writeError(const u8 data) {
    PLOG_WARNING << "Write to Joybus Control (data = " << std::hex << (u16)data << ")";
}

void writeTransmit(const u8 data) {
    PLOG_VERBOSE << "Write to Joybus Transmit (data = " << std::hex << (u16)data << ")";

    if (txPointer >= TX_BUFFER_SIZE) {
        PLOG_FATAL << "Invalid TX pointer";

        exit(0);
    }

    if (isFirstAccess) {
        txBuffer[txPointer] = data << 4;
    } else {
        txBuffer[txPointer++] |= data;
    }

    switch (state) {
        case JoybusState::ReceiveCommand:
            doCommand();
            break;
        case JoybusState::ReceiveData:
            {
                if (txPointer == dataSize) {
                    const u8 command = txBuffer[0];
                    switch (command) {
                        case JoybusCommand::WriteControllerAccessory:
                            cmdWriteControllerAccessory();
                            break;
                        default:
                            PLOG_FATAL << "Unrecognized Joybus command " << std::hex << (u16)command << " (channel = " << (u16)currentChannel << ")";

                            exit(0);
                    }
                }
            }
            break;
    }

    isFirstAccess = !isFirstAccess;
}

}
