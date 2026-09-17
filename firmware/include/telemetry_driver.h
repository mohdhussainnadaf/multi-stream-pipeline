#ifndef TELEMETRY_DRIVER_H
#define TELEMETRY_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t timestamp_ms;
    uint16_t adc_value;
    float temperature_c;
    bool dma_complete;
} TelemetryFrame;

void hardware_timer_init(uint32_t frequency_hz);
void dma_channel_config(uint8_t channel);
TelemetryFrame firmware_acquire_sensor_data(void);

#endif
