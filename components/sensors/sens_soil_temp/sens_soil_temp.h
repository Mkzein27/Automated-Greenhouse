// ifndef and define guards to prevent multiple inclusions of this header file
#ifndef SOIL_TEMP_SENSOR_H
//definition statement in case it is not yet defined
#define SOIL_TEMP_SENSOR_H
// include standard integer and boolean types for consistent data types
#include <stdint.h>
#include <stdbool.h>

/* This module handles the DS18B20 OneWire temperature sensor 
for measuring soil temperature in the greenhouse. 
Hardware: DS18B20 waterproof probe
Protocol: OneWire (Dallas/Maxim)
 * GPIO Pin: 4
 * Resolution: 9-12 bits configurable (default 12-bit = 0.0625°C)
 */

// Configuration
#define SOIL_TEMP_GPIO          4           // GPIO pin for DS18B20 data line
#define SOIL_TEMP_RESOLUTION    12          // 12-bit resolution (0.0625°C)
#define SOIL_TEMP_SAMPLES       5           // Number of samples for filtering

// Valid temperature range for greenhouse soil
#define SOIL_TEMP_MIN          -5.0         // Minimum valid temperature (°C)
#define SOIL_TEMP_MAX          60.0         // Maximum valid temperature (°C)

// Data structure for soil temperature reading
typedef struct {
    float temperature;          // Temperature in Celsius
    bool valid;                 // Reading validity flag
    uint32_t timestamp;         // Reading timestamp (milliseconds)
    char error_message[64];     // Error description if invalid
} soil_temp_reading_t;

/**Initialize the soil temperature sensor
 * Configures GPIO pin for OneWire communication and
 * searches for DS18B20 sensor on the bus.
 * return true if initialization successful, false otherwise
 */
bool soil_temp_init(void);

/*Read temperature from DS18B20 sensor
 * Performs a single temperature reading with conversion time.
 * Returns immediately if sensor not present or conversion fails.
 * returns the soil_temp_reading_t structure with temperature and validity
 */
soil_temp_reading_t soil_temp_read(void);

/**
 * Read temperature with median filtering
 * Takes multiple samples and returns the median value to
 * filter out noise and transient spikes.
 * @param num_samples Number of samples to take (default: SOIL_TEMP_SAMPLES)
 * @return soil_temp_reading_t structure with filtered temperature
 */
soil_temp_reading_t soil_temp_read_filtered(float num_samples);

/**
 * @brief Check if sensor is connected
 * 
 * Performs a OneWire search to verify DS18B20 is on the bus.
 * 
 * @return true if sensor detected, false otherwise
 */
bool soil_temp_is_connected(void);

/**
 * @brief Set sensor resolution
 * 
 * @param resolution Resolution in bits (9, 10, 11, or 12)
 * @return true if successful, false if invalid resolution
 */
bool soil_temp_set_resolution(uint8_t resolution);

/**
 * @brief Get sensor ROM code (64-bit unique ID)
 * 
 * @param rom_code Pointer to 8-byte array to store ROM code
 * @return true if successful, false if sensor not found
 */
bool soil_temp_get_rom_code(uint8_t *rom_code);

#endif // SOIL_TEMP_SENSOR_H