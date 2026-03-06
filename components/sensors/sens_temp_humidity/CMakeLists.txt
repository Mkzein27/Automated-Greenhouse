idf_component_register(
    #tells the compiler this is the actual code to turn into binary
    SRCS "sens-light-intensity.c"
    # tells compiler the .h file is in this same directory
    INCLUDE_DIRS "."
    # tells compiler to link the driver and adc libraries
    REQUIRES driver esp_adc
)