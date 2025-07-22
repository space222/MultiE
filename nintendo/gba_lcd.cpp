#include "gba.h"

void gba::write_lcd_io(u32 addr, u32 v)
{
	lcd.regs[(addr&0x7f)>>1] = v;
}

u32 gba::read_lcd_io(u32 addr)
{
	//if( addr == 0x04000006 ) return VCOUNT;
	return lcd.regs[(addr&0x7f)>>1];
}










