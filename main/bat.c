#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "ADC1_CH2";

/* ===== ADC 配置 ===== */
#define ADC_CH          ADC_CHANNEL_2
#define ADC_ATTEN       ADC_ATTEN_DB_12   // 0~3.3V
#define ADC_UNIT        ADC_UNIT_1

/* ===== 校准函数声明 ===== */
static bool adc_calibration_init(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_atten_t atten,
    adc_cali_handle_t *out_handle
);

void init_adc(void)
{
    int adc_raw = 0;
    int voltage_mv = 0;

    /* 1. 创建 ADC1 oneshot 单元 */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    /* 2. 配置 ADC 通道 */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CH, &chan_cfg));

    /* 3. 初始化校准 */
    adc_cali_handle_t cali_handle = NULL;
    bool calibrated = adc_calibration_init(
        ADC_UNIT,
        ADC_CH,
        ADC_ATTEN,
        &cali_handle
    );

    /* 4. 采样循环 */
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CH, &adc_raw));
        ESP_LOGI(TAG, "Raw ADC: %d", adc_raw*3);

        if (calibrated) {
            ESP_ERROR_CHECK(
                adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv)
            );
            ESP_LOGI(TAG, "Voltage: %d mV", voltage_mv*3);
             ESP_LOGI(TAG, "Percent: %f %%", ((float)voltage_mv*3-3700)/(4200-3700));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ===== ADC 校准实现 ===== */
static bool adc_calibration_init(
    adc_unit_t unit,
    adc_channel_t channel,
    adc_atten_t atten,
    adc_cali_handle_t *out_handle
)
{
    esp_err_t ret;
    bool calibrated = false;
    adc_cali_handle_t handle = NULL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cfg = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cfg, &handle);
    if (ret == ESP_OK) calibrated = true;
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cfg = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cfg, &handle);
        if (ret == ESP_OK) calibrated = true;
    }
#endif

    *out_handle = handle;

    if (calibrated) {
        ESP_LOGI(TAG, "ADC calibration success");
    } else {
        ESP_LOGW(TAG, "ADC calibration not supported, use raw value");
    }

    return calibrated;
}
