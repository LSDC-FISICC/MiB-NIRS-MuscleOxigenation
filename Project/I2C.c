#include "I2C.h"
#include "stm32f303x8.h"

/**
    * @brief Configures I2C1 on pins PB6 (SCL) and PB7 (SDA) for 400 kHz operation
    * @param None
    * @return None
*/
void I2C1_Config(void) {
    // Enable I2C1 clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    // Enable GPIOB clock
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    // Configure PB6 (SCL) and PB7 (SDA) as open-drain alternate function
    // Clear mode bits for PB6 and PB7
    GPIOB->MODER &= ~((3 << 12) | (3 << 14));
    // Set to alternate function (10)
    GPIOB->MODER |= (2 << 12) | (2 << 14);
    // Set open-drain output type
    GPIOB->OTYPER |= (1 << 6) | (1 << 7);
    // Set alternate function to I2C1 (AF4 for PB6 and PB7)
    GPIOB->AFR[0] &= ~((0xF << 24) | (0xF << 28));
    GPIOB->AFR[0] |= (4 << 24) | (4 << 28);
    // Reset I2C1
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
    // Disable I2C1 to configure it
    I2C1->CR1 &= ~I2C_CR1_PE;
    // TIMINGR register for 400 kHz with APB1 = 32 MHz
    // (PRESC=0, SCLDEL=12, SDADEL=5, SCLH=15, SCLL=38)
    I2C1->TIMINGR = 0x00C50F26;
    // Enable I2C1
    I2C1->CR1 |= I2C_CR1_PE;
}


/**
 * @brief Writes a single byte to an I2C slave device register
 * @param slave 7-bit slave address (not shifted)
 * @param addr Register address to write to
 * @param data Data byte to write
 * @return None
 */
void I2C1_Write(uint8_t slave, uint8_t addr, uint8_t data){
    // Wait for bus to be available
    while(I2C1->ISR & I2C_ISR_BUSY);
    // Set up transfer: slave address, 2 bytes, AUTOEND, START
    I2C1->CR2 = 0x00;
    I2C1->CR2 = I2C_CR2_AUTOEND | (2<<16) | (slave<<1) | I2C_CR2_START;
    // Send register address
    while(!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = addr;
    // Send data byte
    while(!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = data;
    // Wait for transmission complete
    while(!(I2C1->ISR & I2C_ISR_TXE));
}

/**
 * @brief Reads multiple bytes from an I2C slave device register
 * @param slave 7-bit slave address (not shifted)
 * @param addr Register address to read from
 * @param data Pointer to buffer to store the read bytes
 * @param size Number of bytes to read
 * @return None
 */
void I2C1_Read(uint8_t slave, uint8_t addr, uint8_t *data, uint8_t size){
    // Wait for bus to be available
    while(I2C1->ISR & I2C_ISR_BUSY);
    // Send register address (write 1 byte, no AUTOEND for repeated START)
    I2C1->CR2 = 0x00;
    I2C1->CR2 = (1<<16) | (slave<<1) | I2C_CR2_START;
    // Send register address
    while(!(I2C1->ISR & I2C_ISR_TXIS));
    I2C1->TXDR = addr;
    // Wait for transfer complete (TC flag)
    while(!(I2C1->ISR & I2C_ISR_TC));
    // Read data (multiple bytes, AUTOEND, RD_WRN=1)
    I2C1->CR2 = 0x00;
    I2C1->CR2 = I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | (size<<16) | (slave<<1) | I2C_CR2_START;
    // Read each byte
    for(uint8_t i = 0; i < size; i++){
        // Wait for data ready
        while(!(I2C1->ISR & I2C_ISR_RXNE));
        data[i] = I2C1->RXDR;
    }
    // Wait for stop condition
    while(!(I2C1->ISR & I2C_ISR_STOPF));
}
