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
#include "hw/pif/joybus.hpp"
#include "hw/pif/memory.hpp"
#include "hw/pif/pif.hpp"
#include "hw/rdp/rasterizer.hpp"
#include "hw/rdp/rdp.hpp"
#include "hw/rsp/rsp.hpp"

#include "renderer/renderer.hpp"

#include "sys/memory.hpp"
#include "sys/scheduler.hpp"

namespace sys::emulator {

namespace ControllerButton {
    enum : u32 {
        DpadRight = 1 << 0,
        DpadLeft = 1 << 1,
        DpadDown = 1 << 2,
        DpadUp = 1 << 3,
        Start = 1 << 4,
        Z = 1 << 5,
        B = 1 << 6,
        A = 1 << 7,
        Reset = 1 << 8,
        LeftTrigger = 1 << 10,
        RightTrigger = 1 << 11,
        CUp = 1 << 12,
        CDown = 1 << 13,
        CLeft = 1 << 14,
        CRight = 1 << 15,
    };
}

u32 buttonState;

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
    hw::pif::joybus::init();
    hw::rdp::init();
    hw::rdp::rasterizer::init();
    hw::rsp::init();
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
    hw::pif::joybus::deinit();
    hw::rdp::deinit();
    hw::rdp::rasterizer::deinit();
    hw::rsp::deinit();
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
        hw::rsp::run(cycles / 2); // TODO: not correct, will fix later

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
    hw::pif::joybus::reset();
    hw::rdp::reset();
    hw::rdp::rasterizer::reset();
    hw::rsp::reset();
    hw::ri::reset();
    hw::si::reset();
    hw::sp::reset();
    hw::vi::reset();

    buttonState = 0;
}

u32 getButtonState() {
    return buttonState;
}

void finishFrame() {
    renderer::drawFrameBuffer(hw::vi::getOrigin(), hw::vi::getFormat());

    updateButtonState();
}

void updateButtonState() {
    const u8 *keyState = SDL_GetKeyboardState(NULL);

    buttonState = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (keyState[SDL_GetScancodeFromKey(SDLK_d)] != 0) {
                    buttonState |= ControllerButton::DpadRight;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_a)] != 0) {
                    buttonState |= ControllerButton::DpadLeft;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_s)] != 0) {
                    buttonState |= ControllerButton::DpadDown;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_w)] != 0) {
                    buttonState |= ControllerButton::DpadUp;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_SPACE)] != 0) {
                    buttonState |= ControllerButton::Start;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_m)] != 0) {
                    buttonState |= ControllerButton::Z;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_b)] != 0) {
                    buttonState |= ControllerButton::B;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_n)] != 0) {
                    buttonState |= ControllerButton::A;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_q)] != 0) {
                    buttonState |= ControllerButton::LeftTrigger;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_e)] != 0) {
                    buttonState |= ControllerButton::RightTrigger;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_u)] != 0) {
                    buttonState |= ControllerButton::CUp;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_j)] != 0) {
                    buttonState |= ControllerButton::CDown;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_h)] != 0) {
                    buttonState |= ControllerButton::CLeft;
                }

                if (keyState[SDL_GetScancodeFromKey(SDLK_k)] != 0) {
                    buttonState |= ControllerButton::CRight;
                }
                break;
            default:
                break;
        }
    }
}

}
