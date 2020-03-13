#include <chrono>
#include <iostream>
#include <thread>

#include "another_source.hpp"
#include "../multitail/multitail.hpp"

using namespace std::chrono_literals;
    
int main(int argc, const char * argv[]) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    another_function();

    for (std::size_t i(0); i < 100; ++i) {
        auto window(std::to_string(std::rand() % 12));
        auto value(std::rand() % 1024);

        std::string message{"(t0): to " + window + ": " + std::to_string(value) + "\n"};
        std::cout << message;

        multitail::post(window, value);

        std::this_thread::sleep_for(0.25s);
    }

    return 0;
}
