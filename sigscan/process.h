#pragma once

#include "signature.h"

#include "Windows.h"
#include "Tlhelp32.h"

#include <string_view>
#include <exception>
#include <format>

class Module;

class Process {
public:
    static Process find_process(std::string_view name);

private:
    static HANDLE get_handle(unsigned int process_id);

    unsigned int id = 0;
    HANDLE handle = nullptr;

public:
    explicit Process(unsigned int id) : id(id), handle(get_handle(id)) {};

    template<typename T>
    size_t read_memory(uintptr_t address, T *out, size_t count) const;

    template<typename T>
    T read_memory(uintptr_t address) const;

    Module find_module(std::string_view name) const;
};

template<typename T>
size_t Process::read_memory(uintptr_t address, T* out, size_t count) const {
    size_t read = 0;

    if (!ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(address),
        reinterpret_cast<LPVOID>(out), count * sizeof(T),
        reinterpret_cast<SIZE_T*>(&read))) {
        return 0;
    }

    return read;
};

template<typename T>
T Process::read_memory(uintptr_t address) const {
    T out;

    if (!read_memory(address, &out, 1)) {
        throw std::runtime_error(std::format("failed reading memory at {:#x}", address));
    }

    return out;
};
