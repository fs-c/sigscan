#include <iostream>
#include <thread>
#include <chrono>

int global_index = 0;

int main() {
    while (true) {
        std::cout << global_index++ << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
