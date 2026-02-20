#include "MAX30101.h"
#include "I2C.h"
#include <stdint.h>

/**
 * @brief Initializes MAX30101 for SpO2 measurement - Low power mode
 * @param None
 * @return None
 * @note Uses 2 LEDs (Red + IR), 50 Hz sample rate, 16-bit resolution
 * @note Lower power consumption, suitable for wearables and battery-powered devices
 */
void MAX30101_InitSPO2Lite(void){
    I2C1_Write(SENSOR_ADDR, FIFO_CONFIG, 0x4F);      // FIFO avg 8, FIFO rollover enabled
    I2C1_Write(SENSOR_ADDR, MODE_CONFIG, 0x03);      // SPO2 Mode (Red + IR)
    I2C1_Write(SENSOR_ADDR, SPO2_CONFIG, 0x23);      // 2048 range, sample rate 50 Hz, pulse width 215 (16 bits)
    I2C1_Write(SENSOR_ADDR, FIFO_READPTR, 0x0);      // Reset FIFO read pointer
    I2C1_Write(SENSOR_ADDR, FIFO_WRITPTR, 0x0);      // Reset FIFO write pointer
    I2C1_Write(SENSOR_ADDR, LED1_PAMPLI, 0x18);      // Red LED power (low)
    I2C1_Write(SENSOR_ADDR, LED2_PAMPLI, 0x18);      // IR LED power (low)
    I2C1_Write(SENSOR_ADDR, DIE_TEMPCFG, 0x01);      // Enable temperature sensor
}

/**
 * @brief Initializes MAX30101 for muscle oxygenation (NIRS) measurement
 * @param None
 * @return None
 * @note Uses 3 LEDs (Red + IR + Green), 100 Hz sample rate, lower power consumption
 * @note Better for tissue penetration in muscle applications
 */
void MAX30101_InitMuscleOx(uint8_t ledPower){
    I2C1_Write(SENSOR_ADDR, FIFO_CONFIG, 0x4F);         // FIFO avg 8, FIFO rollover enabled
    I2C1_Write(SENSOR_ADDR, MODE_CONFIG, 0x07);         // Multi-LED mode (Red + IR + Green)
    I2C1_Write(SENSOR_ADDR, SPO2_CONFIG, 0x26);         // 2048 range, sample rate 100 Hz, pulse width 215 (16 bits)
    I2C1_Write(SENSOR_ADDR, FIFO_READPTR, 0x0);         // Reset FIFO read pointer
    I2C1_Write(SENSOR_ADDR, FIFO_WRITPTR, 0x0);         // Reset FIFO write pointer 
    I2C1_Write(SENSOR_ADDR, LED1_PAMPLI, ledPower);     // Red LED power (medium)
    I2C1_Write(SENSOR_ADDR, LED2_PAMPLI, ledPower);     // IR LED power (medium)
    I2C1_Write(SENSOR_ADDR, LED3_PAMPLI, ledPower);     // Green LED power (medium)
    I2C1_Write(SENSOR_ADDR, DIE_TEMPCFG, 0x01);         // Enable temperature sensor
}

/**
 * @brief Returns the number of available samples in the MAX30101 FIFO
 * @details Reads FIFO_WR_PTR and FIFO_RD_PTR registers to calculate available samples
 * @param None
 * @return uint8_t - Number of available samples in the FIFO (0-32)
 * @note FIFO has 32 sample slots. If write pointer >= read pointer: samples = write - read.
 *       If write pointer < read pointer: samples = (32 - read) + write (wrap-around case)
 */
uint8_t MAX30101_GetNumAvailableSamples(void){
    uint8_t write_ptr = 0;
    uint8_t read_ptr = 0;
    uint8_t num_samples = 0;
    
    // Read FIFO write pointer and read pointer from sensor
    I2C1_Read(SENSOR_ADDR, FIFO_WRITPTR, &write_ptr, 1);
    I2C1_Read(SENSOR_ADDR, FIFO_READPTR, &read_ptr, 1);
    
    // Mask to 5 bits since FIFO pointers are 5 bits (0-31)
    write_ptr &= 0x1F;
    read_ptr &= 0x1F;
    
    // Calculate available samples
    if (write_ptr >= read_ptr) {
        num_samples = write_ptr - read_ptr;
    } else {
        num_samples = (32 - read_ptr) + write_ptr;
    }
    
    return num_samples;
}

/**
 * @brief Reads multiple samples from the MAX30101 FIFO
 * @details Reads complete samples from the FIFO. Each sample consists of 6 bytes
 *          (2 bytes per channel × 3 channels for multi-LED mode).
 *          Data is read sequentially from FIFO_DATAREG (0x07).
 * @param samples - Pointer to array of MAX30101_Sample structures
 * @param num_samples - Number of complete samples to read
 * @return None
 * @note In multi-LED mode, each complete sample requires 6 bytes from the FIFO
 */
