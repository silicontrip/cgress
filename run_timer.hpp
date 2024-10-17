#ifndef SILICONTRIP_RUN_TIMER_HPP
#define SILICONTRIP_RUN_TIMER_HPP

#include <chrono>

using namespace std::chrono;
using std::chrono::high_resolution_clock;

namespace silicontrip {

    class run_timer {
    private:
        high_resolution_clock::time_point start_time;
        high_resolution_clock::time_point split_time;

    public:
        void start();
        double split();
        double stop();
    };

} // namespace silicontrip

#endif // SILICONTRIP_RUN_TIMER_HPP
