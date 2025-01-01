#include <cstring>
#include "n64.h"



void n64::vi_draw_frame()
{
	raise_mi_bit(MI_INTR_VI_BIT);

	u32 width = (640.f * (VI_X_SCALE&0xfff)) / 0x400;
	u32 height = (240.f * (VI_Y_SCALE&0xfff)) / 0x400;
	curwidth = width>0 ? width : 320;
	curheight = height>0 ? height : 240;
	curbpp = 16;

	u32 type = VI_CTRL & 3;
	if( type < 2 )
	{
		memset(fbuf, 0, 640*480*4);
		return;
	}
	
	//printf("VI type %i, %ix%i pixels\n", type, width, height);
	if( type == 2 )
	{
		u32 stride = VI_WIDTH*2;	
		u32 offset = (VI_ORIGIN&0x1FFFffff) + (VI_WIDTH - width)*2;
		for(u32 line = 0; line < height && offset < 8*1024*1024; ++line, offset+=stride)
		{
			for(u32 i = 0; i < width; ++i)
			{
				*(u16*)&fbuf[line*width*2 + i*2] = __builtin_bswap16(*(u16*)&mem[offset + i*2]);
			}
		}
		return;
	}

	curbpp = 32;
	u32 stride = VI_WIDTH*4;	
	u32 offset = (VI_ORIGIN&0x1FFFffff) + (VI_WIDTH - width)*4;
	for(u32 line = 0; line < height && offset < 8*1024*1024; ++line, offset+=stride)
	{
		for(u32 i = 0; i < width; ++i)
		{
			*(u32*)&fbuf[line*width*4 + i*4] = __builtin_bswap32(*(u32*)&mem[offset + i*4]);
		}
	}
}

void n64::vi_write(u32 addr, u32 v)
{
	if( addr == 4 ) 
	{	// VI_V_CURRENT: writes clear irq
		clear_mi_bit(MI_INTR_VI_BIT);
		return; 
	}
	vi_regs[(addr&0x3F)>>2] = v;
}

u32 n64::vi_read(u32 addr)
{
	//if( (addr&0x3F) != 0x10 ) printf("N64 VI read <$%X\n", addr);
	return vi_regs[(addr&0x3F)>>2];
}



