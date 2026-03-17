#ifndef TIMING_UTILS_H
#define TIMING_UTILS_H

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//extremely accurate microsecond delay function using busy-wait loop
static inline void delay_us(uint32_t us) {
    uint64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us) {
        // Busy wait
    }
}

#endif // TIMING_UTILS_H
