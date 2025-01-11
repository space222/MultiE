#include <string>
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

		case 0x24: // texture rectangle
			texture_rect(cmd, __builtin_bswap64(list[i++]));
			break;
		case 0x25: // texture flipped rect
			texture_rect_flip(cmd, __builtin_bswap64(list[i++]));
			break;
		
		case 0x2D: // scissor
		
		
			break;
		
		case 0x2F: // other modes
			other.cycle_type = (cmd>>52)&3;
			other.alpha_compare_en = cmd&1;
			//fprintf(stderr, "set other modes = $%lX, ctype = %i\n", cmd, other.cycle_type);
			break;
			
		case 0x32: // set tile size
			set_tile_size(cmd);
			break;
			
		case 0x34: // load tile
			load_tile(cmd);
			break;
		case 0x35: // set tile
			set_tile(cmd);		
			break;
			
		case 0x36: // fill rect
			fill_rect(cmd);
			break;
		
		case 0x37: // fill color
			fill_color = cmd;
			break;
		case 0x38: // fog color
			fog_color = dc::from32(cmd);
			break;
		case 0x39: // blend color
			blend_color = dc::from32(cmd);
			break;
		case 0x3A: // primitive color
			break;
		case 0x3B: // environment color
			env_color = dc::from32(cmd);
			break;	
		case 0x3C: // set combine mode
			break;
		
		case 0x3D: // texture image
			teximg.addr = cmd & 0x7ffff8;
			teximg.width = ((cmd>>32)&0x3ff)+1;
			teximg.bpp = imgbpp[((cmd>>51)&3)];
			teximg.format = ((cmd>>53)&7);
			break;
		case 0x3E: // depth image
			depth_image = cmd & 0x7ffff8;
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
		
		case 0x26: // sync load
			break;
		case 0x27: // sync pipe
			break;
		case 0x28: // sync tile
			break;
		case 0x29: // sync full
			rdp_irq();
			break;
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
	//printf("fill rect (%i, %i) to (%i, %i)\n", ulx, uly, lrx, lry);
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

int texnum = 0;

void n64_rdp::load_tile(u64 cmd)
{
	auto& T = tiles[(cmd>>24)&7];
	u32 ulS = (cmd>>44)&0xfff;
	u32 ulT = (cmd>>32)&0xfff;
	u32 lrS = (cmd>>12)&0xfff;
	u32 lrT = cmd&0xfff;
	
	T.SL = ulS;
	T.SH = lrS;
	T.TL = ulT;
	T.TH = lrT;
	
	ulS >>= 2; ulT >>= 2; lrS >>= 2; lrT >>= 2;
	//printf("RDP: Load Tile (%i,%i) to (%i,%i)\n", ulS, ulT, lrS, lrT);
	
	u32 roffs = (((ulS*T.bpp)+7)/8) + teximg.addr;
	u32 rdram_stride = ((teximg.width*T.bpp) + 7) / 8; 

	for(u32 Y = ulT; Y <= lrT; ++Y)
	{
		memcpy(tmem+T.addr*8+((Y-ulT)*T.line*8), rdram+roffs+(Y*rdram_stride), rdram_stride);
	}
}

void n64_rdp::set_tile(u64 cmd)
{
	auto& T = tiles[(cmd>>24)&7];
	T.addr = (cmd>>32)&0x1ff;
	T.palette = (cmd>>20)&0xf;
	T.format = (cmd>>53)&7;
	T.bpp = imgbpp[(cmd>>51)&3];
	T.line = (cmd>>41)&0x1ff;
	
	T.clampT = (cmd&BIT(19));
	T.mirrorT = (cmd&BIT(18));
	T.maskT = (cmd>>14)&0xf;
	T.shiftT = (cmd>>10)&0xf;
	T.clampS = cmd&BIT(9);
	T.mirrorS = cmd&BIT(8);
	T.maskS = (cmd>>4)&0xf;
	T.shiftS = cmd&0xf;
	
	//if( T.clampT || T.mirrorT || T.clampS || T.mirrorS )
	//	fprintf(stderr, "cT = %i, mT = %i, cS = %i, mS = %i\n", !!T.clampT, !!T.mirrorT, !!T.clampS, !!T.mirrorS);
	//fprintf(stderr, "maskT = $%X, maskS = $%X\n", T.maskT, T.maskS);
}

