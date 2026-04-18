#include "soil_temp_sensor.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>
#include "utilities/timing_utils.h"

static const char *TAG = "soil_temp";

// OneWire timing parameters (in microseconds)
#define ONEWIRE_RESET_LOW_TIME      480
#define ONEWIRE_RESET_WAIT_TIME     70
#define ONEWIRE_RESET_SAMPLE_TIME   410
#define ONEWIRE_WRITE_1_LOW_TIME    10
#define ONEWIRE_WRITE_0_LOW_TIME    65
#define ONEWIRE_WRITE_RECOVERY_TIME 5
#define ONEWIRE_READ_LOW_TIME       3
#define ONEWIRE_READ_SAMPLE_TIME    10
#define ONEWIRE_READ_RECOVERY_TIME  53

// DS18B20 Commands
#define DS18B20_RESET 0xCC //reset the DS18B20 device.
#define DS18B20_SKIP_ROM 0xCC //used when only one DS18B20 is on the one-wire bus.
#define DS18B20_MATCH_ROM 0x55 //used when communicating with a specific DS18B20 device on the one-wire bus. 
#define DS18B20_SEARCH_ROM 0xF0 //search for all DS18B20 devices on the one-wire bus.
#define DS18B20_READ_SCRATCHPAD 0xBE //read the scratchpad memory of the DS18B20 device, which contains temperature data and configuration settings.
#define DS18B20_WRITE_SCRATCHPAD 0x4E //write to the scratchpad memory of the DS18B20 device, typically for setting configuration parameters.
#define DS18B20_COPY_SCRATCHPAD 0x48 //copy the contents of the scratchpad memory to the EEPROM of the DS18B20 device, making the settings permanent.
#define DS18B20_RECALL_EEPROM 0xB8 //recall the EEPROM contents back to the scratchpad memory of the DS18B20 device.
#define DS18B20_ALARM_SEARCH 0xEC //search for DS18B20 devices that have triggered an alarm condition based on their temperature thresholds.
#define DS18B20_CONVERT_T 0x44 //initiate a temperature conversion on the DS18B20. measure temperature & store result in  scratchpad memory
#define DS18B20_RESUME 0xC2 //resume communication with DS18B20 device after a reset or power-up, allowing sending commands for full ROM search 

#define SOIL_TEMP_GPIO 4
#define SOIL_TEMP_RESOLUTION 12
#define SOIL_TEMP_SAMPLES 5
// Static variables
static bool sensor_initialized = false; 
static uint8_t sensor_rom[8] = {0}; // Store sensor ROM code

//OneWire reset pulse
//return true if device present, false otherwise
static bool onewire_reset(void) {
    bool presence;
    
    // Pull line low for reset pulse
    gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(SOIL_TEMP_GPIO, 0);
    delay_us(ONEWIRE_RESET_LOW_TIME);
    
    // Release line and wait for presence pulse
    gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_INPUT);
    delay_us(ONEWIRE_RESET_WAIT_TIME);
    
    // Sample presence pulse
    presence = !gpio_get_level(SOIL_TEMP_GPIO);
    
    // Wait for presence pulse to finish
    delay_us(ONEWIRE_RESET_SAMPLE_TIME);
    
    return presence;
}

// Write a single bit on OneWire bus
static void onewire_write_bit(uint8_t bit) {
    if (bit) {
        // Write '1' bit
        gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(SOIL_TEMP_GPIO, 0);
        delay_us(ONEWIRE_WRITE_1_LOW_TIME);
        gpio_set_level(SOIL_TEMP_GPIO, 1);
        delay_us(ONEWIRE_WRITE_RECOVERY_TIME);
    } else {
        // Write '0' bit
        gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_OUTPUT);
        gpio_set_level(SOIL_TEMP_GPIO, 0);
        delay_us(ONEWIRE_WRITE_0_LOW_TIME);
        gpio_set_level(SOIL_TEMP_GPIO, 1);
        delay_us(ONEWIRE_WRITE_RECOVERY_TIME);
    }
}

