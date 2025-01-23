#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "n64_rdp.h"

const u32 imgbpp[] = { 4, 8, 16, 32 };

void n64_rdp::recv(u64 cmd)
{
	u8 cmdbyte = 0;
	if( cmdbuf.empty() ) 
	{
		cmdbyte = (cmd>>56)&0x3F;
	} else {
		cmdbyte = (cmdbuf[0]>>56)&0x3F;
	}
	//printf("RDP Cmd $%X\n", u8((cmd>>56)&0x3F));
	switch( cmdbyte )
	{
	case 0x08:{ // basic non-anything flat triangle
		cmdbuf.push_back(cmd);
		if( cmdbuf.size() < 4 ) return;
		flat_triangle(cmdbuf[0], cmdbuf[1], cmdbuf[2], cmdbuf[3]);
		//if( cmdbyte & 4 ) i += 8; // shading
		//if( cmdbyte & 2 ) i += 8; // texturing
		//if( cmdbyte & 1 ) i += 2; // depth
		}break;
				
	case 0x24: // texture rectangle
		cmdbuf.push_back(cmd);
		if( cmdbuf.size() < 2 ) return;
		texture_rect(cmdbuf[0], cmdbuf[1]);
		break;
	case 0x25: // texture flipped rect
		cmdbuf.push_back(cmd);
		if( cmdbuf.size() < 2 ) return;
		texture_rect_flip(cmdbuf[0], cmdbuf[1]);
		break;
		
	case 0x2A: // Set Key GB
		//printf("set key gb = $%lX\n", cmd);
		break;
	case 0x2B: // Set Key R
		//printf("set key R = $%lX\n", cmd);
		break;
	case 0x2C: // set convert
		//printf("set conv = $%lX\n", cmd);
		break;
	
	case 0x2D: // scissor
		scissor.ulX = (cmd>>46)&0x3ff;
		scissor.ulY = (cmd>>34)&0x3ff;
		scissor.field = cmd & BITL(25);
		scissor.odd = cmd & BITL(24);
		scissor.lrX = (cmd>>14)&0x3ff;
		scissor.lrY = (cmd>>2)&0x3ff;
		break;
	case 0x2E: // primitive depth
		prim_z = cmd>>16;
		prim_delta_z = cmd;
		break;
	case 0x2F: // other modes
		other.cycle_type = (cmd>>52)&3;
		other.alpha_compare_en = cmd&1;
		other.force_blend = cmd&BITL(14);
		//fprintf(stderr, "set other modes = $%lX, ctype = %i\n", cmd, other.cycle_type);
		break;
		
	case 0x30: // Load TLUT
		load_tlut(cmd);
		break;
	case 0x33: // Load Block
		load_block(cmd);
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
		//fprintf(stderr, "combine = $%lX\n", cmd&0x00ffFFFFffffFFFFull);
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
	case 0x23: 
	case 0x31:
		break; // lots of nop
	
	case 0x26: // sync load
		break;
	case 0x27: // sync pipe
		break;
	case 0x28: // sync tile
		break;
	case 0x29: // sync full
		rdp_irq();
		break;
	default: printf("RDP: Unimpl cmd = $%X\n", u32(cmdbyte)); exit(1);
	}
	cmdbuf.clear();	
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
	//fprintf(stderr, "LT Tile%i Size = (%i, %i) - (%i, %i)\n\n", u8((cmd>>24)&7), ulS, ulT, lrS, lrT);
	
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
	if( T.bpp == 32 ) T.line <<= 1;
	//fprintf(stderr, "set tile%i, addr = $%X, line = $%X\n\tformat = $%X, bpp=$%X\n", 
	//	u8((cmd>>24)&7), T.addr, T.line, T.format, T.bpp);
	
	T.clampT = (cmd&BITL(19));
	T.mirrorT = (cmd&BITL(18));
	T.maskT = (cmd>>14)&0xf;
	T.shiftT = (cmd>>10)&0xf;
	T.clampS = cmd&BITL(9);
	T.mirrorS = cmd&BITL(8);
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
	s32 T = ((cmd1>>32)&0xffff); T <<= 16; T >>= 16; T <<= 5;
	s32 DsDx = ((cmd1>>16)&0xffff); DsDx <<= 16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff); DtDy <<= 16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	}
	
	//fprintf(stderr, "texrect t%i (%i, %i) to (%i, %i)\n\tst(%X, %X), d(%X,%X)\n\n", 
	//	other.cycle_type, ulX, ulY, lrX, lrY, S, T, DsDx, DtDy);
		
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, 16, Sl, T);
				if( other.alpha_compare_en && !(__builtin_bswap16(sample)&1) ) continue;
				if( other.force_blend && !(__builtin_bswap16(sample)&1)) continue;
				
				*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = sample;
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y <= lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u32 sample = tex_sample(tile, 32, Sl, T);
				if( other.alpha_compare_en && !(__builtin_bswap32(sample)&0xff) ) continue;
				if( other.force_blend && !(__builtin_bswap32(sample)&0xff) ) continue;
				
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = sample;
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
	s32 S = ((cmd1>>48)&0xffff)<<16; S >>= 16; S <<= 5;
	s32 T = ((cmd1>>32)&0xffff)<<16; T >>= 16; T <<= 5;
	s32 DsDx = ((cmd1>>16)&0xffff); DsDx <<= 16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff); DtDy <<= 16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	} else {
		//DtDy *= 4;
	}
	
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, 16, T, Sl);
				if( !other.alpha_compare_en || (other.alpha_compare_en && (__builtin_bswap16(sample)&1)) )
				{
					*(u16*)&rdram[(cimg.addr + (Y*cimg.width*2) + X*2)&0x7ffffe] = sample;
				}
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y <= lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				*(u32*)&rdram[(cimg.addr + (Y*cimg.width*4) + X*4)&0x7ffffc] = tex_sample(tile, 32, T, Sl);
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
	
	//fprintf(stderr, "Set Tile%i Size = (%i, %i) - (%i, %i)\n", u8((cmd>>24)&7), ulS, ulT, lrS, lrT);
}

