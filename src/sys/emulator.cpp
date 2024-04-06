/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/emulator.hpp"

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "hw/pi.hpp"
#include "hw/ri.hpp"
#include "hw/vi.hpp"
#include "hw/cpu/cpu.hpp"

#include "renderer/renderer.hpp"

#include "sys/memory.hpp"

namespace sys::emulator {

constexpr bool IS_FAST_BOOT = true;

constexpr i64 CPU_FREQUENCY = 93750000;
constexpr i64 CPU_CYCLES_PER_FRAME = CPU_FREQUENCY / 60;

void init(const char *romPath) {
    PLOG_INFO << "ROM path = " << romPath;

    renderer::init();

    sys::memory::init(romPath);

    hw::cpu::init();
    hw::pi::init();
    hw::ri::init();
    hw::vi::init();
}

void deinit() {
    sys::memory::deinit();

    hw::cpu::deinit();
    hw::pi::deinit();
    hw::ri::deinit();
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

    sys::memory::reset(IS_FAST_BOOT);

    hw::cpu::reset(IS_FAST_BOOT);
    hw::pi::reset();
    hw::ri::reset();
    hw::vi::reset();
}

}
