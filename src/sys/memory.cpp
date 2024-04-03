/*
 * Satou64 is a Nintendo 64 emulator written in C++.
 * Copyright (C) 2024  noumidev
 */

#include "sys/memory.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <vector>

#include <plog/Log.h>

#include "hw/pi.hpp"
#include "hw/vi.hpp"

namespace sys::memory {

constexpr u64 NUM_PAGES = MemorySize::AddressSpace >> PAGE_SHIFT;

// Page table for software fastmem
std::array<u8 *, NUM_PAGES> pageTable;

// Memory arrays

std::array<u8, MemorySize::RSP_DMEM> dmem;
std::array<u8, MemorySize::RDRAM> rdram;
std::array<u8, PAGE_SIZE> pif;

std::vector<u8> rom;

void init(const char *romPath) {
    // Read ROM
    FILE *file = std::fopen(romPath, "rb");
    if (file == NULL) {
        PLOG_FATAL << "Unable to open ROM file";

        exit(0);
    }

    // Get file size
    std::fseek(file, 0, SEEK_END);
    rom.resize(std::ftell(file));
    std::fseek(file, 0, SEEK_SET);

    // Read file
    std::fread(rom.data(), sizeof(u8), rom.size(), file);
    std::fclose(file);

    // Map all memory regions
    map(MemoryBase::RDRAM, MemorySize::RDRAM, rdram.data());
    map(MemoryBase::RSP_DMEM, MemorySize::RSP_DMEM, dmem.data());
    map(MemoryBase::CART_DOM1_A2, rom.size(), rom.data());
    map(MemoryBase::PIF_RAM, PAGE_SIZE, pif.data());
}

void deinit() {}

void run() {}

void reset(const bool isFastBoot) {
    // Copy IPL3 to RSP DMEM upon fast boot
    if (isFastBoot) {
        std::memcpy(dmem.data(), rom.data(), MemorySize::RSP_DMEM);
    }
}

u64 addressToPage(const u64 addr) {
    return addr >> PAGE_SHIFT;
}

constexpr u64 addressToIOPage(const u64 addr) {
    constexpr u64 IO_SHIFT = 20;

    return addr >> IO_SHIFT;
}

u64 pageToAddress(const u64 page) {
    return page << PAGE_SHIFT;
}

bool isValidPhysicalAddress(const u64 paddr) {
    return paddr < MemorySize::AddressSpace;
}

void map(const u64 paddr, const u64 size, u8 *mem) {
    const u64 page = addressToPage(paddr);
    const u64 pageNum = addressToPage(size);

    const u64 endPage = page + pageNum;

    for (u64 i = page; i < endPage; i++) {
        pageTable[i] = &mem[pageToAddress(i - page)];
    }
}

u8 *getPointer(const u64 paddr) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        return &pageTable[page][offset];
    }

    PLOG_FATAL << "Unrecognized physical address " << std::hex << paddr;

    exit(0);
}

template<>
u8 read(const u64 paddr) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        return pageTable[page][offset];
    }

    PLOG_FATAL << "Unrecognized read8 (address = " << std::hex << paddr << ")";

    exit(0);
}

template<>
u16 read(const u64 paddr) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        u16 data;
        std::memcpy(&data, &pageTable[page][offset], sizeof(u16));

        return byteswap(data);
    }

    PLOG_FATAL << "Unrecognized read16 (address = " << std::hex << paddr << ")";

    exit(0);
}

template<>
u32 read(const u64 paddr) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        u32 data;
        std::memcpy(&data, &pageTable[page][offset], sizeof(u32));

        return byteswap(data);
    }

    // Try to read I/O
    return readIO(paddr);
}

template<>
u64 read(const u64 paddr) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        u64 data;
        std::memcpy(&data, &pageTable[page][offset], sizeof(u64));

        return byteswap(data);
    }

    PLOG_FATAL << "Unrecognized read64 (address = " << std::hex << paddr << ")";

    exit(0);
}

u32 readIO(const u64 ioaddr) {
    const u64 iopage = addressToIOPage(ioaddr);

    switch (iopage) {
        case addressToIOPage(hw::vi::IORegister::IOBase):
            return hw::vi::readIO(ioaddr);
        case addressToIOPage(hw::pi::IORegister::IOBase):
            return hw::pi::readIO(ioaddr);
        default:
            PLOG_FATAL << "Unrecognized IO read (address = " << std::hex << ioaddr << ")";

            exit(0);
    }
}

template<>
void write(const u64 paddr, const u8 data) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;

        pageTable[page][offset] = data;

        return;
    }

    PLOG_FATAL << "Unrecognized write8 (address = " << std::hex << paddr << ", data = " << (u32)data << ")";

    exit(0);
}

template<>
void write(const u64 paddr, const u16 data) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;
        const u16 swappedData = byteswap(data);

        std::memcpy(&pageTable[page][offset], &swappedData, sizeof(u16));
        return;
    }

    PLOG_FATAL << "Unrecognized write16 (address = " << std::hex << paddr << ", data = " << data << ")";

    exit(0);
}

template<>
void write(const u64 paddr, const u32 data) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;
        const u32 swappedData = byteswap(data);

        std::memcpy(&pageTable[page][offset], &swappedData, sizeof(u32));
        return;
    }

    // Try to write I/O
    return writeIO(paddr, data);
}

template<>
void write(const u64 paddr, const u64 data) {
    if (!isValidPhysicalAddress(paddr)) {
        PLOG_FATAL << "Invalid physical address " << std::hex << paddr;

        exit(0);
    }

    const u64 page = addressToPage(paddr);

    if (pageTable[page] != NULL) {
        const u64 offset = paddr & PAGE_MASK;
        const u64 swappedData = byteswap(data);

        std::memcpy(&pageTable[page][offset], &swappedData, sizeof(u64));
        return;
    }

    PLOG_FATAL << "Unrecognized write64 (address = " << std::hex << paddr << ", data = " << data << ")";

    exit(0);
}

void writeIO(const u64 ioaddr, const u32 data) {
    const u64 iopage = addressToIOPage(ioaddr);

    switch (iopage) {
        case addressToIOPage(hw::vi::IORegister::IOBase):
            return hw::vi::writeIO(ioaddr, data);
        case addressToIOPage(hw::pi::IORegister::IOBase):
            return hw::pi::writeIO(ioaddr, data);
        default:
            PLOG_FATAL << "Unrecognized IO write (address = " << std::hex << ioaddr << ", data = " << data << ")";

            exit(0);
    }
}

}