//Read a single bit from OneWire bus
static uint8_t onewire_read_bit(void) {
    uint8_t bit;
    
    // Pull line low briefly
    gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(SOIL_TEMP_GPIO, 0);
    delay_us(ONEWIRE_READ_LOW_TIME);
    
    // Release and sample
    gpio_set_direction(SOIL_TEMP_GPIO, GPIO_MODE_INPUT);
    delay_us(ONEWIRE_READ_SAMPLE_TIME);
    bit = gpio_get_level(SOIL_TEMP_GPIO);
    
    // Recovery time
    delay_us(ONEWIRE_READ_RECOVERY_TIME);
    
    return bit;
}

//Write a byte on OneWire bus
static void onewire_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        onewire_write_bit(data & 0x01);
        data >>= 1;
    }
}

// Read a byte from OneWire bus
static uint8_t onewire_read_byte(void) {
    uint8_t data = 0;
    
    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (onewire_read_bit()) {
            data |= 0x80;
        }
    }
    
    return data;
}

// DS18B20 SPECIFIC FUNCTIONS

// Calculate CRC8 for DS18B20 data validation
static uint8_t calculate_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8C;
            }
            byte >>= 1;
        }
    }
    
    return crc;
}

// Comparison function for qsort (median filter)
static int compare_floats(const void *a, const void *b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);// Returns 1 if fa > fb, -1 if fa < fb, 0 if equal
}

// PUBLIC API IMPLEMENTATION
// Initialize the soil temperature sensor
bool soil_temp_init(void) {
    ESP_LOGI(TAG, "Initializing DS18B20 soil temperature sensor on GPIO %d", SOIL_TEMP_GPIO);
    
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SOIL_TEMP_GPIO)
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Enable internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down 
        .intr_type = GPIO_INTR_DISABLE,
    };
    // error handling for GPIO config
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(err));
        return false;
    }
    
    // Check if sensor is present
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for sensor to stabilize
    
    if (!onewire_reset()) {
        ESP_LOGW(TAG, "No DS18B20 device detected on bus");
        return false;
    }
    sensor_initialized = true;
    ESP_LOGI(TAG, "DS18B20 initialized successfully");
    
    // Set resolution to 12-bit (optional)
    soil_temp_set_resolution(SOIL_TEMP_RESOLUTION);
    
    return true;
    
}

bool soil_temp_is_connected(void) {
    if (!sensor_initialized) {
        return false;
    }
    
    return onewire_reset();
}

soil_temp_reading_t soil_temp_read(void) {
    soil_temp_reading_t reading = {
        .temperature = 0.0,
        .valid = false,
        .timestamp = esp_timer_get_time() / 1000,  // Convert to milliseconds
        .error_message = ""
    };
    
    if (!sensor_initialized) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                 "Sensor not initialized");
        ESP_LOGE(TAG, "%s", reading.error_message);
        return reading;
    }
    
    // Reset and check presence
    if (!onewire_reset()) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                 "Sensor not present");
        ESP_LOGW(TAG, "%s", reading.error_message);
        return reading;
    }
    
    // Start temperature conversion
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_CONVERT_T);
    
    // Wait for conversion (750ms for 12-bit resolution)
    vTaskDelay(pdMS_TO_TICKS(750));
    
    // Reset and read scratchpad
    if (!onewire_reset()) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                 "Lost connection during conversion");
        ESP_LOGW(TAG, "%s", reading.error_message);
        return reading;
    }
    
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_READ_SCRATCHPAD);
    
    // Read 9 bytes of scratchpad data
    uint8_t scratchpad[9];
    for (int i = 0; i < 9; i++) {
        scratchpad[i] = onewire_read_byte();
    }
    
    // Verify CRC
    uint8_t crc = calculate_crc8(scratchpad, 8);
    if (crc != scratchpad[8]) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                 "CRC error (expected 0x%02X, got 0x%02X)", crc, scratchpad[8]);
        ESP_LOGW(TAG, "%s", reading.error_message);
        return reading;
    }
    
    // Extract temperature from bytes 0 and 1
    int16_t raw_temp = (scratchpad[1] << 8) | scratchpad[0];
    // converting raw to decimal point by shifting right 4 bits (divide by 16)
    float temperature = (float)raw_temp / 16.0;
    
    // Validate range
    if (temperature < SOIL_TEMP_MIN || temperature > SOIL_TEMP_MAX) {
        snprintf(reading.error_message, sizeof(reading.error_message), 
                 "Temperature out of range: %.2f°C", temperature);
        ESP_LOGW(TAG, "%s", reading.error_message);
        return reading;
    }
    
    // Success!
    reading.temperature = temperature;
    reading.valid = true;
    
    ESP_LOGD(TAG, "Temperature: %.2f°C", temperature);
    
    return reading;
}

