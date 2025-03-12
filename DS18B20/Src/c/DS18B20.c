/* DS18B20.c
 * 
 * Publicly avialable DS18B20 firmware driver from embeddedOK,LLC
 * 
 * 
 */
#include <stdint.h>
#include <stddef.h>

#include "DS18B20_REG.h"
#include "DS18B20.h"

// DS18B20 converson time lookup table
const uint32_t DS18B20_CONVERSION_TIME_MS[] =
{
		DS18B20_CONFIG_9BITS_TCONV_MS,
		DS18B20_CONFIG_10BITS_TCONV_MS,
		DS18B20_CONFIG_11BITS_TCONV_MS,
		DS18B20_CONFIG_12BITS_TCONV_MS
};

int8_t DS18B20_resetPulse(DS18B20 *device)
{
	uint32_t master_rx_time = 0; //Track time in Master RX mode, 480us Min
	uint8_t reading = 0;

	if(device->enumerated == 0)
	{
		return -DS18B20_INVALID_ACCESS;
	}

	//Send Wire1 reset pulse width for 480us min
	device->wire1_setZero();
	device->delayUS(DS18B20_RESET_PULSE_WIDTH_MIN_US);
	device->wire1_setOne();

	//Check for DS18B20 TX Presence response
	device->delayUS(DS18B20_PRESENCE_PULSE_WAITS_MAX_US);
	master_rx_time += DS18B20_PRESENCE_PULSE_WAITS_MAX_US;

	reading = device->wire1_read();
	//Delay 15us and Read again
	//Check for DS18B20 TX Presence
	device->delayUS(DS18B20_PRESENCE_PULSE_RESAMPLE_US);
	master_rx_time += DS18B20_PRESENCE_PULSE_RESAMPLE_US;
	reading <<= 1;
	reading |= device->wire1_read();

	//Wait for remaining Master RX mode Time
	device->delayUS(DS18B20_RESET_PULSE_WIDTH_MIN_US-master_rx_time);

	return reading;
}

int8_t DS18B20_sendBusStart(DS18B20 *device)
{
	if(device->enumerated == 0)
	{
		return -DS18B20_INVALID_ACCESS;
	}

	//Wait for 1us recovery time in case we just sent a bit
	device->wire1_setOne();
	while(device->wire1_read() == 0);	//Wait for RC and bus to go high

	device->delayUS(DS18B20_SLOT_RECOVERY_US);
	device->wire1_setZero();
	device->delayUS(DS18B20_START_OF_SLOT_MIN_US);
	device->wire1_setOne();

	return DS18B20_OK;
}

int8_t DS18B20_sendBusBit(DS18B20 *device, uint8_t bit)
{
	int8_t ret = DS18B20_sendBusStart(device);
	if(ret != DS18B20_OK)
	{
		return ret;
	}

	if(bit)
	{
		device->wire1_setOne();
	}
	else
	{
		device->wire1_setZero();
	}

	//Wait for DS18B20 Sample Time DS18B20_TIME_SLOT_MIN_US
	device->delayUS(DS18B20_TIME_SLOT_MIN_US);
	device->wire1_setOne();

	return DS18B20_OK;
}

int8_t DS18B20_readBusBit(DS18B20 * device)
{
	uint8_t reading = 0;
	int8_t ret = DS18B20_sendBusStart(device);
	if(ret != DS18B20_OK)
	{
		return ret;
	}

	device->delayUS(SD18B20_MASTER_READ_SLOT_US-DS18B20_START_OF_SLOT_MIN_US);
	reading = device->wire1_read();
	device->delayUS(DS18B20_TIME_SLOT_MIN_US-SD18B20_MASTER_READ_SLOT_US);

	return reading;
}

int8_t DS18B20_sendBusCMD(DS18B20 * device, uint8_t cmd)
{
	if(device->enumerated == 0)
	{
		return -DS18B20_INVALID_ACCESS;
	}

	for(int x=0; x<DS18B20_CMD_BIT_SIZE; x++)
	{
		DS18B20_sendBusBit(device, cmd&1);
		cmd >>=1;	//Right shift cmd
	}

	return DS18B20_OK;
}


