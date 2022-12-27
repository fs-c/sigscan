#include "signature.h"
#include "process.h"
#include "module.h"

#include <format>
#include <iostream>
#include <thread>
#include <ranges>

int main(int argc, char *argv[]) {    
    const auto process_name = argc >= 3 ? argv[2] : "sample.exe";

    try {
        const auto process = Process::find_process(process_name);
        
        const auto counter_ptr = process.find_module("sample.exe")
            .find_signature(Signature{ "83 EC ? A1 ? ? ? ? 8b 0D", 4 });
        const auto counter = process.read_memory<uintptr_t>(counter_ptr);

        std::cout << std::format("found counter at {:#x} -> {:#x}\n", counter, counter_ptr);

        while (true) {
            std::cout << process.read_memory<int32_t>(counter) << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::runtime_error &err) {
        std::cout << std::format("runtime error: {}\n", err.what());
    }
}