soil_temp_reading_t soil_temp_read_filtered(uint8_t num_samples) {
    num_samples = SOIL_TEMP_SAMPLES;
    
    ESP_LOGI(TAG, "Reading %d samples for median filtering", num_samples);
    
    float samples[SOIL_TEMP_SAMPLES ];
    uint8_t valid_count = 0;
    
    // Collect samples
    for (uint8_t i = 0; i < num_samples; i++) {
        soil_temp_reading_t reading = soil_temp_read();
        
        if (reading.valid) {
            samples[valid_count] = reading.temperature;
            valid_count++;
        }
        // Small delay between samples
        if (i < num_samples - 1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
    soil_temp_reading_t result = {
        .temperature = 0.0,
        .valid = false,
        .timestamp = esp_timer_get_time() / 1000,
        .error_message = "no valid samples obtained"
    };
    
    if (valid_count == 0) {
        snprintf(result.error_message, sizeof(result.error_message), 
                 "All %d samples invalid", num_samples);
        ESP_LOGE(TAG, "%s", result.error_message);
        return result;
    }
    
    if (valid_count < num_samples / 2) {
        ESP_LOGW(TAG, "Only %d/%d samples valid", valid_count, num_samples);
    }
    
    // Sort samples and get median
    qsort(samples, valid_count, sizeof(float), compare_floats);
    
    if (valid_count % 2 == 0) {
        // Even number: average of two middle values
        result.temperature = (samples[valid_count/2 - 1] + samples[valid_count/2]) / 2.0;
    } else {
        // Odd number: middle value
        result.temperature = samples[valid_count/2];
    }
    result.valid = true;
    
    ESP_LOGI(TAG, "Filtered temperature: %.2f°C (from %d valid samples)", 
             result.temperature, valid_count);
    return result;
}

bool soil_temp_set_resolution(uint8_t resolution) {
    if (resolution < 9 || resolution > 12) {
        ESP_LOGE(TAG, "Invalid resolution: %d (must be 9-12)", resolution);
        return false;
    }
    
    if (!sensor_initialized) {
        ESP_LOGE(TAG, "Sensor not initialized");
        return false;
    }
    
    ESP_LOGI(TAG, "Setting resolution to %d bits", resolution);
    // Reset and write scratchpad
    if (!onewire_reset()) {
        ESP_LOGE(TAG, "Sensor not present");
        return false;
    }
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_WRITE_SCRATCHPAD);
    // Write TH, TL, and configuration registers
    onewire_write_byte(0x00);  // TH (not used)
    onewire_write_byte(0x00);  // TL (not used)
    // Configuration: resolution is in bits 5-6
    // 9-bit: 0x1F, 10-bit: 0x3F, 11-bit: 0x5F, 12-bit: 0x7F
    uint8_t config = ((resolution - 9) << 5) | 0x1F;
    onewire_write_byte(config);
    
    // Copy scratchpad to EEPROM to make it permanent
    if (!onewire_reset()) {
        return false;
    }
    onewire_write_byte(DS18B20_CMD_SKIP_ROM);
    onewire_write_byte(DS18B20_CMD_COPY_SCRATCHPAD);
    vTaskDelay(pdMS_TO_TICKS(10));  // Wait for EEPROM write
    ESP_LOGI(TAG, "Resolution set successfully");
    return true;
}

bool soil_temp_get_rom_code(uint8_t *rom_code) {
    if (!sensor_initialized || rom_code == NULL) {
        return false;
    }
    memcpy(rom_code, sensor_rom, 8);
    return true;
}