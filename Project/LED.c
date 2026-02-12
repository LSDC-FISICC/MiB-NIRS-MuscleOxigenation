#include "LED.h"
#include "stm32f303x8.h"

/** 
    * @brief Configures the GPIO PB3 pin for the LED
    * @param None
    * @return None
*/
void LED_config(void) {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    GPIOB->MODER &= ~((3<<6)|(3<<8));
    GPIOB->MODER |= (1<<6) | (1<<8);
}

/** 
    * @brief Turns on the LED connected to PB3
    * @param None
    * @return None
*/
void LED_On(void) {
    GPIOB->ODR |= (1<<3);
}   

/** 
    * @brief Turns off the LED connected to PB3
    * @param None
    * @return None
*/
void LED_Off(void) {
    GPIOB->ODR &= ~(1<<3);
}

/**
    * @brief Toggles the state of the LED connected to PB3
    * @param None
    * @return None
*/
void LED_Toggle(void) {
    GPIOB->ODR ^= (1<<3);
}
