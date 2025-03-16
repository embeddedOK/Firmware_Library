#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <stdint.h>
#include <stddef.h>

#define DS18B20_WAIT_FOR_BUS_DELAY_US 100	//Number of us to wait between checking for bus ready during wait_for_bus call
// Enum for DS18B20 Errors
typedef enum
{
	DS18B20_OK = 0,			// No Error
	DS18B20_NULL_VALUE,		// A Null value detected, bad function pointer?
	DS18B20_INVALID_ACCESS,	// Invaild Bus or device access
	DS18B20_CRC_ERROR,		// Bad CRC check
	DS18B20_DATA_ERROR,		// Bad Data check
	DS18B20_BUS_IN_USE,		// Bus currently in use TODO: not valid as we wait_for_bus?
	DS18B20_BUS_ACTION_FAILED,	//Bus Action failed

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
	void (*delayUS)(uint32_t delay);	// Function pointer to function for delay with 1uS resolution
	void (*setBus)(uint8_t value);		// Function pointer to function for setting wire1 bus to 0 (Low) or 1 (High)
	void (*enableParasitePower)(uint8_t value); // Function pointer to function for setting 1wire power bypass 0 (Disabled) or 1 (Enabled
	uint8_t (*read)();					// Function pointer to function to read current wire1 bus value
	uint8_t in_use;						// Variable to track when wire1 bus is in use, (eg. Temperature conversion in progress)
} DS18B20_wire1_interface;

// Structure holding DS28B20 device info
typedef struct DS18B20
{
	DS18B20_wire1_interface *wire1;		// Pointer to wire1 BSP implement functions
	uint8_t wire1_valid;				// Flag to track wire1 function pointers are assigned and valid
	uint64_t address;					// DS18B20 ROM Code aka Vender Address
	uint8_t enumerated;					// Flag to track when device is enumerated
	uint8_t power_mode;					// Parasite power = 0
//	uint8_t converting;					// Flag when device is converting //TODO: remove no need as wire1->in_use tracks this
	uint8_t scratchpad[DS18B20_SCRATCHPAD_BYTE_SIZE];	// Device Scratchpad stored MSb first
	DS18B20_RESOLUTION resolution;		// Resolution mapping for conversion time lookup
	int8_t  th;							// High Temperature Alert Value stored MSb first
	int8_t  tl;							// Low Temperature Alert Value stored MSb first
	int16_t temperature_raw;			// 16bit Raw Temperature value stored MSb first
	int8_t temperature_integer;			// Calculated Temperature Integer Number
	uint16_t temperature_decimal;		// Calculated Temperature Decimal Number
} DS18B20;

/* Global Functions Exposed */
// Function to enumerate up to max_devices wire1 devices on a bus controlled by functions stored in wire1_fptrs.
//Each device is stored in passed devices pointer.
int8_t DS18B20_init(DS18B20 *devices, DS18B20_wire1_interface *wire1_fptrs, uint8_t max_devices);

// Sends convert temperature command to specific device,
//returns without waiting for conversion to complete and wire1 bus to free up so wire1->in_use is set
int8_t DS18B20_convertTemperatureDevice(DS18B20 *device);
// Sends convert temperature command to all devices on the bus using the SKIP_ROM command,
//returns without waiting for conversion to complete and wire1 bus to free up so wire1->in_use is set
int8_t DS18B20_convertTemperatureAll(DS18B20 *device);
// Waits for wire1 Bus to become free by sending a read slot and waiting until 1 is returned
int8_t DS18B20_waitForBus(DS18B20 *device);

// Reads scratchpad and stores values in device data buffers
int8_t DS18B20_getTemperature(DS18B20 *device);
// Writes Values stored in device data buffers to the scratchpad
int8_t DS18B20_writeScratchpad(DS18B20 *device);

/* Device specific functions User MAY implement */
//Calulates the DS18B20 CRC value, message is passed as MSb first and bits are flipped in this function
uint8_t DS18B20_wire1_calcCRC_LSB(uint8_t *message, uint8_t byteLen);

/* Functions exposed only during DEBUG */
#ifdef DS18B20_DEBUG
void DS18B20_wire1_debugToggle(DS18B20 *device);
int8_t DS18B20_resetPulse(DS18B20 *device);
int8_t DS18B20_sendBusBit(DS18B20 *device, uint8_t bit);
#endif

#endif //_DS18B20_H_
