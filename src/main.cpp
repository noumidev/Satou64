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
    plog::init(plog::verbose, &consoleAppender);

    if (argc < 2) {
        PLOG_ERROR << "Usage: Satou64 [path to N64 ROM]";

        return -1;
    }

    sys::emulator::init(argv[1]);
    sys::emulator::reset();
    sys::emulator::run();
    sys::emulator::deinit();

    return 0;
}
