idf_component_register(
    SRCS "hardware.c"
         "wifi_connection.c"
         "wifi_connect.c"
         "touchpad.c"
    INCLUDE_DIRS "." "include"
    REQUIRES
        "bus-i2c"
        "spi-st7789v"
        "pax-graphics"
        "wpa_supplicant"
        "nvs_flash"
)
