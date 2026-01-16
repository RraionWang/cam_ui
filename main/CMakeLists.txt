file(GLOB_RECURSE APP_SOURCES
    "*.c"
    "*.cpp"
)

idf_component_register(
    SRCS ${APP_SOURCES}
    PRIV_REQUIRES
        nvs_flash
        esp_psram
        esp_lcd
        esp_driver_jpeg
        fatfs
    INCLUDE_DIRS "." "ui"
)
