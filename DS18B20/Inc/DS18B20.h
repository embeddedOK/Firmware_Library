#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <stdint.h>
#include <stddef.h>

// Enum for DS18B20 Errors
typedef enum
{
	DS18B20_OK = 0,
	DS18B20_NULL_VALUE,
	DS18B20_INVALID_ACCESS
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
typedef struct DS18B20_BSP_FPTRS
{
	void (*delayUS)(uint32_t delay);
	void (*wire1_setZero)();
	void (*wire1_setOne)();
	uint8_t (*wire1_read)();
} DS18B20_BSP_FPTRS;

// Structure holding DS28B20 device info
typedef struct DS18B20
{
	void (*delayUS)(uint32_t delay);	// BSP function for microsecond delay
	void (*wire1_setZero)();			// BSP function for wire1 set bus to zero
	void (*wire1_setOne)();				// BSP function for wire1 set bus to one
	uint8_t (*wire1_read)();			// BSP function to read wire1 bus
	uint64_t address;					// DS18B20 Vender Address
	uint8_t enumerated;					// Flag when device is enumerated and BSP fptrs are !NULL
	uint8_t power_mode;					// Parasite power = 0
	DS18B20_RESOLUTION resolution;		// Resolution mapping for conversion time lookup
	uint8_t scratchpad[DS18B20_SCRATCHPAD_BYTE_SIZE];
	int16_t temperature;
} DS18B20;



/* Global Functions Exposed */
int8_t DS18B20_init(DS18B20 *devices, DS18B20_BSP_FPTRS *fptrs, uint8_t max_devices);
uint8_t DS18B20_convertTemperature(DS18B20 *device, uint8_t wait_for_complete);

#ifdef DS18B20_FLOAT_POINT
int8_t DS18B20_getTemperature(DS18B20 *device, float *value);
#else
int8_t DS18B20_getTemperature(DS18B20 *device, int16_t *value);
#endif

/* Device specific functions User MAY implement */
uint8_t DS18B20_wire1_calcCRC(uint8_t *message, uint8_t byteLen);

/* Functions exposed only during DEBUG */
#ifdef DS18B20_DEBUG
void DS18B20_wire1_debugToggle(DS18B20 *device);
int8_t DS18B20_resetPulse(DS18B20 *device);
int8_t DS18B20_sendBusBit(DS18B20 *device, uint8_t bit);
#endif

#endif //_DS18B20_H_
