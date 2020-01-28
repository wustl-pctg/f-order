#include <cstdarg> // va_list
#include <cstdlib> // exit

#include "debug.hpp"

namespace debug {

void printf(DebugLevel level, const char *fmt, ...) {
    if(g_level & level) {
        std::va_list l;
        va_start(l, fmt);
        std::vfprintf(stderr, fmt, l);
        va_end(l);
    }
}

// Print out the error message and exit
[[noreturn]] void die(const char *fmt, ...) {

    std::va_list l;
    std::fprintf(stderr, "=================================================\n");
    std::fprintf(stderr, "racedetector: fatal error\n");

    va_start(l, fmt);
    std::vfprintf(stderr, fmt, l);
    std::fprintf(stderr, "=================================================\n");
    fflush(stderr);
    va_end(l);
    std::exit(1);
}

} // namespace debug
