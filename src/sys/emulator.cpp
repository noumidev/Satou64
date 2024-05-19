/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "hw/ai.hpp"
#include "hw/mi.hpp"
#include "hw/pi.hpp"
#include "hw/ri.hpp"
#include "hw/sp.hpp"
#include "hw/vi.hpp"
#include "hw/cpu/cpu.hpp"

#include "renderer/renderer.hpp"

#include "sys/memory.hpp"

namespace sys::emulator {

constexpr i64 CPU_FREQUENCY = 93750000;
constexpr i64 CPU_CYCLES_PER_FRAME = CPU_FREQUENCY / 60;

void init(const char *bootPath, const char *romPath) {
    PLOG_INFO << "Boot ROM path = " << bootPath;
    PLOG_INFO << "ROM path = " << romPath;

    renderer::init();

    sys::memory::init(bootPath, romPath);

    hw::cpu::init();
    hw::ai::init();
    hw::mi::init();
    hw::pi::init();
    hw::ri::init();
    hw::sp::init();
    hw::vi::init();
}

void deinit() {
    sys::memory::deinit();

    hw::cpu::deinit();
    hw::ai::deinit();
    hw::mi::deinit();
    hw::pi::deinit();
    hw::ri::deinit();
    hw::sp::deinit();
    hw::vi::deinit();

    renderer::deinit();
}

void run() {
    while (true) {
        hw::cpu::run(CPU_CYCLES_PER_FRAME);

        renderer::drawFrameBuffer(hw::vi::getOrigin(), hw::vi::getFormat());

        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    return;
                default:
                    break;
            }
        }
    }
}

void reset() {
    renderer::reset();

    sys::memory::reset();

    hw::cpu::reset();
    hw::ai::reset();
    hw::mi::reset();
    hw::pi::reset();
    hw::ri::reset();
    hw::sp::reset();
    hw::vi::reset();
}

}
