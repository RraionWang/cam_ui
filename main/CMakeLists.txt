idf_component_register(SRCS "main.c" "lcd.c"  "my_fs.c" "key.c" "cam.c" 
                           "ui/ui_font_cn_font_16.c"
                           "ui/eez-flow.cpp"                    
                            "ui/images.c"
                            "ui/screens.c"
                            "ui/styles.c"
                            "ui/ui.c"
                            "ui/actions.c"
                            "ui/vars.c"
                            
 PRIV_REQUIRES nvs_flash esp_psram  esp_lcd esp_driver_jpeg fatfs 
                       INCLUDE_DIRS "." "ui")
