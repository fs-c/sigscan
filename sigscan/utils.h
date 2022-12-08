#pragma once

#include "Windows.h"
#include <TlHelp32.h>

#include <string>

namespace utils {
    HANDLE get_handle(const std::string &name) {
        auto find_process = [](const std::string &name) {
            PROCESSENTRY32 processInfo;
            processInfo.dwSize = sizeof(PROCESSENTRY32);

            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (!snapshot || snapshot == INVALID_HANDLE_VALUE) {
                return 0ul;
            }

            Process32First(snapshot, &processInfo);
            if (name == processInfo.szExeFile) {
                CloseHandle(snapshot);
                return processInfo.th32ProcessID;
            }

            while (Process32Next(snapshot, &processInfo)) {
                if (name == processInfo.szExeFile) {
                    CloseHandle(snapshot);
                    return processInfo.th32ProcessID;
                }
            }

            CloseHandle(snapshot);
            return 0ul;
        };

        auto process_id = find_process(name);
        if (!process_id) {
            return 0;
        }

        HANDLE handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, process_id);
        if (!handle || handle == INVALID_HANDLE_VALUE) {
            return 0;
        }

        return handle;
    }
}
