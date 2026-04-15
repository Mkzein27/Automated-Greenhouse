#  Water Level Float Sensor API

## Overview

This module provides an interface to a float-based water level sensor using a GPIO input on an ESP-based system.

It supports:

-   Simple digital water level detection
    
-   Debounced readings for noise reduction
    
-   Timestamped results for control logic
    

The sensor is assumed to be a **normally-closed (NC) float switch**:

-   `LOW (0)` → Water present
    
-   `HIGH (1)` → Water level low
    

----------

##  Data Types

### `water_level_reading_t`

Represents a water level reading.

```c
typedef struct {
    bool water_present;   // true = water present, false = low level
    bool valid;           // true if reading is valid
    uint64_t timestamp;   // Time in milliseconds
} water_level_reading_t;

```

----------

##  API Functions

### `bool water_level_float_init(void)`

Initializes the float sensor.

**Behavior:**

-   Configures GPIO as input with internal pull-up
    
-   Performs an initial reading
    
-   Logs current water state
    

**Returns:**

-   `true` → Initialization successful
    
-   `false` → GPIO configuration failed
    

----------

### `water_level_reading_t water_level_float_read(void)`

Performs a single raw reading from the float switch.

**Behavior:**

-   Reads GPIO level
    
-   Converts signal to water state:
    
    -   `0 → water_present = true`
        
    -   `1 → water_present = false`
        

**Returns:**

-   Valid reading (`valid = true`)
    
-   Invalid reading if sensor not initialized (`valid = false`)
    

----------

### `water_level_reading_t water_level_read_debounced(void)`

Performs multiple readings and applies debouncing.

**Behavior:**

-   Takes 5 samples
    
-   Requires at least 4 agreeing readings
    
-   Filters out noise and switch bouncing
    
-   Adds small delays between samples
    

**Returns:**

-   Stable debounced reading
    
-   Always `valid = true` if initialized
    

----------

## Configuration

Constant

Description

`WATER_LEVEL_FLOAT_GPIO`

GPIO pin connected to float switch

`WATER_LEVEL_DEBOUNCE_MS`

Total debounce duration

----------

## Signal Logic

GPIO Level

Water State

`0`

Water PRESENT

`1`

Water LOW

> This assumes a **normally-closed (NC)** float switch.  
> If using normally-open (NO), logic must be inverted.

----------
