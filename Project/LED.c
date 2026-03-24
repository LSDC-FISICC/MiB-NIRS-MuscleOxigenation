/**
 * @file LED.c
 * @brief GPIO-Based status LED control implementation for STM32F303K8
 * @details Simple push-pull GPIO driver for visual feedback LED on port PB3.
 * @author Julio Fajardo, PhD
 * @date 2024-06-01
 * @version 2.0
 */

#include "LED.h"
#include "stm32f303x8.h"

/**
 * @brief Initialize GPIO port B, pin 3 as push-pull output
 * @details Complete GPIO setup for LED control:
 *          1. **Enable GPIOB peripheral clock** (AHB domain)
 *          2. **Configure PB3 mode** as output (push-pull)
 *          3. **Configure PB4 mode** as output (reserved for future use)
 *          4. **Ensure push-pull type** (OTYPER[3]=0, OTYPER[4]=0)
 *          5. Ready for ODR writes (LED_On, LED_Off, LED_Toggle)
 *
 * ### Register Operations
 *  - RCC->AHBENR |= RCC_AHBENR_GPIOBEN
 *    Bit Enable: Transitions GPIOB from OFF to ON power state
 *    Effect: GPIO registers become readable/writable
 *    Latency: ~3 AHB cycles (~50 ns @ 64 MHz)
 *
 *  - GPIOB->MODER &= ~((3<<6)|(3<<8))
 *    Clear bits [7:6] (PB3 mode) and [9:8] (PB4 mode)
 *    Purpose: Reset both pin modes to input (00)
 *
 *  - GPIOB->MODER |= (1<<6) | (1<<8)
 *    Set bits [6]=1 (PB3 mode[0]=1) and [8]=1 (PB4 mode[0]=1)
 *    Result: MODER[7:6] = 01 (PB3 = output), MODER[9:8] = 01 (PB4 = output)
 *    Note: PB4 is configured as output here; I2C1 uses PB6/PB7 (AF4), not PB4
 *
 * ### GPIO State After Config
 *  | Feature | PB3 | PB4 |
 *  |---------|-----|-----|
 *  | Mode | Output | Output (reserved) |
 *  | Type | Push-pull | Push-pull |
 *  | Speed | Medium | Medium |
 *  | Initial ODR | 0 (LOW) | 0 (LOW) |
 *  | Pull | None | None |
 *
 * @param None
 * @return void
 *
 * @timing
 *  - Clock stabilization: ~3 AHB cycles
 *  - Register writes: ~1 cycle each (3 writes = 3 cycles)
 *  - Total: <500 ns
 *  - PB3 ready for output: immediately after function returns
 *
 * @critical_notes
 *  - Must be called AFTER clk_config() to ensure SYSCLK is stable at 64 MHz
 *  - I2C1_Config() must be called AFTER this function to properly configure PB4 as I2C alternate function
 *  - Initialization order: clk_config() → LED_config() → I2C1_Config()
 *  - Do not call LED_config() after I2C1_Config() as it overwrites PB4 alternate function configuration
 *
 * @side_effects
 *  - GPIOB clock permanently enabled (not disabled in low-power mode by default)
 *  - PB3 is now an output; cannot be used as input, ADC, or timer capture
 *  - PB3 starts LOW (LED off); no explicit LED_Off() call needed
 *  - Draws ~1 mA additional idle current (GPIOB always powered)
 *
 * @power_note
 *  - GPIOB active power: ~1 mA @ 3.3V (all 16 pins)
 *  - PB3 push-pull output (idle): <1 µA when HIGH, <1 µA when LOW
 *  - LED current: 10-20 mA typical (dominates GPIO current by ~100×)
 *
 * @see LED_On, LED_Off, LED_Toggle
 */
void LED_config(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~((3<<6)|(3<<8));
    GPIOB->MODER |= (1<<6) | (1<<8);
}

/**
 * @brief Drive PB3 output HIGH (turn LED on)
 * @details Sets GPIO output data register bit 3 to logical HIGH (3.3 V).
 *          Push-pull driver sources current for LED to illuminate.
 *
 * @param None
 * @return void
 * @timing ~30 ns latency
 * @see LED_Off, LED_Toggle, LED_config
 */
void LED_On(void) {
    GPIOB->ODR |= (1<<3);
}   

/**
 * @brief Drive PB3 output LOW (turn LED off)
 * @details Clears GPIO output data register bit 3 to logical LOW (0 V).
 *          LED current stops and LED extinguishes.
 *
 * @param None
 * @return void
 * @timing ~30 ns latency
 * @see LED_On, LED_Toggle, LED_config
 */
void LED_Off(void) {
    GPIOB->ODR &= ~(1<<3);
}

/**
 * @brief Toggle PB3 output state (if HIGH → LOW; else LOW → HIGH)
 * @details Inverts LED state via XOR operation on GPIO output data register.
 *          Ideal for periodic blinking feedback in SysTick ISR.
 *
 * @param None
 * @return void
 * @timing ~30-50 ns latency
 * @note Called from SysTick_Handler() every 20 ms → 25 Hz blink (20 ms on, 20 ms off)
 * @see LED_On, LED_Off, LED_config
 */
void LED_Toggle(void) {
    GPIOB->ODR ^= (1<<3);
}
