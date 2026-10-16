#include "crash_report.h"

#include <cstdio>

void QtCrashReport::Present(const char* log_file) {
    std::fprintf(stderr, "CE Runtime Foundation stopped on an unexpected error. Log: %s\n",
                 log_file[0] != '\0' ? log_file : "no log file");
    std::fflush(stderr);
}
