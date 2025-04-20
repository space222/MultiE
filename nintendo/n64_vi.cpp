#include <print>
#include <cstring>
#include "n64.h"

void n64::vi_draw_frame()
{
	u32 H_START = (VI_H_VIDEO>>16)&0x3ff;
	u32 H_END = (VI_H_VIDEO&0x3ff);
	u32 type = VI_CTRL & 3;
	if( (H_START == 0 && H_END == 0) || type < 2 ) 
	{
		memset(fbuf, 0, 640*480*4);
		return;
	}
	
	//if( H_START != 108 ) std::println("H_START = {}", H_START);
	//if( H_END != 748 ) std::println("H_END = {}", H_END);
	
	u32 V_START = (VI_V_VIDEO>>16)&0x3ff;
	if( V_START >= 35 ) V_START -= 35;
	u32 V_END = VI_V_VIDEO&0x3ff;
	if( V_END >= 35 ) V_END -= 35;
	u32 v_height = (V_END - V_START)>>((VI_CTRL&BIT(6))?0:1);
	
	u32 width = ((H_END-H_START) * (VI_X_SCALE&0xfff)) / 0x400;
	u32 height = (240.f * (VI_Y_SCALE&0xfff)) / 0x400;
	curwidth = width>0 ? width : 320;
	curheight = height>0 ? height : 240;
	curbpp = 16;
	
	
	//printf("VI type %i, %ix%i pixels\n", type, width, height);
	if( type == 2 )
	{
		u32 stride = VI_WIDTH*2;	
		u32 offset = (VI_ORIGIN&0x1FFFffff);
		for(u32 line = 0; line < v_height && offset < 8*1024*1024; ++line, offset+=stride)
		{
			if( 0 ) // line < (V_START/2) )
			{
				memset(fbuf + line*width*2, 0, width*2);
				continue;
			}
			for(u32 i = 0; i < width-1; ++i)
			{
				u16 p = __builtin_bswap16(*(u16*)&mem[offset + i*2]);
				u8 b = (p>>11)&0x1f;
				u8 g = (p>>6)&0x1f;
				u8 r = (p>>1)&0x1f;
				*(u16*)&fbuf[line*width*2 + i*2] = (r<<10)|(g<<5)|b;
			}
		}
		return;
	}

	curbpp = 32;
	u32 stride = VI_WIDTH*4;
	u32 offset = (VI_ORIGIN&0x1FFFffff);// + (VI_WIDTH - width)*4;
	memset(fbuf, 0, (V_START)*width*4);
	for(u32 line = 0; line < v_height && offset < 8*1024*1024; ++line, offset+=stride)
	{
		for(u32 i = 0; i < width-1; ++i)
		{
			*(u32*)&fbuf[line*width*4 + i*4] = __builtin_bswap32(*(u32*)&mem[offset + i*4]);
		}
	}
}

void n64::vi_write(u32 addr, u32 v)
{
	addr = (addr&0x3F)>>2;
	if( addr == 4 ) 
	{	// VI_V_CURRENT: writes clear irq
		//printf("N64: VI irq cleared\n");
		clear_mi_bit(MI_INTR_VI_BIT);
		return; 
	}
	vi_regs[addr] = v;
}

u32 n64::vi_read(u32 addr)
{
	//if( (addr&0x3F) != 0x10 ) printf("N64 VI read <$%X\n", addr);
	return vi_regs[(addr&0x3F)>>2];
}



