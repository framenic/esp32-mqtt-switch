
#define SLED_ON				0x0F
#define SLED_OFF			0x00
#define SLED_FAST_BLINK		0x3F
#define SLED_NORMAL_BLINK	0x2F
#define SLED_SLOW_BLINK		0x1F
#define SLED_FAST_FLASH		0xBF
#define SLED_NORMAL_FLASH	0xAF
#define SLED_SLOW_FLASH		0x9F

void set_sled_state(uint8_t sled_state);
void set_sled_blink(uint8_t sled_state,uint8_t blinks);

void update_sled_state();