u32 n64_rdp::tex_sample(u32 tile, u32 bpp, s32 s, s32 t)
{
	auto& T = tiles[tile];
	//s += 0x400;
	//t += 0x400;
	s >>= 10;
	t >>= 10;
	s -= T.SL>>2;
	t -= T.TL>>2;
	if( T.maskS ) s &= (1<<T.maskS)-1;
	if( T.maskT ) t &= (1<<T.maskT)-1;
	
	dc res;
	if( T.bpp == 16 )
	{
		res = dc::from16(__builtin_bswap16( *(u16*)&tmem[(T.addr*8 + (t*T.line*8) + s*2)&0xffe] ));
	} else if( T.bpp == 32 ) {
		//todo: put the rg/ba in the separate banks
		res = dc::from32( __builtin_bswap32( *(u32*)&tmem[(T.addr*8 + (t*T.line*8) + s*4)&0xffc] ));
	} else if( T.bpp == 8 ) {
		if( T.format == 2 )
		{
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + s)&0xfff];
			res = dc::from16(__builtin_bswap16(*(u16*)&tmem[(0x100+v)*8]));
		} else {
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + s)&0xfff];
			res = dc::from32((v<<24)|(v<<16)|(v<<8)|v);
		}
	} else if( T.bpp == 4 ) {
		if( T.format == 2 )
		{
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + (s>>1))&0xfff];
			v >>= (s&1)*4;
			v &= 15;
			res = dc::from16(__builtin_bswap16(*(u16*)&tmem[((0x100+T.palette*16+v)*8)&0xfff]));
		}
	}
	
	if( bpp == 32 ) return __builtin_bswap32(res.to32());
	return ( __builtin_bswap16(res.to16()) );
}

