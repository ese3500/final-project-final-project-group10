#include <i2c.h>
#include <util/delay.h>

// USING CSB High ADDRESS, 0b1110110.  Switch to 0b1110111 for CSB Low
void init(void) {
	i2cinit();
}

void reset(void) {
	i2cStart();
	if (i2cStatus() != 0x08) {
		// Error
	} else {
		uint8_t addr = 0b1110110;
		TWI_Write(addr);
		// Acknowledged transmission
		if (i2cStatus() != 0x18) {
			// Error
		} else {
			i2cWrite(0x1E);
			// Acknowledged transmission
			if (i2cStatus() != 0x28) {
				// Error
			} else {
				TWI_Stop();
			}
		}


uint16_t readProm(uint8_t promNmbr) {
	// Prom number is 0-7
	uint8_t promAddr = 0xA0 + (promNmbr * 2);	

	// If prom == 0, there was an  error
	i2cStart();
	uint16_t prom[16];
	if (i2cStatus() != 0x08) {
		//Throw error here
	} else {
		uint8_t addr = 0b1110110;
		i2cWrite(addr);
		// Acknowledged transmission
		if (i2cStatus != 0x18) {
			// Throw error here
			return prom;
		} else {
			i2cWrite(promAddr);
			// Acknowledged transmission
			if (i2cStatus() != 0x28) {
				return prom;
			}

		 	uint8_t first = i2cReadAck();
			if (i2cStatus() != 0x50) {
				// Throw error here
				return prom;
			} 
			
			uint8_t second = i2cReadNack();
			if (i2cStatus() != 0x58) {
			// Throw error here
				return prom;
			} else {
				i2cStop();
				prom = ((uint16_t)first << 8) | second;
				return prom;
			}
		}

uint32_t readPressure() {
	i2cStart();
	uint32_t pressure;
	if (i2cStatus() != 0x08) {
		return pressure;
	} else {
		uint8_t addr = 0b1110110;
		i2cWrite(addr)
		if (i2cStatus != 0x18) {
			return pressure;
		} else {
			// Send D1 Pressure Command
			i2cWrite(0x48);
			if (i2cStatus() != 0x28) {
				return pressure;
			} 

			i2cStop();

			// Wait for conversion to complete
			_delay_ms(50);
			
			// Send Read Command
			i2cStart();
			if (i2cStatus() != 0x08) {
				return pressure;
			}
			
			i2cWrite(addr);

			if (i2cStatus() != 0x18) {
				return pressure;
			} else {
				i2cWrite(0x00);
				if (i2cStatus() != 0x28) {
					return pressure;
				}
			}
			
			// Data Reading section
			for (int i = 3; i >= 0; i--) {
				if (i == 0) {
					uint8_t val = i2cReadNack();
				} else {
					uint8_t val = i2cReadAck();
				}
				
				if (i2cStatus() != 0x50) {
					return pressure;
				} else {
					pressure |= (uint32_t)val << (i * 8);

			} 	
			i2cStop();
			return pressure;
}

uint32_t readTemperature() {
	
	i2cStart();
	uint32_t temperature;
	if (i2cStatus() != 0x08) {
		return temperature;
	} else {
		uint8_t addr = 0b1110110;
		i2cWrite(addr)
		if (i2cStatus != 0x18) {
			return temperature;
		} else {
			// Send D2 Temperature Command
			i2cWrite(0x48);
			if (i2cStatus() != 0x28) {
				return temperature;
			} 

			i2cStop();

			// Wait for conversion to complete
			_delay_ms(50);
			
			// Send Read Command
			i2cStart();
			if (i2cStatus() != 0x08) {
				return temperature;
			}
			
			i2cWrite(addr);

			if (i2cStatus() != 0x18) {
				return temperature;
			} else {
				i2cWrite(0x00);
				if (i2cStatus() != 0x28) {
					return temperature;
				}
			}
			
			// Data Reading section
			for (int i = 3; i >= 0; i--) {
				if (i == 0) {
					uint8_t val = i2cReadNack();
				} else {
					uint8_t val = i2cReadAck();
				}

				if (i2cStatus() != 0x50) {
					return temperature;
				} else {
					temperature |= (uint32_t)val << (i * 8);

			} 	
			i2cStop();
			return temperature;

}

