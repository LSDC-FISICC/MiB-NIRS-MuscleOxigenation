/**
 * @file UART.c
 * @brief USART2 driver implementation for MAX30101 data transmission
 * @details Configures USART2 (PA2=TX, PA15=RX) at variable baud rate
 *          with blocking transmission support.
 * @author Julio Fajardo, PhD
 * @date 2024-06-01
 * @version 2.0
 */

#include "UART.h"
#include "stm32f303x8.h"
#include "system_stm32f3xx.h"
#include <stdint.h>

/**
 * @brief Initialize USART2 for configurable baud rate transmission
 * @details Complete USART2 setup sequence:
 *          1. Enable GPIOA and USART2 clocks
 *          2. Configure PA2 (TX) and PA15 (RX) as AF7 (Alternate Function 7)
 *          3. Configure BRR for desired baud rate
 *          4. Enable transmitter, receiver, and receiver interrupt
 *
 * @param baud_rate - Desired baud rate (e.g., 460800 as used in this project)
 * @return void
 *
 * @timing
 *  - Total init time: <1 ms
 *  - USART2 ready immediately after function returns
 *
 * @warning Call once at system startup, BEFORE any USART2_Send operations
 * @see USART2_Send, USART2_putString
 */
void UART_Config(uint32_t baud_rate) {
    // Enable GPIOA clock (for PA2 and PA15)
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    // Enable USART2 clock (APB1)
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    
    // Configure PA2 and PA15 as alternate function pins
    GPIOA->MODER |= (0x02 << 4) | (0x2 << 30);
    // Set PA2 alternate function to AF7 (USART2_TX)
    GPIOA->AFR[0] |= (0x07 << 8);
    // Set PA15 alternate function to AF7 (USART2_RX)
    GPIOA->AFR[1] |= (0x07 << 28);
    
    // Configure baud rate: BRR = (APB1_Clock / baud_rate)
    // APB1 = 32 MHz (SYSCLK/2), so BRR = 32MHz / baud_rate
    USART2->BRR = (uint32_t)((SystemCoreClock / 2) / baud_rate);
    // Enable transmitter and receiver
    USART2->CR1 |= USART_CR1_RE | USART_CR1_TE;
    // Enable USART2
    USART2->CR1 |= USART_CR1_UE;
    // Enable receiver interrupt for future use
    USART2->CR1 |= USART_CR1_RXNEIE;
}

/**
 * @brief Send single character via USART2
 * @details Blocks until previous transmission complete, then sends one byte
 *
 * @param c - Character byte to transmit
 * @return void
 *
 * @timing
 *  - Per-byte latency: ~22 µs at 460800 baud (10 bits/byte: 8N1)
 *
 * @note Blocking function; waits for TC (transmission complete) flag
 * @see UART_Config, USART2_putString
 */
void USART2_Send(uint8_t c) {
    // Wait for previous transmission to complete (TC flag)
    while (!(USART2->ISR & USART_ISR_TC));
    // Load character into transmit data register
    USART2->TDR = c;
}

/**
 * @brief Send null-terminated string via USART2
 * @details Transmits string character-by-character using USART2_Send()
 *
 * @param string - Pointer to null-terminated character string
 * @return void
 *
 * @timing
 *  - Per-character latency: ~22 µs at 460800 baud (10 bits/byte: 8N1)
 *  - Example: 10-character string ≈ 220 µs total; typical CSV frame ~350 µs
 *
 * @data_format String must be null-terminated (\\0)
 * @note Blocking function; waits for each character's transmission
 * @see UART_Config, USART2_Send
 */
void USART2_putString(char *string) {
    // Transmit each character until null terminator
    while (*string) {
        USART2_Send(*string);
        string++;
    }
}