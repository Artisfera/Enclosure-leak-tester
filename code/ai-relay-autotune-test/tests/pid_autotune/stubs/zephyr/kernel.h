#pragma once
#include <stdint.h>
extern int64_t test_time;
static inline int64_t k_uptime_get(void) { return test_time; }
static inline int64_t k_uptime_delta(int64_t *previous)
{
    int64_t dt = test_time - *previous;
    *previous = test_time;
    return dt;
}
