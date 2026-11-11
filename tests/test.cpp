#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
static unsigned checks = 0;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            std::cerr << "FAIL line " << __LINE__ << ": " << #x << "\n";                           \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

#include "recorder.hpp"
int main() {
    recorder::Debounce d;
    CHECK(d.edge(UINT32_MAX - 100));
    CHECK(!d.edge(100));
    CHECK(d.edge(500));
    CHECK(d.count == 2 && d.rejected == 1);
    recorder::Recorder r;
    r.update(UINT32_MAX - 49999, UINT32_MAX - 4, UINT32_MAX - 49999, true, 2);
    auto v = r.update(50000, 5, 50000, true, 2);
    CHECK(std::abs(v.rpm - 3000) < 1e-6);
    CHECK(!v.stalled);
    CHECK(r.update(2050000, 5, 50000, true, 2).stalled);
    CHECK(r.update(2150000, 6, 2150000, true, 2).stalled == false);
    recorder::Recorder idle;
    CHECK(idle.update(1, 0, 0, false, 2).stalled);
    CHECK(idle.update(1, 0, 0, false, 0).rpm == 0);
    // Idle must remain latched across full microsecond timer wraps.
    recorder::Recorder longidle;
    longidle.update(100, 1, 100, true, 2);
    uint32_t t = 100;
    for (unsigned i = 0; i < 90000; ++i) {
        t += 100000;
        auto out = longidle.update(t, 1, 100, true, 2);
        if (i >= 19)
            CHECK(out.stalled);
    }
    uint32_t n;
    for (auto s : {"PPR 1", "PPR 2", "PPR 32"})
        CHECK(recorder::parse_ppr(s, n));
    for (auto s : {"PPR 0", "PPR 33", "PPR -1", "PPR 4294967296", "PPR 2foo", "PPR ", "ACK 2"})
        CHECK(!recorder::parse_ppr(s, n));
    recorder::Config c;
    c.seal();
    CHECK(c.valid());
    auto saved = c;
    CHECK(saved.valid());
    c.ppr = 0;
    c.seal();
    CHECK(!c.valid());
    for (size_t i = 0; i < sizeof(saved); ++i) {
        auto bad = saved;
        reinterpret_cast<uint8_t *>(&bad)[i] ^= 1;
        CHECK(!bad.valid());
    }
    core::Line<32> line;
    for (int i = 0; i < 100; ++i)
        CHECK(!line.feed('x'));
    CHECK(!line.feed('\n'));
    bool done = false;
    for (char x : std::string("PPR 3\r\n"))
        done |= line.feed(x);
    CHECK(done && recorder::parse_ppr(line.data, n) && n == 3);
    core::Line<32> binary_line;
    const char binary_command[] = {'P', 'P', 'R', ' ', '2', 0, 'x', '\n'};
    for (char byte : binary_command)
        CHECK(!binary_line.feed(byte));
    std::cout << "PASS " << checks
              << " assertions: debounce, rollover, long idle, parser, configuration\n";
}
