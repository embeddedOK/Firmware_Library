#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <stdint.h>
#include <stddef.h>

/* This Firmware driver does not support float points and instead returns the whole and decimal parts separately in a structure */
typedef struct DS18B20_TEMPERATURE
{
	int8_t integer;
	uint16_t decimal;
} DS18B20_TEMPERATURE;

// Enum for DS18B20 Errors
typedef enum
{
	DS18B20_OK = 0,
	DS18B20_NULL_VALUE,
	DS18B20_INVALID_ACCESS,
	DS18B20_BAD_CRC,
	DS18B20_BUS_IN_USE,

} DS18B20_ERRORS;

// Enum for Resolution time mapping to lookup table
typedef enum
{
	DS18B20_RESOLUTION_9BITS = 0,
	DS18B20_RESOLUTION_10BITS,
	DS18B20_RESOLUTION_11BITS,
	DS18B20_RESOLUTION_12BITS
} DS18B20_RESOLUTION;



// Structure to hold the required user implemented Board Specific function pointers
typedef struct DS18B20_wire1_interface
{
	void (*delayUS)(uint32_t delay);
	void (*setZero)();
	void (*setOne)();
	uint8_t (*read)();
	uint8_t in_use;
} DS18B20_wire1_interface;

// Structure holding DS28B20 device info
typedef struct DS18B20
{
	DS18B20_wire1_interface *wire1;		// Pointer to wire1 implement functions
	uint8_t wire1_valid;				// wire1 pointers assigned and valid
	uint64_t address;					// DS18B20 Vender Address
	uint8_t enumerated;					// Flag when device is enumerated and BSP fptrs are !NULL
	uint8_t power_mode;					// Parasite power = 0
	uint8_t converting;					// Flag when device is converting
	uint8_t scratchpad[DS18B20_SCRATCHPAD_BYTE_SIZE];	// Raw Values LSb first for CRC check
	DS18B20_RESOLUTION resolution;		// Resolution mapping for conversion time lookup
	int8_t  th;							// High Temperature Alert Value MSb first
	int8_t  tl;							// Low Temperature Alert Value MSb first
	int16_t temperature_raw;			// 16bit Raw Temperature value
	int8_t temperature_integer;			// Temperature Integer Number
	uint8_t temperature_decimal;		// Temperature Decimal Number
} DS18B20;



/* Global Functions Exposed */
int8_t DS18B20_init(DS18B20 *devices, DS18B20_wire1_interface *wire1_fptrs, uint8_t max_devices);
uint8_t DS18B20_convertTemperatureDevice(DS18B20 *device, uint8_t wait_for_complete);
uint8_t DS18B20_convertTemperatureAll(DS18B20 *device);
uint8_t DS18B20_waitForBus(DS18B20 *device);


int8_t DS18B20_getTemperature(DS18B20 *device, DS18B20_TEMPERATURE *value);

/* Device specific functions User MAY implement */
uint8_t DS18B20_wire1_calcCRC_LSB(uint8_t *message, uint8_t byteLen);

/* Functions exposed only during DEBUG */
#ifdef DS18B20_DEBUG
void DS18B20_wire1_debugToggle(DS18B20 *device);
int8_t DS18B20_resetPulse(DS18B20 *device);
int8_t DS18B20_sendBusBit(DS18B20 *device, uint8_t bit);
#endif

#endif //_DS18B20_H_
