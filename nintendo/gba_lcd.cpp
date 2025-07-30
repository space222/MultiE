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

	if( addr >= 0x04000028 && addr <= 0x0400002E )
	{
		memcpy(&lcd.bg2x, &lcd.regs[0x14], 4); lcd.bg2x = (lcd.bg2x<<4)>>4;
		memcpy(&lcd.bg2y, &lcd.regs[0x16], 4); lcd.bg2y = (lcd.bg2y<<4)>>4;
		memcpy(&lcd.bg3x, &lcd.regs[0x1C], 4); lcd.bg3x = (lcd.bg3x<<4)>>4;
		memcpy(&lcd.bg3y, &lcd.regs[0x1E], 4); lcd.bg3y = (lcd.bg3y<<4)>>4;
		return;
	}
}

u32 gba::read_lcd_io(u32 addr)
{
	//if( addr == 0x04000006 ) return VCOUNT;
	return lcd.regs[(addr&0x7f)>>1];
}










