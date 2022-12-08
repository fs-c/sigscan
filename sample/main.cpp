#include <iostream>
#include <thread>
#include <chrono>

int main() {
    int i = 0;

    while (true) {
        std::cout << i++ << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