void n64_rdp::flat_triangle(u64 cmd0, u64 cmd1, u64 cmd2, u64 cmd3)
{
	//todo: actually use the settings to determine color outside fill mode
	u32 flat_color = (other.cycle_type == CYCLE_TYPE_FILL) ? fill_color : ((blend_color.to16()<<16)|blend_color.to16());
	bool left = cmd0 & BITL(55);
	s64 Y3 = s32(((cmd0>>32)&0x3FFF) << 19); Y3 >>= 5;
	s64 Y2 = s32(((cmd0>>16)&0x3FFF) << 19); Y2 >>= 5;
	s64 Y1 = s32((cmd0&0x3FFF) << 19); Y1 >>= 5;
	//printf("flattri %f, %f, %f\n", Y1/65536.f, Y2/65536.f, Y3/65536.f);
		
	s64 XL = s32((cmd1>>32)<<4); XL >>= 4;
	s64 ISL = s32((cmd1<<2)); ISL >>= 2;
	s64 XH = s32((cmd2>>32)<<4); XH >>= 4;
	s64 ISH = s32((cmd2<<2)); ISH >>= 2;
	s64 XM = s32((cmd3>>32)<<4); XM >>= 4;
	s64 ISM = s32((cmd3<<2)); ISM >>= 2;
	
	if( !left ) { std::swap(XH, XM); std::swap(ISH, ISM); }
	
	s64 X1 = XH;
	s64 X2 = XM;
	for(s64 Y = Y1>>16; Y <= Y3>>16; ++Y)
	{
		if( Y >= scissor.lrY ) return;
		if( Y >= scissor.ulY )
		{
			for(s64 X = X1>>16; X <= X2>>16; ++X)
			{
				if( X < scissor.ulX || X > scissor.lrX ) continue;
				if( cimg.bpp == 16 )
				{
					*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = __builtin_bswap16(u16(flat_color>>((X&1)*16)));
				} else {
					*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = __builtin_bswap32(flat_color);
				}
			}
		}
		X1 += ISH;
		X2 += ISM;
		if( Y == Y2>>16 ) { if( left ) { X2 = XL; ISM = ISL; } else { X1 = XL; ISH=ISL; } }
	}
}

void n64_rdp::load_block(u64 cmd)
{
	u32 dxt = cmd & 0xfff;
	u32 lrS = (cmd>>14)&0x3ff;
	auto& T = tiles[(cmd>>24)&7];
	u32 ulT = (cmd>>34)&0x3ff;
	u32 ulS = (cmd>>46)&0x3ff;
	T.SL = ulS<<2;
	T.TL = ulT<<2;
	u32 num_texels = lrS - ulS + 1;
	if( num_texels > 2048 ) 
	{
		printf("RDP: Num texels was >2048\n");
		exit(1);
		return;
	}
	u32 num_bytes = (num_texels*T.bpp)/8;
	u32 rdram_stride = ((teximg.width*T.bpp) + 7) / 8; 
	for(u32 i = 0; i < num_bytes; ++i)
	{
		tmem[(T.addr*8 + T.line*8 + i)&0xfff] = rdram[teximg.addr + (ulT*rdram_stride) + ((lrS*T.bpp+7)/8) + i];
	}
}

void n64_rdp::load_tlut(u64 cmd)
{
	auto& T = tiles[(cmd>>24)&7];
	u32 lrT = (cmd>>2) & 0x3ff;
	u32 lrS = (cmd>>14) & 0x3ff;
	u32 ulT = (cmd>>34) & 0x3ff;
	u32 ulS = (cmd>>46) & 0x3ff;
	
	if( ulT != lrT || ulT != 0 || lrT != 0 )
	{
		printf("RDP: LoadTLUT with ulT != lrT != both 0\n");
		//exit(1);
	}
	
	u32 num = (lrS - ulS + 1);
	for(u32 i = 0; i < num; ++i)
	{
		u64 p = (*(u16*)&rdram[teximg.addr + i*2]);
		p = (p<<48)|(p<<32)|(p<<16)|p;
		*(u64*)&tmem[(T.addr*8 + i*8)&0xfff] = p;
	}
}