int8_t DS18B20_matchROM(DS18B20 *device)
{
	int8_t ret = DS18B20_resetPulse(device);
	if(ret != DS18B20_OK)	//reset the bus and verify device presence
	{
		return ret;
	}

	DS18B20_sendBusCMD(device, DS18B20_CMD_MATCH_ROM);

	for(int x=0; x<DS18B20_ROM_CODE_BIT_SIZE; x++)
	{
		DS18B20_sendBusBit(device, ((uint64_t)(device->address)>>x) & 1);
	}
	return DS18B20_OK;
}

uint8_t DS18B20_convertTemperature(DS18B20 *device, uint8_t wait_for_complete)
{

	uint8_t check_cnt = 2;
	int8_t ret = DS18B20_matchROM(device);
	if(ret != DS18B20_OK)
	{
		return ret;
	}

	DS18B20_sendBusCMD(device, DS18B20_CMD_CONVERT_T);

	for(; check_cnt > 0; check_cnt--)
	{
		ret = DS18B20_readBusBit(device);
		if(ret == 0)
		{
			break;
		}
	}

	if(check_cnt == 0)	//Never detected sensor conversion start
	{
		return -1;
	}

	if(wait_for_complete)
	{
		uint32_t convert_time_US = DS18B20_CONVERSION_TIME_MS[device->resolution]*1000; //  Conversion delay in uS
		device->delayUS(convert_time_US);
		while(DS18B20_readBusBit(device) == 0);
	}

	return DS18B20_OK;
}

int8_t DS18B20_getScratchPad(DS18B20 * device)
{
	int8_t ret = DS18B20_matchROM(device);
	if(ret != DS18B20_OK)
	{
		return ret;
	}

	DS18B20_sendBusCMD(device, DS18B20_CMD_READ_SCRATCHPAD);
	for(int x=0; x<DS18B20_SCRATCHPAD_BYTE_SIZE; x++)
	{
		device->scratchpad[x] = 0;
		for(int y=0; y<8; y++)
		{
			device->scratchpad[x] <<=1;
			device->scratchpad[x] |= DS18B20_readBusBit(device);
		}
	}

	device->temperature = 0;
	for(int x=(DS18B20_TEMPERATURE_BIT_SIZE/8); x>0; x--)
	{
			device->temperature <<=8;
			device->temperature |= device->scratchpad[x-1];
	}
	for(int x=0; x<DS18B20_TEMPERATURE_BIT_SIZE; x++)
	{
		//TODO: Convert to temp here?
	}
	return DS18B20_OK;
}
int8_t DS18B20_getTemperature(DS18B20 *device, int16_t *value)
{
	int8_t ret = DS18B20_getScratchPad(device);
	*value = device->scratchpad[1]<<8 | device->scratchpad[0];
	return ret;
}
// Enumerate up to max_devices DS18B20 devices utilizing the SEARCH ROM Command and populate the DS18B20 structure
int8_t DS18B20_init(DS18B20 *devices, DS18B20_BSP_FPTRS *fptrs, uint8_t max_devices)
{
	uint64_t rom_code = 0;
	uint64_t address = 0;
	uint64_t address_contention = 0;
	uint8_t  crc_value = 0;
	uint8_t dev_cnt = 0;
	uint8_t reading = 0;
	uint8_t crc_cal = 0;

	if(fptrs->delayUS 		== NULL
	|| fptrs->wire1_setZero == NULL
	|| fptrs->wire1_setOne 	== NULL
	|| fptrs->wire1_read 	== NULL )
	{
		return -DS18B20_NULL_VALUE;
	}

	while(dev_cnt < max_devices)
	{
		devices->delayUS 		= fptrs->delayUS;
	    devices->wire1_setZero 	= fptrs->wire1_setZero;
		devices->wire1_setOne 	= fptrs->wire1_setOne;
		devices->wire1_read   	= fptrs->wire1_read;
		devices->enumerated     = 1;	//Fptrs set, set enumerated to allow resetPulse check
		reading = DS18B20_resetPulse(devices);
		if(reading != 0)
		{
		 //No Device Detected
			devices->enumerated = 0;	//Reset enumerated bit, not valid when we return 0 devices found but reset in case we use it in main logic anyway
			return 0;
		}
		DS18B20_sendBusCMD(devices, DS18B20_CMD_SEARCH_ROM);
		for(int x=0; x<DS18B20_ROM_CODE_BIT_SIZE; x++)
		{
			reading = DS18B20_readBusBit(devices);	//Read TX Bit 0
			reading <<= 1;
			reading |= DS18B20_readBusBit(devices);	//Read TX ~Bit 0
			switch(reading)
			{
				case 0:	//"00" Bus contention, Two Devices different address bits
					address_contention |= (uint64_t)1<<x;	//Store for later use, navigate 0 address first
					//TODO: loop through device tree branches to enumerate multiple devices
					break;
				case 1:	//"01" Address 0 match
					//Nothing to do as Address bits default to 0
					break;
				case 2:	//"10" Address 1 match
					rom_code |= (uint64_t)1<<(DS18B20_ROM_CODE_BIT_SIZE-1-x);	//Store 1 in address, LSb first for CRC
					address |= (uint64_t)1<<x;	//Store 1 in address, LSb as LSb
					break;
				case 3:	//"11" No Devices responded, maybe error in master TX bit?
					devices->enumerated = 0;	//Reset enumerated bit, not valid when we return 0 devices found but reset in case we use it in main logic anyway
					return dev_cnt;
				default:
					break;
			}
			DS18B20_sendBusBit(devices, ((uint64_t)address>>x) & 1);
		}
		//got entire address!!
		//Check CRC
		crc_value = (uint8_t)rom_code; //Store message CRC
		rom_code >>= 8;	//Shift out recv'd crc code TODO: Parameterize
		crc_cal = DS18B20_wire1_calcCRC((uint8_t *)&rom_code, DS18B20_ADDRESS_BIT_SIZE/8);
		if(crc_cal == crc_value)
		{
//			devices->enumerated = 1;	// enumerated already set
			devices->address = address;

			devices->resolution = DS18B20_RESOLUTION_12BITS;
			devices++;
			dev_cnt++;
		}
		else	// Bus message error, loop again TODO: add retry/error cnt
		{
			devices->enumerated = 0;
		}
	}
	return dev_cnt;
}

