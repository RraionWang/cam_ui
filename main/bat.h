#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_adc(void);
void adc_task(void *pvParam); 


#ifdef __cplusplus
}
#endif