void n64_rdp::texture_rect(u64 cmd0, u64 cmd1)
{
	u32 tile = (cmd0>>24)&7;
	u32 lrX = ((cmd0>>44)&0xfff)>>2;
	u32 lrY = ((cmd0>>32)&0xfff)>>2;
	u32 ulX = ((cmd0>>12)&0xfff)>>2;
	u32 ulY = (cmd0&0xfff)>>2;
	s32 S = ((cmd1>>48)&0xffff); S <<= 16; S >>= 16; S <<= 5;
	s32 T = ((cmd1>>32)&0xffff); S <<= 16; T >>= 16; T <<= 5;
	s32 DsDx = ((cmd1>>16)&0xffff); DsDx <<= 16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff); DtDy <<= 16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	}
	
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, 16, Sl>>10, T>>10);
				if( other.alpha_compare_en && (__builtin_bswap16(sample)&1) )
				{
					*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = sample;
				}
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y <= lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = tex_sample(tile, 32, Sl>>10, T>>10);
			}
		}	
	}
}

void n64_rdp::texture_rect_flip(u64 cmd0, u64 cmd1)
{
	u32 tile = (cmd0>>24)&7;
	u32 lrX = ((cmd0>>44)&0xfff)>>2;
	u32 lrY = ((cmd0>>32)&0xfff)>>2;
	u32 ulX = ((cmd0>>12)&0xfff)>>2;
	u32 ulY = (cmd0&0xfff)>>2;
	s32 S = ((cmd1>>48)&0xffff); S <<= 16; S >>= 16; S <<= 5;
	s32 T = ((cmd1>>32)&0xffff); S <<= 16; T >>= 16; T <<= 5;
	s32 DsDx = ((cmd1>>16)&0xffff); DsDx <<= 16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff); DtDy <<= 16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	}
	
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, 16, T>>10, Sl>>10);
				//if( other.alpha_compare_en && (__builtin_bswap16(sample)&1) )
				{
					*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = sample;
				}
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y <= lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = tex_sample(tile, 32, T>>10, Sl>>10);
			}
		}	
	}	
	return;
}

void n64_rdp::set_tile_size(u64 cmd)
{
	auto& T = tiles[(cmd>>24)&7];
	
	u32 ulS = (cmd>>44)&0xfff;
	u32 ulT = (cmd>>32)&0xfff;
	u32 lrS = (cmd>>12)&0xfff;
	u32 lrT = cmd&0xfff;
	
	T.SL = ulS;
	T.SH = lrS;
	T.TL = ulT;
	T.TH = lrT;
}

u32 n64_rdp::tex_sample(u32 tile, u32 bpp, s32 s, s32 t)
{
	auto& T = tiles[tile];
	if( T.maskS ) s &= (1<<T.maskS)-1;
	if( T.maskT ) t &= (1<<T.maskT)-1;

	dc res;
	if( T.bpp == 16 )
	{
		res = dc::from16(__builtin_bswap16( *(u16*)&tmem[(T.addr*8 + (t*T.line*8) + s*2)&0xffe] ));
	} else if( T.bpp == 32 ) {
		//todo: put the rg/ba in the separate banks
		res = dc::from32( __builtin_bswap32( *(u32*)&tmem[(T.addr*8 + (t*T.line*8) + s*4)&0xfff] ));
	} else if( T.bpp == 8 ) {
		u8 v = tmem[T.addr*8 + (t*T.line*8) + s];
		res = dc::from32((v<<24)|(v<<16)|(v<<8)|v);
	}
	
	if( bpp == 32 ) return __builtin_bswap32(res.to32());
	return ( __builtin_bswap16(res.to16()) );
}