/*
 * Device specific functions User MAY implement
 */
////Calculate CRC based on CRC_POLY of 100110001, BITS are LSB First so
__attribute__ ((weak)) uint8_t DS18B20_wire1_calcCRC(uint8_t *message, uint8_t byteLen)	//Weak function as instead of slow bit wise loop we can override it to utilize CRC hw module
{

	uint16_t crc_poly = ((uint8_t)DS18B20_CRC_POLYNOMIAL)<<8;
	uint16_t crc;

	uint8_t xor_op=0;

	crc = *(message+byteLen-1)<<8;
	for(int x=byteLen-1; x>=0; x--)
	{
		crc &=0xFF00;
		if(x > 0)	//Add next message byte to end
		{
			crc |= *(message+x-1);
		}

		for(int y = 0; y<8; y++)
		{
			if(crc & 0x8000)	//MSb of value is 1, we can apply CRC XOR operation
			{
				xor_op = 1;
			}
			crc <<=1;	//Shift over value

			if(xor_op) //Since CRC poly MSb is always 1 and MSb of value is 1, XOR will always be 0! So we can shift left and XOR 8bits or poly with 8bits of value!
			{
				xor_op = 0;
				crc ^= crc_poly;
			}
		}
	}
	return (crc>>8);	//value should be CRC!
}

#ifdef DS18B20_DEBUG
void DS18B20_wire1_debugToggle(DS18B20 *device)
{
	if(device->enumerated ==0)
	{
		return;
	}
	for(int x=0; x<100; x++)
	{
		device->delayUS(1);
		device->wire1_setZero();
		device->delayUS(2);
		device->wire1_setOne();
	}
}
#endif
