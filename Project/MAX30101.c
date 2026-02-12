#include "MAX30101.h"
#include "I2C.h"
#include <stdint.h>

/**
 * @brief Initializes MAX30101 for SpO2 (blood oxygen) measurement - Standard mode
 * @param None
 * @return None
 * @note Uses 2 LEDs (Red + IR), 400 Hz sample rate, 18-bit resolution
 * @note High accuracy, higher power consumption
 */
void MAX30101_Init(void){
    I2C1_Write(SENSOR_ADDR, FIFO_CONFIG, 0x10);      // FIFO avg 1, FIFO rollover
    I2C1_Write(SENSOR_ADDR, MODE_CONFIG, 0x03);      // SPO2 Mode (Red + IR)
    I2C1_Write(SENSOR_ADDR, SPO2_CONFIG, 0x2F);      // 4096 range, sample rate 400 Hz, pulse width 411 (18 bits)
    I2C1_Write(SENSOR_ADDR, FIFO_READPTR, 0x0);      // Reset FIFO read pointer
    I2C1_Write(SENSOR_ADDR, FIFO_WRITPTR, 0x0);      // Reset FIFO write pointer
    I2C1_Write(SENSOR_ADDR, LED1_PAMPLI, 0x3F);      // Red LED power (max)
    I2C1_Write(SENSOR_ADDR, LED2_PAMPLI, 0x3F);      // IR LED power (max)
}

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
