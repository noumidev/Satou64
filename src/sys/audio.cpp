/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/audio.hpp"

#include <array>
#include <cstdlib>
#include <cstring>
#include <ios>

#include <plog/Log.h>

#include <SDL2/SDL.h>

#include "hw/ai.hpp"

#include "sys/scheduler.hpp"

namespace sys::audio {

constexpr int SAMPLE_RATE = 48000;
constexpr int DEVICE_SAMPLE_BUFFER_SIZE = 1024;
constexpr int SAMPLE_BUFFER_SIZE = 16 * DEVICE_SAMPLE_BUFFER_SIZE;
constexpr int SAMPLE_BUFFER_MASK = SAMPLE_BUFFER_SIZE - 1;

constexpr i64 CYCLES_PER_AUDIO_FRAME = scheduler::CPU_FREQUENCY / SAMPLE_RATE;

SDL_AudioDeviceID audioDev;

std::array<i16, 16 * SAMPLE_BUFFER_SIZE> audioData;

u64 audioReadIdx, audioWriteIdx;

u64 idDoSample;

void init() {
    // Initialize audio subsystem
    SDL_Init(SDL_INIT_AUDIO);

    SDL_AudioSpec audioSpec;
    audioSpec.freq = SAMPLE_RATE;
    audioSpec.format = AUDIO_S16;
    audioSpec.channels = 2;
    audioSpec.samples = DEVICE_SAMPLE_BUFFER_SIZE;
    audioSpec.callback = audioCallback;
    audioSpec.userdata = NULL;

    audioDev = SDL_OpenAudioDevice(NULL, 0, &audioSpec, NULL, 0);
    if (audioDev == 0) {
        PLOG_FATAL << "Failed to open audio device";

        exit(0);
    }

    SDL_PauseAudioDevice(audioDev, 0);

    idDoSample = scheduler::registerEvent([](int) { doSample(); });
}

void deinit() {}

void reset() {
    audioData.fill(0);

    audioReadIdx = audioWriteIdx = 0;

    scheduler::addEvent(idDoSample, 0, CYCLES_PER_AUDIO_FRAME);
}

void audioCallback(void *userData, u8 *buffer, int length) {
    (void)userData;

    i16 *sampleBuffer = (i16 *)buffer;

    const int sampleNum = length / sizeof(i16);
    for (int i = 0; i < sampleNum;) {
        sampleBuffer[i++] = audioData[(audioReadIdx + 0) & SAMPLE_BUFFER_MASK];
        sampleBuffer[i++] = audioData[(audioReadIdx + 1) & SAMPLE_BUFFER_MASK];

        audioReadIdx += 2;
    }
}

void pushSamples(const i16 left, const i16 right) {
    audioData[(audioWriteIdx + 0) & SAMPLE_BUFFER_MASK] = left;
    audioData[(audioWriteIdx + 1) & SAMPLE_BUFFER_MASK] = right;

    audioWriteIdx += 2;
}

void doSample() {
    if (hw::ai::isEnabled()) {
        // Pull AI samples
        const u32 samples = hw::ai::getSamples();

        const i16 left = (i16)samples;
        const i16 right = (i16)(samples >> 16);

        pushSamples(left, right);
    } else {
        pushSamples(0, 0);
    }

    scheduler::addEvent(idDoSample, 0, CYCLES_PER_AUDIO_FRAME);
}

}
