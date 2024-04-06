/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "renderer/renderer.hpp"

#include <cstdlib>
#include <cstring>
#include <vector>

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "sys/memory.hpp"

namespace renderer {

constexpr int DEFAULT_WIDTH = 640;
constexpr int DEFAULT_HEIGHT = 512;

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

    u32 width, height;
};

Screen screen;

void init() {
    screen.width = DEFAULT_WIDTH;
    screen.height = DEFAULT_HEIGHT;

    // Initialize SDL2
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // Create window and renderer
    SDL_CreateWindowAndRenderer(DEFAULT_WIDTH, DEFAULT_HEIGHT, 0, &screen.window, &screen.renderer);
    SDL_SetWindowSize(screen.window, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    SDL_RenderSetLogicalSize(screen.renderer, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    SDL_SetWindowResizable(screen.window, SDL_FALSE);
    SDL_SetWindowTitle(screen.window, "Satou64");

    // Create texture
    screen.texture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

void deinit() {
    SDL_DestroyTexture(screen.texture);
    SDL_DestroyRenderer(screen.renderer);
    SDL_DestroyWindow(screen.window);

    SDL_Quit();
}

void reset() {}

void changeResolution(const u32 width) {
    if (width == screen.width) {
        return;
    }

    screen.width = width;
    switch (width) {
        case 320:
            screen.height = 256;
            break;
        case 640:
            screen.height = 512;
            break;
        default:
            PLOG_FATAL << "Unrecognized screen width " << width;

            exit(0);
    }

    SDL_DestroyTexture(screen.texture);
    SDL_RenderSetLogicalSize(screen.renderer, screen.width, screen.height);

    screen.texture = SDL_CreateTexture(screen.renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, screen.width, screen.height);
}

void drawFrameBuffer(const u64 paddr, const u32 format) {
    std::vector<u32> frameBuffer;
    frameBuffer.resize(screen.width * screen.height);

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
            for (u64 i = 0; i < frameBuffer.size(); i++) {
                const u16 color = sys::memory::read<u16>(paddr + 2 * i);

                // Extract RGBA5551 color channels
                const u32 b = (color >>  1) & 0x1F;
                const u32 g = (color >>  6) & 0x1F;
                const u32 r = (color >> 11) & 0x1F;

                // Convert to RGBA8888
                u32 newColor = 0;
                newColor |= ((b << 3) | (b >> 3)) <<  8;
                newColor |= ((g << 3) | (g >> 3)) << 16;
                newColor |= ((r << 3) | (r >> 3)) << 24;

                // Alpha
                if ((color & 1) != 0) {
                    newColor |= 0xFF;
                }

                frameBuffer[i] = newColor;
            }
            break;
        case Format::Reserved:
        default:
            PLOG_FATAL << "Unrecognized frame buffer format " << format;

            exit(0);
    }

    // Draw frame buffer
    SDL_UpdateTexture(screen.texture, nullptr, frameBuffer.data(), 4 * screen.width);
    SDL_RenderCopy(screen.renderer, screen.texture, nullptr, nullptr);
    SDL_RenderPresent(screen.renderer);
}

}
