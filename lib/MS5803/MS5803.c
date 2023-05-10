#include <twi.h>
#include <stdint.h>
#include <MS5803.h>
#include <util/delay.h>

uint16_t getPROMVal(unsigned char TWI_targetSlaveAddress, int promVal) {
    unsigned char messageBuf[8];
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_READ_BIT);

	switch (promVal)
	{
	case 0:
		messageBuf[1] = 0xA0;
		break;
	case 1:
		messageBuf[1] = 0xA2;
		break;
	case 2:
		messageBuf[1] = 0xA4;
		break;
	case 3:
		messageBuf[1] = 0xA6;
		break;
	case 4:
		messageBuf[1] = 0xA8;
		break;
	case 5:
		messageBuf[1] = 0xAA;
		break;
	case 6:
		messageBuf[1] = 0xAC;
		break;
	case 7:
		messageBuf[1] = 0xAE;
		break;
	}

	TWI_Start_Transceiver_With_Data(messageBuf, 2);

	_delay_ms(100);

	// Read value
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (TRUE << TWI_READ_BIT);

	unsigned char received_data[4] = {0};
	TWI_Start_Transceiver_With_Data(messageBuf, 3);
	if (TWI_Get_Data_From_Transceiver(received_data, 3)) {
		return  ((uint16_t) received_data[1]) << 8 
				| ((uint16_t) received_data[0]);
	}
}

uint32_t getPressure(unsigned char TWI_targetSlaveAddress) {
    unsigned char messageBuf[8];

	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_READ_BIT);
	//D1 Pressure reading at 2048 OSR precision to improve SNR
	messageBuf[1] = 0x46;
	TWI_Start_Transceiver_With_Data(messageBuf, 2);

	_delay_ms(100);

	// Read ADC command
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_READ_BIT);
	messageBuf[1] = 0x0;
	TWI_Start_Transceiver_With_Data(messageBuf, 2);

	//Read ADC Response
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (TRUE << TWI_READ_BIT);
	TWI_Start_Transceiver_With_Data(messageBuf, 4);

	// Data conversion to uint32 from trailing 3 bytes of receive buffer
	unsigned char received_data[6] = {0};
	if (TWI_Get_Data_From_Transceiver(received_data, 4)) {
		return ((uint32_t) received_data[1]) << 16 
				| ((uint32_t) received_data[2]) << 8 
				| ((uint32_t) received_data[3]);	
	}

	return 0;
}

uint32_t getTemperature(unsigned char TWI_targetSlaveAddress) {
    unsigned char messageBuf[8];

	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_READ_BIT);
	//D1 Pressure reading at 2048 OSR precision to improve SNR
	messageBuf[1] = 0x56;
	TWI_Start_Transceiver_With_Data(messageBuf, 2);

	_delay_ms(100);

	// Read ADC command
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (FALSE << TWI_READ_BIT);
	messageBuf[1] = 0x0;
	TWI_Start_Transceiver_With_Data(messageBuf, 2);

	//Read ADC Response
	messageBuf[0] = (TWI_targetSlaveAddress << TWI_ADR_BITS) | (TRUE << TWI_READ_BIT);
	TWI_Start_Transceiver_With_Data(messageBuf, 4);

	// Data conversion to uint32 from trailing 3 bytes of receive buffer
	unsigned char received_data[6] = {0};
	if (TWI_Get_Data_From_Transceiver(received_data, 4)) {
		return ((uint32_t) received_data[1]) << 16 
				| ((uint32_t) received_data[2]) << 8 
				| ((uint32_t) received_data[3]);	
	}

	return 0;
}