#include "utils.h"

#include <format>
#include <iostream>
#include <functional>

// iterate over all regions (subsequent pages with equal attributes) of the process
// with the given handle, call given callback function for each
void map_regions(HANDLE handle, const std::function<void(MEMORY_BASIC_INFORMATION *)> &callback) {
    uintptr_t cur_address = 0;
    // ensure architecture compatibility, UINTPTR_MAX is the largest (data) pointer i.e. address
    // for 32bit this is 0xFFFFFFFF or UINT32_MAX
    const uintptr_t max_address = UINTPTR_MAX;

    while (cur_address < max_address) {
        MEMORY_BASIC_INFORMATION info;

        if (!VirtualQueryEx(handle, reinterpret_cast<void *>(cur_address), &info, sizeof(info))) {
            std::cout << std::format("couldn't query at {}\n", cur_address);

            // can't really recover from this gracefully
            return;
        }

        callback(&info);

        // don't just add region size to current address because we might have
        // missed the actual region boundary
        cur_address = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    }
}

int main(int argc, char *argv[]) {
    // give sample.exe some time to start up
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const std::string process_name = argc == 3 ? argv[2] : "sample.exe";

    const auto handle = utils::get_handle(process_name);
    if (!handle) {
        std::cout << std::format("couldn't get handle to '{}'\n", process_name);
        return 1;
    }

    std::cout << std::format("got handle {} to '{}'\n", handle, process_name);

    map_regions(handle, [](MEMORY_BASIC_INFORMATION *info) {
        std::cout << std::format("region at {}\n", info->BaseAddress);
    });

    return 0;
}
