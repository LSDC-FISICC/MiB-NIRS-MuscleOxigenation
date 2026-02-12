#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>

void I2C1_Config(void);
void I2C1_Write(uint8_t slave, uint8_t addr, uint8_t data);
void I2C1_Read(uint8_t slave, uint8_t addr, uint8_t *data, uint8_t size);

#endif /* I2C_H_ */    