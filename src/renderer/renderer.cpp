/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "renderer/renderer.hpp"

#include <array>
#include <cstdlib>
#include <cstring>

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "sys/memory.hpp"

namespace renderer {

constexpr int DEFAULT_WIDTH = 320;
constexpr int DEFAULT_HEIGHT = 256;

namespace Format {
    enum : u32 {
        Blank,
        Reserved,
        RGBA5551,
        RGBA8888,
    };
}

struct Screen {
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Texture *texture;
};

Screen screen;

void init() {
    // Initialize SDL2
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // Create window and renderer
    SDL_CreateWindowAndRenderer(DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, &screen.window, &screen.renderer);
    SDL_SetWindowSize(screen.window, 2 * DEFAULT_WIDTH, 2 * DEFAULT_HEIGHT);
    SDL_RenderSetLogicalSize(screen.renderer, 2 * DEFAULT_WIDTH, 2 * DEFAULT_HEIGHT);
    SDL_SetWindowResizable(screen.window, SDL_FALSE);
    SDL_SetWindowTitle(screen.window, "Satou64");

    // Create texture
    screen.texture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_BGRX8888, SDL_TEXTUREACCESS_STREAMING, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

void deinit() {
    SDL_DestroyTexture(screen.texture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);

    SDL_Quit();
}

void reset() {}

void drawFrameBuffer(const u64 paddr, const u32 format) {
    std::array<u32, DEFAULT_WIDTH * DEFAULT_HEIGHT> frameBuffer;

    switch (format) {
        case Format::Blank:
            std::memset(frameBuffer.data(), 0, frameBuffer.size() * sizeof(u32));
            break;
        case Format::RGBA8888:
            std::memcpy(frameBuffer.data(), sys::memory::getPointer(paddr), frameBuffer.size() * sizeof(u32));

            for (u64 i = 0; i < frameBuffer.size(); i++) {
                frameBuffer[i] = byteswap(frameBuffer[i]);
            }
            break;
        case Format::RGBA5551:
        case Format::Reserved:
        default:
            PLOG_FATAL << "Unrecognized frame buffer format " << format;

            exit(0);
    }

    // Draw frame buffer
    SDL_UpdateTexture(screen.texture, nullptr, frameBuffer.data(), 4 * DEFAULT_WIDTH);
    SDL_RenderCopy(screen.renderer, screen.texture, nullptr, nullptr);
    SDL_RenderPresent(screen.renderer);
}

}
