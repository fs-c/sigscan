#include "signature.h"
#include "process.h"

#include <format>
#include <iostream>
#include <thread>
#include <ranges>

int main(int argc, char *argv[]) {    
    const auto process_name = argc >= 2 ? argv[2] : "sample.exe";

    try {
        const auto process = Process::find_process(process_name);

        const auto value_ptr_addr = process.find_signature(Signature{ "40 50 A3 ? ? ? ? FF 15", 3 }, "sample.exe");

        if (!value_ptr_addr) {
            throw std::runtime_error("couldn't find signature");
        }

        const auto value_ptr = process.read_memory<uintptr_t>(value_ptr_addr);

        std::cout << std::format("found value at {:#x} -> {:#x}\n", value_ptr, value_ptr_addr);

        while (true) {
            std::cout << process.read_memory<int32_t>(value_ptr) << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::runtime_error &err) {
        std::cout << std::format("runtime error: {}\n", err.what());
    }
}
