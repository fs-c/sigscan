#include "process.h"

#include "module.h"

#include <iostream>

Process Process::find_process(std::string_view name) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("couldn't enumerate running processes");
    }

    Process32First(snapshot, &entry);
    if (name == entry.szExeFile) {
        CloseHandle(snapshot);
        return Process(entry.th32ProcessID);
    }

    while (Process32Next(snapshot, &entry)) {
        if (name == entry.szExeFile) {
            CloseHandle(snapshot);
            return Process(entry.th32ProcessID);
        }
    }

    CloseHandle(snapshot);
    throw std::runtime_error(std::format("couldn't find process '{}'", name));
}

HANDLE Process::get_handle(unsigned int process_id) {
    HANDLE handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(std::format("couldn't open handle to process {}", process_id));
    }

    return handle;
}

Module Process::find_module(std::string_view name) const {
    MODULEENTRY32 entry;
    entry.dwSize = sizeof(MODULEENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, id);
    if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error(std::format("couldn't enumerate modules of process {}", id));
    }

    Module32First(snapshot, &entry);

    if (name == entry.szModule) {
        CloseHandle(snapshot);
        return Module(this, reinterpret_cast<uintptr_t>(entry.modBaseAddr), entry.modBaseSize);
    }

    while (Module32Next(snapshot, &entry)) {
        if (name == entry.szModule) {
            CloseHandle(snapshot);
            return Module(this, reinterpret_cast<uintptr_t>(entry.modBaseAddr), entry.modBaseSize);
        }
    }

    CloseHandle(snapshot);
    throw std::runtime_error(std::format("couldn't find module '{}' in process {}", name, id));
}
