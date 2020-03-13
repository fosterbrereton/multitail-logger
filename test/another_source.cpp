#include <chrono>
#include <iostream>
#include <thread>

#include "../multitail/multitail.hpp"

using namespace std::chrono_literals;

void another_function() {
    std::thread([]{
        for (std::size_t i(0); i < 100; ++i) {
            auto window(std::to_string(std::rand() % 12));
            auto value(std::rand() % 1024);

            std::string message{"(t1): to " + window + ": " + std::to_string(value) + "\n"};
            std::cout << message;

            multitail::post(window, value);

            std::this_thread::sleep_for(0.25s);
        }
    }).detach();
}
