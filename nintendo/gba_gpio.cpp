#include <print>
#include "gba.h"

void gba::gpio_write(u32 reg, u32 v)
{
	if( reg == 2 ) { gpio_reads_en = v&1; return; }
	if( reg == 1 ) { gpio_dir = v&15; return; }
	
	//std::println("GPIO Write {:X} = ${:X}", reg, v);
	
	//only supported GPIO device will be RTC. 
	u8 CS = (v>>2)&1;
	u8 CLK = v&1;
	u8 SIO = (v>>1)&1;
	
	if( !CS ) { rtc_state = 0; gpio_data = v&7; return; }
	
	if( !(gpio_data&1) && CLK )
	{  // rising CLK, sample SIO
		if( rtc_state < 8 )
		{  // if state < 8, still building cmd byte
			rtc_cmd = (rtc_cmd<<1)|SIO;
			rtc_state += 1;
			if( rtc_state == 8 )
			{ // got a full cmd byte
				if( (rtc_cmd&0xf0) != 0x60 )
				{
					u8 b0 = rtc_cmd&1;
					u8 b1 = (rtc_cmd>>1)&1;
					u8 b2 = (rtc_cmd>>2)&1;
					u8 b3 = (rtc_cmd>>3)&1;
					u8 b4 = (rtc_cmd>>4)&1;
					u8 b5 = (rtc_cmd>>5)&1;
					u8 b6 = (rtc_cmd>>6)&1;
					u8 b7 = (rtc_cmd>>7)&1;
					rtc_cmd = (b0<<7)|(b1<<6)|(b2<<5)|(b3<<4)|(b4<<3)|(b5<<2)|(b6<<1)|b7;
				}
				if( rtc_cmd & 1 )
				{ // hardcoded values that just make Pokemon Emerald not complain on startup
					switch( (rtc_cmd>>1) & 7 )
					{
					case 1: rtc_out = 0x40; break;//rtc_ctrl; break;
					case 2: rtc_out = 0x511303060825; break;//get_datetime(); break;
					case 3: rtc_out = 0x511303; break; //get_time(); break;
					default: break;					
					}				
				}					
			}
		} else if( rtc_cmd&1 ) {
			gpio_data &= ~2;
			gpio_data |= (rtc_out&1)<<1;
			rtc_out >>= 1;
		}
	}
	gpio_data = (gpio_data&2) | (v&5);	
}

u8 gba::gpio_read(u32 reg)
{
	if( reg == 2 ) { return gpio_reads_en; }
	if( reg == 1 ) { return gpio_dir&15; }
	return gpio_data;
}

