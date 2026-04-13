#include "water_level_float.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char *TAG = "water_float";
static bool sensor_initialized = false;

// PUBLIC API 

bool water_level_float_init(void) {
    
    // Configure GPIO as input with pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WATER_LEVEL_FLOAT_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,      // Internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO: %s", esp_err_to_name(err));
        return false;
    }
    
    sensor_initialized = true;
    
    // Test initial reading
    water_level_reading_t initial = water_level_float_read();
    ESP_LOGI(TAG, "Initial state: Water %s", 
             initial.water_present ? "PRESENT" : "LOW");
    
    ESP_LOGI(TAG, "Float switch initialized successfully");
    return true;
}

water_level_reading_t water_level_float_read(void) {
    water_level_reading_t reading = {
        .water_present = false,
        .timestamp = esp_timer_get_time() / 1000,
        .valid = false
    };
    
    if (!sensor_initialized) {
        ESP_LOGE(TAG, "Sensor not initialized! Call water_level_float_init() first.");
        return reading;
    }
    
    // Read GPIO state
    int gpio_level = gpio_get_level(WATER_LEVEL_FLOAT_GPIO);

    reading.water_present = (gpio_level == 0); // For NC switch, 0 means water present, 1 means low
    
    reading.valid = true;
    
    ESP_LOGD(TAG, "GPIO=%d → Water %s", 
             gpio_level, reading.water_present ? "PRESENT" : "LOW");
    
    return reading;
}

water_level_reading_t water_level_read_debounced(void) {
    const uint8_t NUM_SAMPLES = 5;
    const uint8_t REQUIRED_CONSENSUS = 4;  // 4 out of 5 must agree
    
    if (!sensor_initialized) {
        water_level_reading_t reading = {.valid = false};
        ESP_LOGE(TAG, "Sensor not initialized!");
        return reading;
    }
    
    ESP_LOGD(TAG, "Reading with debouncing (%d samples)", NUM_SAMPLES);
    
    uint8_t water_present_count = 0;
    
    // Take multiple samples
    for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
        water_level_reading_t sample = water_level_float_read();
        if (sample.valid) {
            if (sample.water_present) {
                water_present_count++;
            }
        }
        else {
            ESP_LOGW(TAG, "Sample %d invalid - ignoring", i);
        }
        // Small delay between samples
        if (i < NUM_SAMPLES - 1) {
            vTaskDelay(pdMS_TO_TICKS(WATER_LEVEL_DEBOUNCE_MS / NUM_SAMPLES));
        }
    }
    
    water_level_reading_t result = {
        .water_present = (water_present_count >= REQUIRED_CONSENSUS),
        .timestamp = esp_timer_get_time() / 1000,
        .valid = true
    };
    ESP_LOGI(TAG, "Debounced result: %d/%d samples = Water %s", 
             water_present_count, NUM_SAMPLES,
             result.water_present ? "PRESENT" : "LOW");
    
    return result;
}