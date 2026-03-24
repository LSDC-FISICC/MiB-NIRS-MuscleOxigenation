/**
 * @file MAX30101.h
 * @brief MAX30101 Optical Biosensor Driver for NIRS Muscle Oxygenation
 * @details Complete driver for Maxim Integrated MAX30101 optical sensor supporting:
 *          - Multi-LED mode (Red, IR, Green) for NIRS measurements
 *          - Dual-LED mode (Red, IR) for SpO2 measurements
 *          - Direct current readout in nanoamps (nA) with 31.25 pA resolution (16-bit ADC)
 *          - 16-bit ADC with 2048 nA full-scale range
 *          - 32-sample FIFO with wrap-around support
 * @author Julio Fajardo, PhD
 * @date 2024-06-01
 * @version 2.0
 */

#ifndef MAX30101_H_
#define MAX30101_H_

#include <stdint.h>
#include "arm_math_types.h"

#define		SENSOR_ADDR 		0xAE

#define		INTR_STATUS1		0x00
#define		INTR_STATUS2		0x01
#define		INTR_ENABLE1		0x02
#define		INTR_ENABLE2		0x03
#define 	FIFO_WRITPTR 	    0x04
#define 	OVRF_COUNTER  	    0x05
#define 	FIFO_READPTR  	    0x06
#define 	FIFO_DATAREG  	    0x07
#define 	FIFO_CONFIG 	 	0x08
#define     MODE_CONFIG			0x09
#define     SPO2_CONFIG			0x0A
#define     PULSEWIDTH_CONFIG	0x0B
#define     LED1_PAMPLI			0x0C
#define     LED2_PAMPLI			0x0D
#define     LED3_PAMPLI			0x0E
#define     LED4_PAMPLI			0x0F
#define     MLED_CONFG1			0x11
#define     MLED_CONFG2			0x12
#define     DIE_TEMPINT			0x1F
#define     DIE_TEMPFRC			0x20
#define     DIE_TEMPCFG			0x21

#define     BUFFERBLOCKSIZE     0x8
#define     MAX30101_ADC_VREF   3.3f        /**< ADC reference voltage in volts */
#define     MAX30101_ADC_BITS   16          /**< ADC resolution in bits */
#define     MAX30101_ADC_MAX    ((1 << MAX30101_ADC_BITS) - 1)  /**< Max ADC count (65535 for 16-bit) */
#define     MAX30101_CURRENT_LSB_PA  31.25f  /**< LSB size in picoamps (pA) */
#define     MAX30101_CURRENT_LSB_NA  (MAX30101_CURRENT_LSB_PA / 1000.0f)  /**< LSB size in nanoamps (nA) */
#define     MAX30101_CURRENT_FULLSCALE  2048.0f  /**< Full scale current range in nanoamps (nA) */

/**
 * @struct MAX30101_Sample
 * @brief Raw FIFO sample data for NIRS mode (4 bytes)
 * @details Dual-LED mode uses Red + IR channels.
 *          Format: 2 channels × 2 bytes (MSB, LSB)
 *          Total: 4 bytes per sample.
 * @note Use MAX30101_ReadFIFO() with NIRS mode or a dedicated NIRS read function
 */
typedef struct {
    uint8_t red[2];      /**< Red LED raw bytes (MSB, LSB) */
    uint8_t ir[2];       /**< IR LED raw bytes (MSB, LSB) */
} MAX30101_Sample;

/**
 * @struct MAX30101_SampleData
 * @brief 16-bit ADC counts for NIRS mode
 * @details Dual-LED data format after byte packing.
 * @note Use MAX30101_ConvertSampleToUint16() with NIRS struct or a NIRS conversion routine
 */
typedef struct {
    uint16_t red;        /**< Red ADC 16-bit value */
    uint16_t ir;         /**< IR ADC 16-bit value */
} MAX30101_DataSample;

/**
 * @struct MAX30101_SampleCurrent
 * @brief Calibrated photodiode current in nA for NIRS mode
 * @details Same 7.81 pA LSB scaling as multi-LED mode.
 */
typedef struct {
    float32_t red;       /**< Red current (0–2048 nA) */
    float32_t ir;        /**< IR current (0–2048 nA) */
} MAX30101_CurrentSample;

/**
 * @brief Initialize MAX30101 for NIRS muscle oxygenation (dual-LED: Red + IR)
 * @details Configures sensor for blood oxygen measurement with low power consumption.
 *          Sample rate: 50 Hz, FIFO rollover enabled, configurable LED power.
 * @param ledPower_red - Red LED current value (0.0 to 51 mA range)
 * @param ledPower_ir - IR LED current value (0.0 to 51 mA range)
 * @note Call once at startup before MAX30101_ReadSingleCurrent()
 * @see MAX30101_InitNIRSLite, MAX30101_ReadSingleCurrent
 * @example
 *   MAX30101_InitNIRSLite(1.6f, 1.6f);  // 1.6 mA per LED for low power operation
 */
void MAX30101_InitNIRSLite(float32_t ledPower_red, float32_t ledPower_ir);

/**
 * @brief Get number of available samples in FIFO
 * @return Number of unread samples (0-32)
 */
uint8_t MAX30101_GetNumAvailableSamples(void);

/**
 * @brief Update FIFO read pointer
 * @param num_samples Number of samples to advance read pointer
 */
void MAX30101_UpdateReadPointer(uint8_t num_samples);
/**
 * @brief Convert raw NIRS sample bytes to 16-bit ADC counts
 * @param sample_in Pointer to MAX30101_Sample with raw byte data
 * @param sample_out Pointer to MAX30101_DataSample for output counts
 */
void MAX30101_ConvertSampleToUint16(MAX30101_Sample *sample_in, MAX30101_DataSample *sample_out);

/**
 * @brief Convert NIRS ADC counts to current in nanoamps (0-2048 nA)
 * @param sample_in - [in] MAX30101_DataSample with ADC counts
 * @param sample_out - [out] MAX30101_CurrentSample with nA values
 */
void MAX30101_ConvertUint16ToCurrent(MAX30101_DataSample *sample_in, MAX30101_CurrentSample *sample_out);

/**
 * @brief Read single NIRS sample from FIFO as 16-bit ADC counts
 * @details Optimized single-sample read returning raw ADC values (0-65535)
 * @param sample - [out] MAX30101_DataSample with 16-bit ADC counts
 * @see MAX30101_GetNumAvailableSamples to check for available data
 */
void MAX30101_ReadSingleData(MAX30101_DataSample *sample);

/**
 * @brief Read single NIRS sample from FIFO with current conversion
 * @details Optimized single-sample read converting 4 bytes directly to nanoamps
 * @param sample - [out] MAX30101_CurrentSample (Red, IR nA values)
 * @see MAX30101_GetNumAvailableSamples to check for available data
 */
void MAX30101_ReadSingleCurrentData(MAX30101_CurrentSample *sample);

#endif /* MAX30101_H_ */  
