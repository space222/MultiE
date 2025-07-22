#include <cstring>
#include "LCDEngine.h"

inline u32 c16to32(u16 c)
{
	u32 R = (c&0x1f)<<(24+3);
	u32 G = ((c>>5)&0x1f)<<(16+3);
	u32 B = ((c>>10)&0x1f)<<(8+3);
	return R|G|B;
}

void LCDEngine::draw_scanline(u32 L)
{
	if( ! (regs[0] & BIT(7)) )
	{
		switch( regs[0] & 7 )
		{
		case 0: draw_mode_0(L); return;
		case 1: draw_mode_1(L); return;
		case 2: draw_mode_2(L); return;
		case 3: draw_mode_3(L); return;
		case 4: draw_mode_4(L); return;
		case 5: draw_mode_5(L); return;
		default: break;
		}
	}	
	memset(fbuf, 0, 240*4);
}

void LCDEngine::draw_mode_0(u32 Y)
{

}

void LCDEngine::draw_mode_1(u32 Y)
{

}

void LCDEngine::draw_mode_2(u32 Y)
{

}

void LCDEngine::draw_mode_3(u32 Y)
{
	for(u32 x = 0; x < 240; ++x)
	{
		fbuf[240*Y + x] = c16to32(*(u16*)&VRAM[240*Y + x]);
	}
}

void LCDEngine::draw_mode_4(u32 Y)
{
	u32 base = (regs[0]&BIT(4)) ? 0xA000 : 0;
	for(u32 x = 0; x < 240; ++x)
	{
		fbuf[240*Y + x] = c16to32(*(u16*)&palette[VRAM[base + 240*Y + x]<<1]);
	}
}

void LCDEngine::draw_mode_5(u32 Y)
{

}


