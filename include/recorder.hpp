#pragma once
#include "common.hpp"
namespace recorder {
struct Config {
    uint32_t magic = 0x50554c31, version = 1, ppr = 2, checksum = 0;
    void seal() {
        checksum = core::crc32(this, offsetof(Config, checksum));
    }
    bool valid() const {
        return magic == 0x50554c31 && version == 1 && ppr >= 1 && ppr <= 32 &&
               checksum == core::crc32(this, offsetof(Config, checksum));
    }
};
inline bool parse_ppr(const char *s, uint32_t &v) {
    return std::strncmp(s, "PPR ", 4) == 0 && core::parse_u32(s + 4, v) && v >= 1 && v <= 32;
}
struct Debounce {
    uint32_t last = 0, count = 0, rejected = 0;
    bool seen = false;
    bool edge(uint32_t now) {
        if (seen && core::elapsed(now, last) < 500) {
            ++rejected;
            return false;
        }
        seen = true;
        last = now;
        ++count;
        return true;
    }
};
struct Result {
    double rpm = 0;
    bool stalled = true;
    uint64_t runtime_us = 0;
};
struct Recorder {
    uint32_t previous_count = 0, previous_time = 0, idle_age = 2000000;
    uint64_t runtime = 0;
    bool initialized = false;
    Result update(uint32_t now, uint32_t count, uint32_t last_edge, bool seen, uint32_t ppr) {
        uint32_t dt = core::elapsed(now, previous_time), dc = count - previous_count;
        // Saturation prevents an old edge becoming recent again after a timer wrap.
        if (!seen)
            idle_age = 2000000;
        else if (!initialized || dc)
            idle_age = core::elapsed(now, last_edge);
        else
            idle_age =
                uint32_t((uint64_t(idle_age) + dt) > 2000000 ? 2000000 : (uint64_t(idle_age) + dt));
        bool stalled = idle_age >= 2000000;
        double rpm = initialized && dt && ppr ? double(dc) * 60000000.0 / (double(dt) * ppr) : 0;
        if (initialized && dt && !stalled)
            runtime += dt;
        previous_count = count;
        previous_time = now;
        initialized = true;
        return {rpm, stalled, runtime};
    }
};
} // namespace recorder
