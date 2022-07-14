#include <chrono>
#include <thread>
//delay
extern "C"  void delay_int(int time) {
    using namespace std::this_thread;
    using namespace std::chrono;
    sleep_for(milliseconds (time));
}