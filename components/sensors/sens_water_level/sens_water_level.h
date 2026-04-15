#ifndef WATER_LEVEL_FLOAT_H
#define WATER_LEVEL_FLOAT_H

#include <stdint.h>
#include <stdbool.h>

/* Water Level Float Switch Module
 Simple vertical float switch for low water detection.
 When water is low, float drops and switch opens.
 Hardware: Vertical Float Switch
 Type: Normally Closed (NC)
 GPIO Pin: 18 (configurable)
 */
// Configuration
#define WATER_LEVEL_FLOAT_GPIO  18      // GPIO pin for float switch
#define WATER_LEVEL_DEBOUNCE_MS 50      // Debounce time (switch bounce)

// Reading structure
typedef struct {
    bool water_present;         // true = water OK, false = water LOW
    uint32_t timestamp;         // Reading timestamp (ms)
    bool valid;                 // Reading validity
} water_level_reading_t;

/* Initialize float switch
return true if successful
 */
bool water_level_float_init(void);

/* Read current water level state
 * 
 water_level_reading_t with current state
 */
water_level_reading_t water_level_float_read(void);

/* Read with debouncing (filters switch bounce)
 Takes multiple readings over debounce period to ensure stable reading.
 return water_level_reading_t with debounced state
 */
water_level_reading_t water_level_float_read_debounced(void);