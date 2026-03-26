/**
 * @file MAX30101.c
 * @brief MAX30101 optical biosensor driver implementation for NIRS measurements
 * @details Complete driver for Maxim Integrated MAX30101 optical sensor supporting
 *          multi-LED and SpO2 measurement modes with I2C communication interface.
 * @author Julio Fajardo, PhD
 * @date 2024-06-01
 * @version 2.0
 */

#include "MAX30101.h"
#include "I2C.h"
#include "arm_math_types.h"
#include <stdint.h>

/**
 * @brief Initialize MAX30101 in SpO2 mode (dual-LED: Red + IR)
 * @details Configures sensor for blood oxygen (SpO2) measurement with low power consumption.
 *          - Mode: SpO2 (Red + IR LEDs)
 *          - Sample Rate: 50 Hz (SPO2_CONFIG = 0x01, bits [4:2] = 000)
 *          - ADC Resolution: 18-bit, 411 µs pulse width (SPO2_CONFIG bits [1:0] = 11)
 *          - ADC Range: 4096 nA full-scale (SPO2_CONFIG bits [6:5] = 01)
 *          - FIFO Configuration: No averaging, rollover enabled
 * @param ledPower_red - Red LED drive current in milliamps (0.0 to 51.0 mA, ~0.2 mA steps)
 *                       Converted to register value as: reg = (uint8_t)(mA / 0.2)
 *                       Example: 1.6 mA → reg 0x08; 10 mA → reg 0x32
 * @param ledPower_ir  - IR LED drive current in milliamps (same range and conversion)
 * @return void
 * @note Suitable for battery-powered wearable applications.
 *       Call once during initialization before reading samples.
 * @see MAX30101_InitMuscleOx
 * @example
 *   MAX30101_InitSPO2Lite(1.6f, 1.6f);  // 1.6 mA per LED, low power
 *   uint8_t samples = MAX30101_GetNumAvailableSamples();
 */
void MAX30101_InitNIRSLite(float32_t ledPower_red, float32_t ledPower_ir) {
    // Configure FIFO: no averaging, rollover enabled
    I2C1_Write(SENSOR_ADDR, FIFO_CONFIG, 0x10);
    // Select SpO2 mode (Red + IR)
    I2C1_Write(SENSOR_ADDR, MODE_CONFIG, 0x03);
    // SpO2 config: 4096 nA range, 50 Hz sample rate, 411 µs pulse width
    I2C1_Write(SENSOR_ADDR, SPO2_CONFIG, 0x23);
    // Reset FIFO read pointer
    I2C1_Write(SENSOR_ADDR, FIFO_READPTR, 0x0);
    // Reset FIFO write pointer
    I2C1_Write(SENSOR_ADDR, FIFO_WRITPTR, 0x0);
    // Set Red LED power
    I2C1_Write(SENSOR_ADDR, LED1_PAMPLI, (uint8_t)(ledPower_red / 0.2f));  // Convert mA to register value (0.2 mA steps)
    // Set IR LED power
    I2C1_Write(SENSOR_ADDR, LED2_PAMPLI, (uint8_t)(ledPower_ir / 0.2f));  // Same LED power for IR
}

/**
 * @brief Query FIFO status from MAX30101 sensor
 * @details Reads FIFO write and read pointer registers to determine number of unread samples.
 *          Accounts for circular 32-sample FIFO with pointer wrap-around.
 * @param None
 * @return uint8_t Number of complete samples available (0 to 32)
 *         - Returns 0 if FIFO empty or pointers equal
 *         - Returns 1 to 32 for available samples
 * @note Call before MAX30101_ReadSingleCurrent()
 *       to confirm new data is ready.
 * @warning Multiple fast consecutive reads may show inconsistent results
 *          due to FIFO updates during pointer reads.
 * @see MAX30101_ReadSingleCurrent, MAX30101_UpdateReadPointer
 * @example
 *   uint8_t count = MAX30101_GetNumAvailableSamples();
 *   if (count > 0) {
 *       MAX30101_ReadSingleCurrent(&sample);
 *       MAX30101_UpdateReadPointer(count);  
 *   }
 */
uint8_t MAX30101_GetNumAvailableSamples(void) {
    uint8_t write_ptr = 0;
    uint8_t read_ptr = 0;
    uint8_t num_samples = 0;
    
    // Read FIFO write and read pointers from sensor
    I2C1_Read(SENSOR_ADDR, FIFO_WRITPTR, &write_ptr, 1);
    I2C1_Read(SENSOR_ADDR, FIFO_READPTR, &read_ptr, 1);
    
    // Mask to 5 bits (FIFO pointers are 5-bit: 0-31)
    write_ptr &= 0x1F;
    read_ptr &= 0x1F;
    
    // Calculate number of available samples (handles wrap-around)
    if (write_ptr >= read_ptr) {
        num_samples = write_ptr - read_ptr;
    } else {
        num_samples = (32 - read_ptr) + write_ptr;
    }
    
    return num_samples;
}

