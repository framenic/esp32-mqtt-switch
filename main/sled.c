#include "stdint.h"

#include "sled.h"
#include "board_gpio.h"



static uint8_t c_sled_state = 0;

/* used to set frequency, number of blinks, on and off
   
  WXFF NNNN
  | |    0000 off, 1111 on, otherwise number of blinks, up to 14
  | |--> 00 0HZ eg. permanent, slow 01 i.e 0.625 Hz, normal 10 i.e 1.25 Hz, fast 11 i.e 2.5 Hz  
  |----> 0 normal 50% duty, 1 flash 100ms
  
*/


static uint8_t count_sled_state = 0;

void set_sled_state(uint8_t sled_state)
{
	c_sled_state = sled_state;
	count_sled_state = 0;
}

void set_sled_blink(uint8_t sled_state,uint8_t blinks)
{
	c_sled_state = (sled_state & 0xF0) | (blinks & 0x0F);
	count_sled_state = 0;
}

// must be called every 100ms

void update_sled_state()
{
	
	static uint8_t sled_pstate = 0;
	uint8_t sled_state = 0;
	
	if (count_sled_state++==16) count_sled_state = 0;
	
	switch ((c_sled_state >> 4) & 0x03) {
		case 0: //permanent
			if ((c_sled_state & 0x0F)==0x00) sled_state = 0;
			else if ((c_sled_state & 0x0F)==0x0F) sled_state = 1;
				else c_sled_state=0x00;
				break;
		case 1: //slow
				if (c_sled_state & 0x0F) sled_state = (count_sled_state & 0x08) ? 0 : 1;
				else sled_state = 0;
				break;
		case 2: //normal
				if (c_sled_state & 0x0F) sled_state = (count_sled_state & 0x04) ? 0 : 1;
				else sled_state = 0;
				break;
		case 3: //fast
				if (c_sled_state & 0x0F) sled_state = (count_sled_state & 0x01) ? 0 : 1;
				else sled_state = 0;
				break;
	}
	
	if ((c_sled_state & 0x80) && (sled_pstate)) { // flash
		led_off();
	}
	
	if (sled_state!=sled_pstate) { // transition
		if (sled_pstate) {
			if (((c_sled_state & 0x0F)!=0x00) && ((c_sled_state & 0x0F)!=0x0F)) c_sled_state= (c_sled_state & 0xF0) | ((c_sled_state & 0x0F)-1);
		}
		led_set(sled_state);
		sled_pstate = sled_state;
		
	}
}