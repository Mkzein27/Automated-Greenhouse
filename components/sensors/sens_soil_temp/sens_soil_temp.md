# Soil Temperature Sensor API
## Data Types

### `soil_temp_reading_t`

Represents a temperature reading.

```c
typedef struct {
    float temperature;       // Temperature in °C
    bool valid;              // True if reading is valid
    uint64_t timestamp;      // Timestamp in milliseconds
    char error_message[...]  // Error message if invalid
} soil_temp_reading_t;

```

----------

## API Functions

### `bool soil_temp_init(void)`

Initializes the soil temperature sensor.

**Behavior:**

-   Configures GPIO with internal pull-up
    
-   Verifies sensor presence
    
-   Sets default resolution (12-bit)
    

**Returns:**

-   `true` → Initialization successful
    
-   `false` → GPIO configuration failed or sensor not detected
    

----------

### `bool soil_temp_is_connected(void)`

Checks whether the sensor is currently connected and responsive.

**Returns:**

-   `true` → Sensor is present
    
-   `false` → Sensor not connected or not initialized
    

----------

### `soil_temp_reading_t soil_temp_read(void)`

Performs a single temperature reading.

**Behavior:**

-   Sends temperature conversion command
    
-   Waits ~750 ms (for 12-bit resolution)
    
-   Reads scratchpad memory of the sensor
    
-   Validates data using CRC
    
-   Converts raw value to °C
    

**Returns:**

-   Reading with the soil_temp_reading_t data structure, with valid = true or false depending on validity of temp if within range or not
    

----------

### `soil_temp_reading_t soil_temp_read_filtered(uint8_t num_samples)`

Performs multiple readings and applies median filtering.

> Note: `num_samples` is internally fixed to `SOIL_TEMP_SAMPLES`it can be adjusted from the module itself.

**Behavior:**

-   Collects multiple readings
    
-   Ignores invalid samples
    
-   Sorts valid samples
    
-   Returns median (or average of two middle values)
    

**Returns:**

-   Filtered valid reading
    
-   Error if no valid samples
    
----------

### `bool soil_temp_set_resolution(uint8_t resolution)`

Sets sensor resolution.

**Valid values:**

Resolution

Conversion Time

9-bit  ---> 94 ms

10-bit ---> 188 ms

11-bit ---> 375 ms

12-bit ---> 750 ms

**Returns:**

-   `true` → Success
    
-   `false` → Invalid resolution or communication error
    

----------

### `bool soil_temp_get_rom_code(uint8_t *rom_code)`

Retrieves the sensor’s unique 64-bit ROM code.

**Parameters:**

-   `rom_code` → Pointer to 8-byte buffer
    

**Returns:**

-   `true` → Success
    
-   `false` → Not initialized or invalid pointer
    

----------

## Configuration

Constant

Description

`SOIL_TEMP_GPIO`

GPIO pin used for sensor

`SOIL_TEMP_RESOLUTION`

Default resolution (12-bit)

`SOIL_TEMP_SAMPLES`

Number of samples for filtering
