#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <set>
using Clock = std::chrono::steady_clock;

#include "recorder.hpp"
int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "--soak") {
        double duration = argc > 2 ? std::atof(argv[2]) : 10;
        auto start = Clock::now();
        uint64_t iterations = 0;
        recorder::Recorder r;
        uint32_t t = 0, count = 0;
        volatile double sum = 0;
        do {
            for (int j = 0; j < 10000; ++j) {
                t += 100000;
                count += 10;
                sum += r.update(t, count, t, true, 2).rpm;
            }
            iterations += 10000;
        } while (std::chrono::duration<double>(Clock::now() - start).count() < duration);
        std::cout << std::setprecision(12) << "{\"duration_seconds\":"
                  << std::chrono::duration<double>(Clock::now() - start).count()
                  << ",\"iterations\":" << iterations << ",\"checksum\":" << sum << "}\n";
        return 0;
    }
    recorder::Recorder r;
    uint32_t now = 0, count = 0, last = 0;
    r.update(0, 0, 0, true, 2);
    const uint32_t schedule[] = {80000, 100000, 120000, 90000, 110000};
    std::cout << "step,time_us,dt_us,count,true_rpm,baseline_rpm,final_rpm,stalled\n";
    for (unsigned step = 0; step < 200; ++step) {
        uint32_t dt = schedule[step % 5];
        now += dt;
        unsigned delta = step < 150 ? dt / 10000 : 0;
        count += delta;
        if (delta)
            last = now;
        auto out = r.update(now, count, last, true, 2);
        double baseline = delta * 600.0 / 2;
        std::cout << step << ',' << now << ',' << dt << ',' << count << ','
                  << (step < 150 ? 3000 : 0) << ',' << baseline << ',' << out.rpm << ','
                  << out.stalled << '\n';
    }
}
