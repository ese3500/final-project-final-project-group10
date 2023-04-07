#include <i2c.h>

void i2cinit(void) {
	TWSR = 0x00; // set prescaler to 1

	TWBR = 0x0C; // set bit rate register to 12

	TWCR = (1<<TWEN); // enable TWI
}

void i2cStart(void) {

	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // send start condition

	while (!(TWCR & (1<<TWINT))); // wait for TWINT flag set
	
}

void i2cStop(void) {
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // send stop condition
}

uint8_t i2cWrite(uint8_t data) {
	TWDR = data; // load data into TWDR register
	TWCR = (1 << TWINT) | (1 << TWEN); // start transmission of data
	while (!(TWCR & (1 << TWINT))); // wait for TWINT flag set to stop transmission
	return TW_STATUS; // return TWI status
}


uint8_t i2cReadAck(void) {
	// Requires ACK bit as 9th bit
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); // listen for transmission of data with ack bit
	while (!(TWCR & (1 << TWINT))); // wait for TWINT flag set to have value finished 
	return TWDR; // return received data
}

uint8_t i2cStatus(void) {
	return TW_STATUS;
}
