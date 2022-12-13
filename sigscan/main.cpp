#include "signature.h"

#include "Windows.h"
#include "Tlhelp32.h"

#include <format>
#include <vector>
#include <iostream>
#include <functional>

class Process {
public:
    static Process find_process(std::string_view name) {
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

private:
    static HANDLE get_handle(unsigned int process_id) {
        HANDLE handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process_id);
        if (!handle || handle == INVALID_HANDLE_VALUE) {
            throw std::runtime_error(std::format("couldn't open handle to process {}", process_id));
        }

        return handle;
    }

    HANDLE handle = nullptr;

public:
    explicit Process(unsigned int process_id) : handle(get_handle(process_id)) {
    }

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

    std::vector<uintptr_t> find_signatures(const std::vector<Signature> &signatures) const {
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
};

class Module {
    const Process &process;
    const uintptr_t base;
    const size_t size;

public:
    Module(const Process &process, uintptr_t base, uintptr_t size) : process(process), base(base),
                                                                     size(size) {
    }
};

int main(int argc, char *argv[]) {
    if (argc == 2) {
        // give sample.exe some time to start up, only relevant for testing
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::string_view process_name = argc == 3 ? argv[2] : "sample.exe";
    const auto process = Process::find_process(process_name);

    try {
        const auto results = process.find_signatures({Signature{"74 61 8B 05", 4}});

        for (const auto &result: results) {
            std::cout << result << std::endl;
        }
    } catch (const std::runtime_error &err) {
        std::cout << std::format("runtime error: {}\n", err.what());
    }
}