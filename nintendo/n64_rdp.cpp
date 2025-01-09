#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "n64_rdp.h"

const u32 imgbpp[] = { 4, 8, 16, 32 };

void n64_rdp::run_commands(u64* list, u32 num)
{
	u32 i = 0;
	while( i < num )
	{
		u64 cmd = __builtin_bswap64(list[i++]);
		
		switch( (cmd>>56)&0x3F )
		{
		case 0x29: // sync full
			rdp_irq();
			break;
		
		case 0x2D: // scissor
		
		
			break;
		
		case 0x2F: // other modes
			break;
			
		case 0x36: // fill rect
			fill_rect(cmd);
			break;
		
		case 0x37: // fill color
			fill_color = cmd;
			break;
		
		case 0x3F: // color image
			cimg.addr = cmd & 0x7ffff8;
			cimg.width = ((cmd>>32)&0x3ff)+1;
			cimg.bpp = imgbpp[((cmd>>51)&3)];
			cimg.format = ((cmd>>53)&7);
			break;


		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7: 
		case 0x10:
		case 0x11:
		case 0x12:
		case 0x13:
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x17:
		case 0x18:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x1C:
		case 0x1D:
		case 0x1E:
		case 0x1F: 
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23: break; // lots of nop
		default: printf("RDP: Unimpl cmd = $%X\n", u32((cmd>>56)&0x3F)); exit(1);
		}	
	}
}

void n64_rdp::fill_rect(u64 cmd)
{
	//todo: 1/2-cycle and copy modes. check coordinate rounding
	u32 uly = cmd&0xfff;
	u32 ulx = (cmd>>12)&0xfff;
	u32 lry = (cmd>>32)&0xfff;
	u32 lrx = (cmd>>44)&0xfff;
	uly >>= 2;
	ulx >>= 2;
	lry >>= 2;
	lrx >>= 2;
	printf("fill rect (%i, %i) to (%i, %i)\n", ulx, uly, lrx, lry);
	if( cimg.bpp == 16 )
	{
		for(u32 y = uly; y < lry; ++y)
		{
			for(u32 x = ulx; x < lrx; ++x)
			{
				u16 fc = (x&1) ? (fill_color&0xffff) : (fill_color>>16);
				*(u16*)&rdram[cimg.addr + (y*cimg.width*2) + x*2] = __builtin_bswap16(fc);
			}
		}
	} else {
		for(u32 y = uly; y < lry; ++y)
		{
			for(u32 x = ulx; x < lrx; ++x)
			{
				*(u32*)&rdram[cimg.addr + (y*cimg.width*4) + x*4] = __builtin_bswap32(fill_color);
			}
		}
	}
}


