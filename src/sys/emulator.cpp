/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "hw/ai.hpp"
#include "hw/cic.hpp"
#include "hw/dp.hpp"
#include "hw/mi.hpp"
#include "hw/pi.hpp"
#include "hw/ri.hpp"
#include "hw/si.hpp"
#include "hw/sp.hpp"
#include "hw/vi.hpp"
#include "hw/cpu/cpu.hpp"
#include "hw/pif/memory.hpp"
#include "hw/pif/pif.hpp"

#include "renderer/renderer.hpp"

#include "sys/memory.hpp"
#include "sys/scheduler.hpp"

namespace sys::emulator {

bool isRunning;

void init(const char *bootPath, const char *pifPath, const char *romPath) {
    PLOG_INFO << "Boot ROM path = " << bootPath;
    PLOG_INFO << "PIF-NUS ROM path = " << pifPath;
    PLOG_INFO << "ROM path = " << romPath;

    renderer::init();

    sys::memory::init(bootPath, romPath);
    sys::scheduler::init();

    hw::pif::memory::init(pifPath);

    hw::cpu::init();
    hw::ai::init();
    hw::cic::init();
    hw::dp::init();
    hw::mi::init();
    hw::pi::init();
    hw::pif::init();
    hw::ri::init();
    hw::si::init();
    hw::sp::init();
    hw::vi::init();

    isRunning = true;
}

void deinit() {
    sys::memory::deinit();
    sys::scheduler::deinit();

    hw::pif::memory::deinit();

    hw::cpu::deinit();
    hw::ai::deinit();
    hw::cic::deinit();
    hw::dp::deinit();
    hw::mi::deinit();
    hw::pi::deinit();
    hw::pif::deinit();
    hw::ri::deinit();
    hw::si::deinit();
    hw::sp::deinit();
    hw::vi::deinit();

    renderer::deinit();
}

void run() {
    // Give PIF-NUS a headstart to simulate the slowness of excuting code from the boot ROM
    hw::pif::run(scheduler::CPU_FREQUENCY / 60 / 6);

    while (isRunning) {
        const i64 cycles = scheduler::getRunCycles();

        hw::pif::run(cycles / 6);
        hw::cpu::run(cycles);

        scheduler::run(cycles);
    }
}

void reset() {
    renderer::reset();

    sys::memory::reset();
    sys::scheduler::reset();

    hw::pif::memory::reset();

    hw::cpu::reset();
    hw::ai::reset();
    hw::cic::reset();
    hw::dp::reset();
    hw::mi::reset();
    hw::pi::reset();
    hw::pif::reset();
    hw::ri::reset();
    hw::si::reset();
    hw::sp::reset();
    hw::vi::reset();
}

void finishFrame() {
    renderer::drawFrameBuffer(hw::vi::getOrigin(), hw::vi::getFormat());

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            default:
                break;
        }
    }
}

}
