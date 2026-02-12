/**
 * @file main.c
 * @brief Main program for STM32F303: LED control, SysTick, and I2C
 * @details Initializes the system clock, configures GPIO for the LED on PB3,
 *          sets up the SysTick timer, and provides a basic configuration
 *          function for I2C1. Intended as an example and starting point.
 * @author Julio Fajardo
 * @date 2026-02-12
 */

#include "stm32f303x8.h"
#include <stdint.h>

#include "LED.h"
#include "I2C.h"
#include "MAX30101.h"

uint32_t counter = 0;
uint32_t ticks = 0;

// Function prototypes
void clk_config(void);

/** 
    * @brief Main function, initializes the GPIO and SysTick timer
    * @param None
    * @return None
*/
int main() {

    // Configure the system clock to 64 MHz
    clk_config();
    // Configure the GPIO pin for the LED on PB3
    LED_config();
    // Configure I2C1 for communication with the MAX30101 sensor
    I2C1_Config();
    // Initialize MAX30101 for muscle oxygenation with medium LED power
    MAX30101_InitMuscleOx(0x4B); 
    // Configure SysTick to generate an interrupt every 100 ms
    SysTick_Config(SystemCoreClock / 10);

    for (;;) {
        counter++;
    }
}

/** 
    * @brief SysTick interrupt handler, toggles the LED on PB3 every 100ms
    * @param None
    * @return None
*/  

void SysTick_Handler(void) {
    ticks++;
    LED_Toggle();
}

/** 
    * @brief Configures the system clock to 64 MHz using PLL with HSI as the source
    * @param None
    * @return None
*/
void clk_config(void){
	// PLLMUL <- 0x0E (PLL input clock x 16 --> (8 MHz / 2) * 16 = 64 MHz )  
	RCC->CFGR |= 0xE<<18;
	// Flash Latency, two wait states for 48<HCLK<=72 MHz
	FLASH->ACR |= 0x2;
	// PLLON <- 0x1 
    RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));	
	// SW<-0x02 (PLL as System Clock), HCLK not divided, PPRE1<-0x4 (APB1 <- HCLK/2), APB2 not divided 
	RCC->CFGR |= 0x402;
	while (!(RCC->CFGR & RCC_CFGR_SWS_PLL));
	SystemCoreClockUpdate();	
}