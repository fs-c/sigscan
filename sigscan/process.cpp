#include "process.h"

#include <format>
#include <exception>

Process Process::find_process(std::string_view name) {
    PROCESSENTRY32 processInfo;
    processInfo.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("couldn't enumerate running processes");
    }

    Process32First(snapshot, &processInfo);
    if (name == processInfo.szExeFile) {
        CloseHandle(snapshot);
        return Process(processInfo.th32ProcessID);
    }

    while (Process32Next(snapshot, &processInfo)) {
        if (name == processInfo.szExeFile) {
            CloseHandle(snapshot);
            return Process(processInfo.th32ProcessID);
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

std::vector<uintptr_t> Process::find_signatures(const std::vector<Signature> &signatures) const {
    auto addresses = std::vector<uintptr_t>(signatures.size(), 0);

    const uintptr_t max_address = UINTPTR_MAX;

    MEMORY_BASIC_INFORMATION info;

    for (uintptr_t cur_address = 0; cur_address < max_address; cur_address += info.RegionSize) {
        if (!VirtualQueryEx(handle, reinterpret_cast<void *>(cur_address), &info,
                sizeof(info))) {
            // make sure that info is always filled after the first iteration,
            // we can't really recover gracefully from this
            throw std::runtime_error(std::format("couldn't query region at {}", cur_address));
        }

        // set current address to base address in case we missed the last region boundary somehow
        const auto base = cur_address = reinterpret_cast<uintptr_t>(info.BaseAddress);

        bool invalidProtection = (info.Protect != PAGE_EXECUTE_READ); // TODO
        if (info.RegionSize == 0 || info.State != MEM_COMMIT || info.Type == MEM_MAPPED ||
                invalidProtection) {
            continue;
        }

        auto buffer = std::vector<uint8_t>(info.RegionSize);
        if (!read_memory<uint8_t>(base, buffer.data(), buffer.size())) {
            continue;
        }

        for (const auto &signature: signatures) {
            const auto result = signature.scan(buffer);

            if (result) {
                addresses.push_back(result);
            }
        }
    }

    return addresses;
}