/**
 * @brief Update the FIFO read pointer
 * @details Advances the read pointer by a specified number of samples, wrapping around at 32.
 * @param num_samples - [in] Number of samples to advance the read pointer
 * @return void
 */
void MAX30101_UpdateReadPointer(uint8_t num_samples) {
    uint8_t read_ptr = 0;
    // Read current FIFO read pointer
    I2C1_Read(SENSOR_ADDR, FIFO_READPTR, &read_ptr, 1);
    // Advance pointer by num_samples with wrap-around at 32
    read_ptr = (read_ptr + num_samples) % 32;
    // Write updated pointer back to sensor
    I2C1_Write(SENSOR_ADDR, FIFO_READPTR, read_ptr);
}

/**
 * @brief Convert raw NIRS sample bytes to 32-bit ADC counts
 * @details Combines 3-byte groups (MSB, LSB, unused) into 32-bit values per channe from 18-bit ADC output.
 *
 * @param sample_in - [in] Pointer to MAX30101_Sample with raw data
 * @param sample_out - [out] Pointer to MAX30101_DataSample for counts (0-4294967295)
 * @return void
 * @see MAX30101_ConvertUint32ToCurrent
 */
void MAX30101_ConvertSampleToUint32(MAX30101_Sample *sample_in, MAX30101_DataSample *sample_out) {
    // Convert Red LED bytes to 32-bit ADC count
    sample_out->red = ((uint32_t)(sample_in->red[0] & 0x3) << 16) | ((uint32_t)sample_in->red[1] << 8) | ((uint32_t)sample_in->red[2]);
    // Convert IR LED bytes to 32-bit ADC count
    sample_out->ir = ((uint32_t)(sample_in->ir[0] & 0x3) << 16) | ((uint32_t)sample_in->ir[1] << 8) | ((uint32_t)sample_in->ir[2]);
}

/**
 * @brief Convert 32-bit NIRS ADC samples to calibrated current in nanoamps
 * @details Scales ADC counts using 31.25 pA LSB to nA range (0-4096 nA).
 *          Current (nA) = ADC_Count × 0.03125 nA  (32-bit ADC, 4096 nA full-scale)
 *
 * @param sample_in - [in] Pointer to MAX30101_DataSample with ADC counts
 * @param sample_out - [out] Pointer to MAX30101_CurrentSample for current (nA)
 * @return void
 * @see MAX30101_ReadFIFO_CurrentSpO2
 */
void MAX30101_ConvertUint32ToCurrent(MAX30101_DataSample *sample_in, MAX30101_CurrentSample *sample_out) {
    // Scale Red LED ADC count to current (nanoamps)
    sample_out->red = (float32_t)sample_in->red * MAX30101_CURRENT_LSB_NA;
    // Scale IR LED ADC count to current (nanoamps)
    sample_out->ir = (float32_t)sample_in->ir * MAX30101_CURRENT_LSB_NA;
}

/**
 * @brief Read single NIRS sample from MAX30101 FIFO as 18-bit ADC counts
 * @details Optimized function for reading one sample at a time.
 *          Reads 6 bytes from FIFO and returns as raw 18-bit ADC values (0-262143).
 *
 * @param sample - [out] Pointer to MAX30101_DataSample for result
 * @return void
 * @see MAX30101_GetNumAvailableSamples
 */
void MAX30101_ReadSingleData(MAX30101_DataSample *sample) {
    uint8_t fifo_data[6];

    // Read 6 bytes from FIFO data register
    I2C1_Read(SENSOR_ADDR, FIFO_DATAREG, fifo_data, 6);

    // Convert Red LED: combine bytes to 32-bit unsigned value
    sample->red = ((uint32_t)(fifo_data[0] & 0x3) << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    
    // Convert IR LED: combine bytes to 32-bit unsigned value
    sample->ir = ((uint32_t)(fifo_data[3] & 0x3) << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
}

/**
 * @brief Read single NIRS sample from MAX30101 FIFO with current conversion
 * @details Optimized function for reading one sample at a time.
 *          Reads 6 bytes from FIFO, extracts 18-bit ADC counts, and converts to nA.
 *
 * @param sample - [out] Pointer to MAX30101_CurrentSample for result
 * @return void
 * @see MAX30101_GetNumAvailableSamples
 */
void MAX30101_ReadSingleCurrentData(MAX30101_CurrentSample *sample) {
    uint8_t fifo_data[6];
    uint32_t temp;

    // Read 6 bytes from FIFO data register
    I2C1_Read(SENSOR_ADDR, FIFO_DATAREG, fifo_data, 6);

    // Convert Red LED: extract 18-bit ADC value and scale to nanoamps
    temp = ((uint32_t)(fifo_data[0] & 0x3) << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    sample->red = (float32_t)temp * MAX30101_CURRENT_LSB_NA;

    // Convert IR LED: extract 18-bit ADC value and scale to nanoamps
    temp = ((uint32_t)(fifo_data[3] & 0x3) << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    sample->ir = (float32_t)temp * MAX30101_CURRENT_LSB_NA;
}

