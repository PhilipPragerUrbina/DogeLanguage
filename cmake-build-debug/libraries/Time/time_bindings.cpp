#include <chrono>
#include <thread>
#include <iostream>
#include <ctime>
#pragma warning(disable : 4996)
//delay in milliseconds
extern "C"  void delay_int(int time) {
    using namespace std::this_thread;
    using namespace std::chrono;
    sleep_for(milliseconds (time));
}

extern "C"  int getTimeSeconds() {
    // get the current time
    const auto now     = std::chrono::system_clock::now();

    // transform the time into a duration since the epoch
    const auto epoch   = now.time_since_epoch();

    // cast the duration into seconds
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);

    // return the number of seconds
    return seconds.count();
}

extern "C"  char* getDateChars() {
    auto time = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(time);
    return  std::ctime(&end_time);
}

