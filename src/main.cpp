/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include <plog/Init.h>
#include <plog/Log.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Appenders/ColorConsoleAppender.h>

#include "sys/emulator.hpp"

int main(int argc, char **argv) {
    // Initialize logger
    static plog::ColorConsoleAppender<plog::FuncMessageFormatter> consoleAppender;
    plog::init(plog::fatal, &consoleAppender);

    if (argc < 4) {
        PLOG_ERROR << "Usage: Satou64 [path to boot ROM] [path to PIF-NUS ROM] [path to N64 ROM]";

        return -1;
    }

    sys::emulator::init(argv[1], argv[2], argv[3]);
    sys::emulator::reset();
    sys::emulator::run();
    sys::emulator::deinit();

    return 0;
}
