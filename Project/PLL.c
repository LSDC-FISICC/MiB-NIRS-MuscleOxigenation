#include "PLL.h"
#include "stm32f303x8.h"

/**
 * @brief Configure system clock to 64 MHz using PLL
 * @details Complete PLL and RCC configuration sequence:
 *          1. Set PLLMUL to 0x0E (×16) for HSI input
 *          2. Configure Flash for 64 MHz operation (2 wait states)
 *          3. Enable PLL oscillator and wait for lock
 *          4. Set APB1 prescaler to 2 (32 MHz)
 *          5. Switch SYSCLK source from HSI to PLL
 *          6. Update SystemCoreClock variable
 *
 * ### Clock Calculation (STM32F303K8)
 *  - Input: HSI = 8 MHz
 *  - PLL Multiplier: 16 (PLLMUL = 0x0E)
 *  - SYSCLK = (8 MHz / 2) × 16 = 64 MHz
 *  - AHB Clock: 64 MHz (no prescaler)
 *  - APB1 Clock: 32 MHz (prescaler = 2)
 *  - APB2 Clock: 64 MHz (no prescaler)
 *
 * ### Register Operations
 *  - RCC_CFGR[21:18] = 0x0E (PLLMUL = 16)
 *  - FLASH_ACR[2:0] = 0x2 (Latency = 2 cycles for 64 MHz)
 *  - RCC_CR[24] = 1 (Enable PLL)
 *  - RCC_CFGR[1:0] = 0x2 (SW = PLL)
 *  - RCC_CFGR[10:8] = 0x4 (PPRE1 = divide by 2)
 *
 * @param None
 * @return void
 */
void clk_config(void) {
    // Set PLLMUL to 16: (8 MHz / 2) × 16 = 64 MHz system clock
    RCC->CFGR |= 0xE << 18;
    // Configure Flash for 2 wait states (required for 64 MHz operation)
    FLASH->ACR |= 0x2;
    // Enable PLL
    RCC->CR |= RCC_CR_PLLON;
    // Wait for PLL to lock
    while (!(RCC->CR & RCC_CR_PLLRDY));
    // Switch SYSCLK to PLL, set APB1 prescaler to 2 (32 MHz)
    RCC->CFGR |= 0x402;
    // Wait for system clock to switch to PLL
    while (!(RCC->CFGR & RCC_CFGR_SWS_PLL));
    // Update SystemCoreClock global variable
    SystemCoreClockUpdate();
}
