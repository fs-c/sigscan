#pragma once

#include "signature.h"

#include "Windows.h"
#include "Tlhelp32.h"

#include <vector>
#include <string_view>

class Process {
public:
    static Process find_process(std::string_view name);

private:
    static HANDLE get_handle(unsigned int process_id);

    HANDLE handle = nullptr;

public:
    explicit Process(unsigned int process_id) : handle(get_handle(process_id)) {};

    template<typename T>
    size_t read_memory(uintptr_t address, T *out, size_t count) const {
        size_t read = 0;

        if (!ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(address),
                reinterpret_cast<LPVOID>(out), count * sizeof(T),
                reinterpret_cast<SIZE_T *>(&read))) {
            return 0;
        }

        return read;
    }

    std::vector<uintptr_t> find_signatures(const std::vector<Signature> &signatures) const;
};

class Module {
    const Process &process;
    const uintptr_t base;
    const size_t size;

public:
    Module(const Process &process, uintptr_t base, uintptr_t size);
};