void MAX30101_ReadFIFO(MAX30101_Sample *samples, uint8_t num_samples){
    uint8_t i = 0;
    uint8_t fifo_data[8];  // 6 bytes per complete sample (3 channels × 2 bytes)
    
    for (i = 0; i < num_samples; i++) {
        // Read 6 consecutive bytes from FIFO_DATAREG
        // Each I2C read from address 0x07 gets the next byte in the FIFO
        I2C1_Read(SENSOR_ADDR, FIFO_DATAREG, fifo_data, 6);
        
        // Store the 2-byte samples for each channel
        samples[i].red[0] = fifo_data[0];
        samples[i].red[1] = fifo_data[1];
        
        samples[i].ir[0] = fifo_data[2];
        samples[i].ir[1] = fifo_data[3];
        
        samples[i].green[0] = fifo_data[4];
        samples[i].green[1] = fifo_data[5];
    }
}

/**
 * @brief Converts 2-byte sample data to 16-bit uint16_t format
 * @details Combines 2 bytes (MSB, LSB) into a single 16-bit value in uint16_t
 *          Formula: (MSB << 8) | LSB
 * @param sample_in - Pointer to MAX30101_Sample with 2-byte values
 * @param sample_out - Pointer to MAX30101_SampleData to store converted values
 * @return None
 */
void MAX30101_ConvertSampleToUint16(MAX30101_Sample *sample_in, MAX30101_SampleData *sample_out){
    // Convert Red LED: combine 2 bytes into 16-bit uint16_t
    sample_out->red = ((uint16_t)sample_in->red[0] << 8) | 
                      ((uint16_t)sample_in->red[1]);
    
    // Convert IR LED: combine 2 bytes into 16-bit uint16_t
    sample_out->ir = ((uint16_t)sample_in->ir[0] << 8) | 
                     ((uint16_t)sample_in->ir[1]);
    
    // Convert Green LED: combine 2 bytes into 16-bit uint16_t
    sample_out->green = ((uint16_t)sample_in->green[0] << 8) | 
                        ((uint16_t)sample_in->green[1]);
}

/**
 * @brief Converts uint16_t ADC counts to current in nanoamps (nA)
 * @details Converts 16-bit ADC values to current using the LSB size (7.81 pA)
 *          Formula: Current (nA) = ADC_Count × LSB_size (nA)
 *          Current (nA) = ADC_Count × 0.00781 nA
 * @param sample_in - Pointer to MAX30101_SampleData with uint16_t ADC counts
 * @param sample_out - Pointer to MAX30101_SampleCurrent to store current values in nA
 * @return None
 * @note Uses MAX30101_CURRENT_LSB_NA (0.00781 nA) for conversion
 *       Full scale range: 2048 nA
 */
void MAX30101_ConvertUint16ToCurrent(MAX30101_SampleData *sample_in, MAX30101_SampleCurrent *sample_out){
    // Convert Red channel ADC count to current in nanoamps
    sample_out->red = (float32_t)sample_in->red * MAX30101_CURRENT_LSB_NA;
    
    // Convert IR channel ADC count to current in nanoamps
    sample_out->ir = (float32_t)sample_in->ir * MAX30101_CURRENT_LSB_NA;
    
    // Convert Green channel ADC count to current in nanoamps
    sample_out->green = (float32_t)sample_in->green * MAX30101_CURRENT_LSB_NA;
}

/**
 * @brief Read FIFO and convert directly to current (nA)
 * @details This helper combines ReadFIFO, conversion to uint16, and scaling to nA
 *          in a single loop to minimize intermediate storage and processing.
 * @param samples - Pointer to array of MAX30101_SampleCurrent to fill
 * @param num_samples - Number of complete samples to read
 * @return None
 * @note Each sample still uses 6 bytes (2 bytes per channel). Conversion
 *       performed on-the-fly without creating SampleData structures.
 */
void MAX30101_ReadFIFO_Current(MAX30101_SampleCurrent *samples, uint8_t num_samples){
    uint8_t fifo_data[6];
    uint8_t i;
    uint16_t temp;
    
    for(i = 0; i < num_samples; i++){
        // read 6 bytes from FIFO
        I2C1_Read(SENSOR_ADDR, FIFO_DATAREG, fifo_data, 6);
        // combine bytes and convert right away
        temp = ((uint16_t)fifo_data[0] << 8) | fifo_data[1];
        samples[i].red = (float32_t)temp * MAX30101_CURRENT_LSB_NA;
        
        temp = ((uint16_t)fifo_data[2] << 8) | fifo_data[3];
        samples[i].ir = (float32_t)temp * MAX30101_CURRENT_LSB_NA;
        
        temp = ((uint16_t)fifo_data[4] << 8) | fifo_data[5];
        samples[i].green = (float32_t)temp * MAX30101_CURRENT_LSB_NA;
    }
}
