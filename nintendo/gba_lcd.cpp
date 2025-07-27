#include "gba.h"

void gba::write_lcd_io(u32 addr, u32 v)
{
	if( addr == 0x04000004 )
	{
		v &= 0xff38;
		lcd.regs[2] &= ~0xff38;
		lcd.regs[2] |= v;
		return;
	}
	if( addr == 0x04000006 ) return;
	lcd.regs[(addr&0x7f)>>1] = v;
}

u32 gba::read_lcd_io(u32 addr)
{
	//if( addr == 0x04000006 ) return VCOUNT;
	return lcd.regs[(addr&0x7f)>>1];
}










