
typedef enum DS18B20_RESOLUTION 
{
	DS18B20_RESOLUTION_9BITS = 0,
	DS18B20_RESOLUTION_10BITS,
	DS18B20_RESOLUTION_11BITS,
	DS18B20_RESOLUTION_12BITS 
};

typedef struct DS18B20
{
	uint64_t rom_code;
	uint64_t address;
	uint8_t power_mode;
	uint16_t temperature;
	uint8_t th_reg;
	uint8_t tl_reg;
	uint8_t config_reg;	
} DS18B20;