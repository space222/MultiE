#include <print>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "n64_rdp.h"

const u32 imgbpp[] = { 4, 8, 16, 32 };

const n64_rdp::dc white(0xff, 0xff, 0xff, 0xff);

#define field(f, d, a) (((f)>>(d))&(a))

void n64_rdp::recv(u64 cmd)
{
	//u8 cmdbyte = 0;
	/*if( cmdbuf.empty() ) 
	{
		cmdbyte = (cmd>>56)&0x3F;
	} else {
		cmdbyte = (cmdbuf[0]>>56)&0x3F;
	}*/
	cmdbuf.push_back(cmd);
	u8 cmdbyte = (cmdbuf[0]>>56)&0x3F;
	//printf("RDP Cmd $%X\n", u8((cmd>>56)&0x3F));
	switch( cmdbyte )
	{
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		{ // basic non-anything flat triangle
		if( cmdbuf.size() < u32(4 + ((cmdbyte&4)?8:0) + ((cmdbyte&2)?8:0) + ((cmdbyte&1)?2:0) ) ) return;
		RS.cmd = cmdbyte;
		RS.y3 = s32(((cmdbuf[0]>>32)&0x3FFF) << 19); RS.y3 >>= 5;
		RS.y2 = s32(((cmdbuf[0]>>16)&0x3FFF) << 19); RS.y2 >>= 5;
		RS.y1 = s32((cmdbuf[0]&0x3FFF) << 19); RS.y1 >>= 5;
		RS.xl = s32((cmdbuf[1]>>32)<<4); RS.xl >>= 4;
		RS.xm = s32((cmdbuf[3]>>32)<<4); RS.xm >>= 4;
		RS.xh = s32((cmdbuf[2]>>32)<<4); RS.xh >>= 4;
		RS.DxlDy = s32((cmdbuf[1]<<2)); RS.DxlDy >>= 2;
		RS.DxhDy = s32((cmdbuf[2]<<2)); RS.DxhDy >>= 2;
		RS.DxmDy = s32((cmdbuf[3]<<2)); RS.DxmDy >>= 2;
		TX.tile = (cmdbuf[0]>>48)&7;
		u32 i = 4;
		
		if( cmdbyte & 4 )
		{ // grab shade attributes
			RS.r = (field(cmdbuf[i], 48, 0xffff)<<16)|field(cmdbuf[i+2], 48, 0xffff);
			RS.g = (field(cmdbuf[i], 32, 0xffff)<<16)|field(cmdbuf[i+2], 32, 0xffff);
			RS.b = (field(cmdbuf[i], 16, 0xffff)<<16)|field(cmdbuf[i+2], 16, 0xffff);
			RS.a = (field(cmdbuf[i], 0, 0xffff)<<16)|field(cmdbuf[i+2], 0, 0xffff);
			//std::println("r{:X}, g{:X}, b{:X}", RS.r, RS.g, RS.b);
			RS.DrDx = s32(field(cmdbuf[i+1], 48, 0xffff)<<16)|field(cmdbuf[i+3], 48, 0xffff);
			RS.DgDx = s32(field(cmdbuf[i+1], 32, 0xffff)<<16)|field(cmdbuf[i+3], 32, 0xffff);
			RS.DbDx = s32(field(cmdbuf[i+1], 16, 0xffff)<<16)|field(cmdbuf[i+3], 16, 0xffff);
			RS.DaDx = s32(field(cmdbuf[i+1], 0, 0xffff)<<16)|field(cmdbuf[i+3], 0, 0xffff);
			RS.DrDe = s32(field(cmdbuf[i+4], 48, 0xffff)<<16)|field(cmdbuf[i+6], 48, 0xffff);
			RS.DgDe = s32(field(cmdbuf[i+4], 32, 0xffff)<<16)|field(cmdbuf[i+6], 32, 0xffff);
			RS.DbDe = s32(field(cmdbuf[i+4], 16, 0xffff)<<16)|field(cmdbuf[i+6], 16, 0xffff);
			RS.DaDe = s32(field(cmdbuf[i+4], 0, 0xffff)<<16)|field(cmdbuf[i+6], 0, 0xffff);
			RS.DrDy = s32(field(cmdbuf[i+5], 48, 0xffff)<<16)|field(cmdbuf[i+7], 48, 0xffff);
			RS.DgDy = s32(field(cmdbuf[i+5], 32, 0xffff)<<16)|field(cmdbuf[i+7], 32, 0xffff);
			RS.DbDy = s32(field(cmdbuf[i+5], 16, 0xffff)<<16)|field(cmdbuf[i+7], 16, 0xffff);
			RS.DaDy = s32(field(cmdbuf[i+5], 0, 0xffff)<<16)|field(cmdbuf[i+7], 0, 0xffff);	
			i += 8;
		}
		if( cmdbyte & 2 )
		{  // grab tex attributes
			RS.s = s32(field(cmdbuf[i], 48, 0xffff)<<16)|field(cmdbuf[i+2], 48, 0xffff);
			RS.t = s32(field(cmdbuf[i], 32, 0xffff)<<16)|field(cmdbuf[i+2], 32, 0xffff);
			RS.w = s32(field(cmdbuf[i], 16, 0xffff)<<16)|field(cmdbuf[i+2], 16, 0xffff);
			RS.DsDx =s32(field(cmdbuf[i+1],48,0xffff)<<16)|field(cmdbuf[i+3],48,0xffff);
			RS.DtDx =s32(field(cmdbuf[i+1],32,0xffff)<<16)|field(cmdbuf[i+3],32,0xffff);
			RS.DwDx =s32(field(cmdbuf[i+1],16,0xffff)<<16)|field(cmdbuf[i+3],16,0xffff);
			RS.DsDe =s32(field(cmdbuf[i+4],48,0xffff)<<16)|field(cmdbuf[i+6],48,0xffff);
			RS.DtDe =s32(field(cmdbuf[i+4],32,0xffff)<<16)|field(cmdbuf[i+6],32,0xffff);
			RS.DwDe =s32(field(cmdbuf[i+4],16,0xffff)<<16)|field(cmdbuf[i+6],16,0xffff);
			RS.DsDy =s32(field(cmdbuf[i+5],48,0xffff)<<16)|field(cmdbuf[i+7],48,0xffff);
			RS.DtDy =s32(field(cmdbuf[i+5],32,0xffff)<<16)|field(cmdbuf[i+7],32,0xffff);
			RS.DwDy =s32(field(cmdbuf[i+5],16,0xffff)<<16)|field(cmdbuf[i+7],16,0xffff);
			i += 8;
		}
		if( cmdbyte & 1 )
		{ // grab z attributes
			RS.z = s32(cmdbuf[i]>>32);
			RS.DzDx = s32(cmdbuf[i]);
			RS.DzDe = s32(cmdbuf[i+1]>>32);
			RS.DzDy = s32(cmdbuf[i+1]);
		}
		triangle();
		}break;
				
	case 0x24: // texture rectangle
		if( cmdbuf.size() < 2 ) return;
		texture_rect(cmdbuf[0], cmdbuf[1]);
		break;
	case 0x25: // texture flipped rect
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
		other.z_compare = cmd&BITL(4);
		other.z_write = cmd&BITL(5);
		other.perspective = cmd&BITL(51);
		other.tlut_type_ia16 = cmd&BITL(46);
		
		BL.p[0] = field(cmd, 30, 3);
		BL.p[1] = field(cmd, 28, 3);
		BL.a[0] = field(cmd, 26, 3);
		BL.a[1] = field(cmd, 24, 3);
		BL.m[0] = field(cmd, 22, 3);
		BL.m[1] = field(cmd, 20, 3);
		BL.b[0] = field(cmd, 18, 3);
		BL.b[1] = field(cmd, 16, 3);
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
		prim_color = dc::from32(cmd);
		break;
	case 0x3B: // environment color
		env_color = dc::from32(cmd);
		break;	
	case 0x3C: // set combine mode
		//fprintf(stderr, "combine = $%lX\n", cmd&0x00ffFFFFffffFFFFull);
		CC.alpha_d[1] = cmd&7;
		CC.alpha_b[1] = field(cmd, 3, 7);
		CC.rgb_d[1] = field(cmd, 6, 7);
		CC.alpha_d[0] = field(cmd, 9, 7);
		CC.alpha_b[0] = field(cmd, 12, 7);
		CC.rgb_d[0] = field(cmd, 15, 7);
		CC.alpha_c[1] = field(cmd, 18, 7);
		CC.alpha_a[1] = field(cmd, 21, 7);
		CC.rgb_b[1] = field(cmd, 24, 15);
		CC.rgb_b[0] = field(cmd, 28, 15);
		CC.rgb_c[1] = field(cmd, 32, 31);
		CC.rgb_a[1] = field(cmd, 37, 15);
		CC.alpha_c[0] = field(cmd, 41, 7);
		CC.alpha_a[0] = field(cmd, 44, 7);
		CC.rgb_c[0] = field(cmd, 47, 31);
		CC.rgb_a[0] = field(cmd, 52, 15);
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
	
	if( T.bpp == 32 )
	{
		//todo: but currently have a barely functional hack of increasing
		//	line size*2 for 32bpp in Set Tile.
	}
	
	u32 linesize = T.line*8;
	
	u32 roffs = (((ulS*T.bpp)+7)/8) + teximg.addr;
	u32 rdram_stride = ((teximg.width*T.bpp) + 7) / 8; 

	for(u32 Y = ulT; Y <= lrT; ++Y)
	{
		memcpy(tmem+(T.addr*8+((Y-ulT)*linesize)), rdram+roffs+(Y*rdram_stride), rdram_stride);
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
	if( T.bpp == 32 ) 
	{	//todo: remove once load tile and sampling can handle the 32bpp rg/ba split bank layout
		T.line <<= 1;
	}
	//fprintf(stderr, "set tile%i, addr = $%X, line = $%X\n\tformat = $%X, bpp=$%X\n", 
	//	u8((cmd>>24)&7), T.addr, T.line, T.format, T.bpp);
	
	T.clampT = (cmd&BITL(19));
	T.mirrorT = (cmd&BITL(18));
	T.maskT = (1u<<((cmd>>14)&0xf))-1;
	T.shiftT = (cmd>>10)&0xf;
	T.clampS = cmd&BITL(9);
	T.mirrorS = cmd&BITL(8);
	T.maskS = (1u<<((cmd>>4)&0xf))-1;
	T.shiftS = cmd&0xf;
	
	//if( T.clampT || T.mirrorT || T.clampS || T.mirrorS )
	//	fprintf(stderr, "cT = %i, mT = %i, cS = %i, mS = %i\n", !!T.clampT, !!T.mirrorT, !!T.clampS, !!T.mirrorS);
	//if( T.shiftS || T.shiftT )
	//	fprintf(stderr, "shiftS = %X, shiftT = %X\n", T.shiftS, T.shiftT);
}

void n64_rdp::texture_rect(u64 cmd0, u64 cmd1)
{
	u32 tile = (cmd0>>24)&7;
	u32 lrX = ((cmd0>>44)&0xfff)>>2;
	u32 lrY = ((cmd0>>32)&0xfff)>>2;
	u32 ulX = ((cmd0>>12)&0xfff)>>2;
	u32 ulY = (cmd0&0xfff)>>2;
	s32 S = ((cmd1>>48)&0xffff)<<16; S >>= 16; S <<= 5;
	s32 T = ((cmd1>>32)&0xffff)<<16; T >>= 16; T <<= 5;
	s32 DsDx = ((cmd1>>16)&0xffff)<<16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff)<<16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		if( tiles[tile].bpp != 16 ) 
		{
			//printf("RDP: COPY mode in bpp %i\n", tiles[tile].bpp);
			//exit(1);
		}
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	} else {
		//lrY += 1;
		//lrX += 1;
	}
	
	//fprintf(stderr, "texrect t%i (%i, %i) to (%i, %i)\n\tst(%X, %X), d(%X,%X)\n\n", 
	//	other.cycle_type, ulX, ulY, lrX, lrY, S>>10, T>>10, DsDx, DtDy);
		
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, Sl>>10, T>>10).to16();
				if( other.alpha_compare_en && !(sample&1) ) continue;
				if( other.force_blend && !(sample&1) ) continue;
				
				*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = __builtin_bswap16(sample);
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u32 sample = tex_sample(tile, Sl>>10, T>>10).to32();
				if( other.alpha_compare_en && !(sample&0xff) ) continue;
				if( other.force_blend && !(sample&0xff) ) continue;
				
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = __builtin_bswap32(sample);
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
	s32 DsDx = ((cmd1>>16)&0xffff)<<16; DsDx >>= 16;
	s32 DtDy = (cmd1&0xffff)<<16; DtDy >>= 16;
	if( other.cycle_type == CYCLE_TYPE_COPY ) 
	{	// convert copy mode to 1/2cycle mode values.
		DsDx /= 4;  // probably need other values for when bpp != 16
		lrY += 1;
		lrX += 1;
	} else {
		//lrY += 1;
		//lrX += 1;
	}
	
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u16 sample = tex_sample(tile, T>>10, Sl>>10).to16();
				if( other.alpha_compare_en && !(sample&1) ) continue;
				if( other.force_blend && !(sample&1) ) continue;
				
				*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = __builtin_bswap16(sample);
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				u32 sample = tex_sample(tile, T>>10, Sl>>10).to32();
				if( other.alpha_compare_en && !(sample&0xff) ) continue;
				if( other.force_blend && !(sample&0xff) ) continue;
				
				*(u32*)&rdram[(cimg.addr + (Y*cimg.width*4) + X*4)&0x7ffffc] = __builtin_bswap32(sample);
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

n64_rdp::dc n64_rdp::tex_sample(u32 tile, s64 s, s64 t)
{
	auto& T = tiles[tile];
	
	/*
	s -= T.SL>>2;
	t -= T.TL>>2;

	if( s < 0 ) { s = ~s; }
	if( t < 0 ) { t = ~t; }	


	if( T.shiftS < 11 ) 
	{
		s >>= T.shiftS;
	} else {
		s <<= 16-T.shiftS;
	}
	if( T.shiftT < 11 ) 
	{
		t >>= T.shiftT;
	} else {
		t <<= 16-T.shiftT;
	}
	
	if( T.mirrorS )
	{
		u32 mbit = s & (T.maskS+1);
		s &= T.maskS;
		if( mbit ) s ^= T.maskS;
	} else if( T.maskS ) {
		s &= T.maskS;
	}
	if( T.mirrorT )
	{
		u32 mbit = t & (T.maskT+1);
		t &= T.maskT;
		if( mbit ) t ^= T.maskT;
	} else if( T.maskT ) {
		t &= T.maskT;
	}

	if( s > (T.SH>>2) ) 
	{
		if( T.clampS ) 
		{
			s = T.SH>>2;
		} else {
			if( T.SH>>2 ) s %= T.SH>>2;
			else s = 0;
		}
	}
	if( t > (T.TH>>2) ) 
	{
		if( T.clampT ) 
		{
			t = T.TH>>2;
		} else {
			if( T.TH>>2 ) t %= T.TH>>2;
			else t = 0;
		}		
	}
	*/
	s -= T.SL>>2;
	t -= T.TL>>2;
	
	   // Clamp, mirror, or mask the S-coordinate based on tile settings
	if( T.clampS ) s = std::max<s64>(std::min<s64>(s, ((T.SH>>2) - (T.SL>>2))), 0);
	if( T.mirrorS && (s & (T.maskS+1)) ) s = ~s;
	s &= T.maskS;

	// Clamp, mirror, or mask the T-coordinate based on tile settings
	if( T.clampT ) t = std::max<s64>(std::min<s64>(t, ((T.TH>>2) - (T.TL>>2))), 0);
	if( T.mirrorT && (t & (T.maskT+1)) ) t = ~t;
	t &= T.maskT;
	
	if( T.bpp == 16 ) 
	{
		return dc::from16(__builtin_bswap16( *(u16*)&tmem[(T.addr*8 + (t*T.line*8) + s*2)&0xffe] ));
	} else if( T.bpp == 32 ) {
		//todo: put the rg/ba in the separate banks
		return dc::from32( __builtin_bswap32( *(u32*)&tmem[(T.addr*8 + (t*T.line*8) + s*4)&0xffc] ));
	} else if( T.bpp == 8 ) {
		if( T.format == 2 )
		{
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + s)&0xfff];
			return dc::from16(__builtin_bswap16(*(u16*)&tmem[(0x100+v)*8]));
		} else {
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + s)&0xfff];
			return dc::from32((v<<24)|(v<<16)|(v<<8)|v);
		}
	} else if( T.bpp == 4 ) {
		if( T.format == 2 )
		{
			u8 v = tmem[(T.addr*8 + (t*T.line*8) + (s>>1))&0xfff];
			v >>= ((s^1)&1)*4;
			v &= 15;
			return dc::from16(__builtin_bswap16(*(u16*)&tmem[((0x100+T.palette*16+v)*8)&0xfff]));
		}
	}
	
	if( !(T.bpp == 4 && T.format == 3) && !(T.bpp == 4 && T.format == 4) )
	{
		std::println(stderr, "tex_sample, tile bpp{}, format{} unsupported.", T.bpp, T.format);
	}
	return dc::from32(0);
}

#define ATTR_YINC RS.r += RS.DrDe;  \
		 RS.g += RS.DgDe;  \
		 RS.b += RS.DbDe;  \
		 RS.a += RS.DaDe;  \
		 RS.s += (RS.DsDe);  \
		 RS.t += (RS.DtDe); \
		 RS.w += (RS.DwDe); \
		 RS.z += RS.DzDe
		 
#define ATTR_XDEC r -= RS.DrDx;  \
		  g -= RS.DgDx;  \
		  b -= RS.DbDx;  \
		  a -= RS.DaDx;  \
		  s -= (RS.DsDx); \
		  t -= (RS.DtDx); \
		  w -= (RS.DwDx); \
		  z -= RS.DzDx
		  
#define ATTR_XINC r += RS.DrDx;  \
		  g += RS.DgDx;  \
		  b += RS.DbDx;  \
		  a += RS.DaDx;  \
		  s += (RS.DsDx); \
		  t += (RS.DtDx); \
		  w += (RS.DwDx); \
		  z += RS.DzDx
		  
#define PERSP  int W = ( other.perspective ) ? (int(w)>>15) : 0x10000; \
	S /= (W ? W : 0x10000); \
	T /= (W ? W : 0x10000); \


void n64_rdp::triangle()
{
	const bool right = (cmdbuf[0] & BITL(55));
	RS.shade_color = white;
		
	if( !right )
	{
		for(s64 y = RS.y1>>16; y < RS.y2>>16 && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= 0 )
			for(s64 x = RS.xh>>16; x >= RS.xm>>16; --x)
			{
				if( x < 0 || x > scissor.lrX ) break;
				RS.cx = x;
				RS.cy = y;
				u16 Z = z>>16;//1/(z/65536.f);
				if( other.z_compare && Z > *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] ) continue;
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s; s64 T = t;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				color_combiner();
				if( !blender() ) continue;
				if( cimg.bpp == 16 )
				{
					if( other.z_write) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = Z;
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
				//*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(col.to32());
				}
				ATTR_XDEC;
			}
			RS.xh += RS.DxhDy;
			RS.xm += RS.DxmDy;
			ATTR_YINC;
		}
		for(s64 y = RS.y2>>16; y < RS.y3>>16 && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= 0 )
			for(s64 x = RS.xh>>16; x >= RS.xl>>16; --x)
			{
				if( x < 0 || x > scissor.lrX ) break;
				RS.cx = x;
				RS.cy = y;
				u16 Z = z>>16;//1/(z/65536.f);
				if( other.z_compare && Z > *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] ) continue;
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s; s64 T = t;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				color_combiner();
				if( !blender() ) continue;
				if( cimg.bpp == 16 )
				{
					if( other.z_write ) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = Z;
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
				//*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(col.to32());
				}
				ATTR_XDEC;
			}
			RS.xh += RS.DxhDy;
			RS.xl += RS.DxlDy;
			ATTR_YINC;
		}	
	} else {
		for(s64 y = RS.y1>>16; y < RS.y2>>16 && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= 0 )
			for(s64 x = RS.xh>>16; x <= RS.xm>>16; ++x)
			{
				if( x < 0 || x > scissor.lrX ) break;
				RS.cx = x;
				RS.cy = y;
				u16 Z = z>>16;//1/(z/65536.f);
				if( other.z_compare && Z > *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] ) continue;
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s; s64 T = t;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				color_combiner();
				if( !blender() ) continue;
				if( cimg.bpp == 16 )
				{
					if( other.z_write) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = Z;
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
				//*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(col.to32());
				}
				ATTR_XINC;
			}
			RS.xh += RS.DxhDy;
			RS.xm += RS.DxmDy;
			ATTR_YINC;
		}
		for(s64 y = RS.y2>>16; y < RS.y3>>16 && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= 0 )
			for(s64 x = RS.xh>>16; x <= RS.xl>>16; ++x)
			{
				if( x < 0 || x > scissor.lrX ) break;
				RS.cx = x;
				RS.cy = y;
				u16 Z = z>>16;//1/(z/65536.f);
				if( other.z_compare && Z > *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] ) continue;
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s; s64 T = t;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				color_combiner();
				if( !blender() ) continue;
				if( cimg.bpp == 16 )
				{
					if( other.z_write) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = Z;
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
				//*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(col.to32());
				}
				ATTR_XINC;
			}
			RS.xh += RS.DxhDy;
			RS.xl += RS.DxlDy;
			ATTR_YINC;
		}
	}
}

void n64_rdp::load_block(u64 cmd)
{
	u32 dxt = cmd & 0xfff;
	u32 lrS = (cmd>>14)&0x3ff;
	auto& T = tiles[(cmd>>24)&7];
	u32 ulT = (cmd>>34)&0x3ff;
	u32 ulS = (cmd>>46)&0x3ff;
	//T.SL = ulS<<2;
	//T.TL = ulT<<2;
	u32 num_texels = lrS - ulS + 1;
	if( num_texels > 2048 ) 
	{
		printf("RDP: Num texels was >2048\n");
		exit(1);
		return;
	}
	if( T.bpp == 32 )
	{
		//printf("32bit load block\n");
		//exit(1);
	}
	if( ulS != 0 )
	{
		printf("ulT was $%X\n", ulT);
		exit(1);
	}
	//fprintf(stderr, "load_block: $%X, $%X, $%X, %ibpp, num_texels = $%X\n", ulS, ulT, lrS, T.bpp, num_texels);
	u32 num_bytes = num_texels*8; // TIMES EIGHT?? WTF. yep, times8 makes both mario, starfox, and lcars work
	u32 swpcnt = dxt;
	u32 ramoff = teximg.addr;// + ((ulS*T.bpp+7)/8);// + ((ulT*teximg.width*T.bpp+7)/8);
	u32 t = 0;
	u32 taddr = (T.addr + T.line*t)*8;
	for(u32 i = 0; i < ((num_bytes+7)/8); ++i, taddr+=8, ramoff+=8)
	{
		u64 dw = *(u64*)&rdram[ramoff];
		//if( swpcnt & BIT(11) ) dw = (dw<<32)|(dw>>32);
		*(u64*)&tmem[taddr&0xfff] = dw;
		swpcnt += dxt;
		//if( swpcnt & BIT(11) ) { ulT+=1; t += 1; }
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

n64_rdp::dc n64_rdp::cc_a(u32 cycle)
{
	switch( CC.rgb_a[cycle] )
	{
	case 0: return CC.out;
	case 1: return TX.tex_sample;
	case 2: return white; //todo: tex1
	case 3: return prim_color;
	case 4: return RS.shade_color;
	case 5: return env_color;
	case 6: return white;
	case 7: return white; //todo: noise
	default: break;
	}
	return dc(0,0,0,0);
}

n64_rdp::dc n64_rdp::cc_d(u32 cycle)
{
	switch( CC.rgb_d[cycle] )
	{
	case 0: return CC.out;
	case 1: return TX.tex_sample;
	case 2: return white; //todo: tex1
	case 3: return prim_color;
	case 4: return RS.shade_color;
	case 5: return env_color;
	case 6: return white;
	default: break;
	}
	return dc(0,0,0,0);
}

n64_rdp::dc n64_rdp::cc_b(u32 cycle)
{
	switch( CC.rgb_b[cycle] )
	{
	case 0: return CC.out;
	case 1: return TX.tex_sample;
	case 2: return white; //todo: tex1
	case 3: return prim_color;
	case 4: return RS.shade_color;
	case 5: return env_color;
	case 6: return white;
	default: break;
	}
	return dc(0,0,0,0);
}
n64_rdp::dc n64_rdp::cc_c(u32 cycle)
{
	switch( CC.rgb_c[cycle] )
	{
	case 0: return CC.out;
	case 1: return TX.tex_sample;
	case 2: return white; //todo: tex1
	case 3: return prim_color;
	case 4: return RS.shade_color;
	case 5: return env_color;
	case 6: return white;
	default: break;
	}
	return dc(0,0,0,0);
}

float n64_rdp::cc_alpha(u32 sel)
{
	switch( sel )
	{
	case 0: return CC.out.a/255.f;
	case 1: return TX.tex_sample.a/255.f;
	case 2: return 1; //todo: tex1
	case 3: return prim_color.a/255.f;
	case 4: return RS.shade_color.a/255.f;
	case 5: return env_color.a/255.f;
	case 6: return 1;
	default: break;
	}
	return 0;
}

void n64_rdp::color_combiner()
{
	//todo: only supporting 1cycle for now
	//	normally 1cycle uses the <em>second</em> cycle settings for the combiner
	//	but <em>first</em> cycle settings for the blender, so the blender is "correct"ish, but
	//	leaving the 1cycle mode incorrectly using first cycle settings makes games work for now
	//	Mario uses 2cycle in levels and Wave Race starts using in the intro.
	//	without this hack, Bob-omb Battlefield is blue and Wave Race land masses are pink.
	dc res;
	{
		res = cc_a(0);
		dc b = cc_b(0);
		dc c = cc_c(0);
		dc d = cc_d(0);
		res -= b;
		res *= c;
		res += d;
	}
	
	float a = cc_alpha(CC.alpha_a[0]);
	float b = cc_alpha(CC.alpha_b[0]);
	float c = cc_alpha(CC.alpha_c[0]);
	float d = cc_alpha(CC.alpha_d[0]);
	res.a = ((a - b) * c + d)*255;
	CC.out = res;
}

bool n64_rdp::blender()
{
	if( !other.force_blend ) 
	{
		BL.out = CC.out;
		return true;
	}
	
	dc P, M;
	switch( BL.p[0] )
	{
	case 0: P = CC.out; break;
	case 1: P = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
				    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
				    break;
	case 2: P = blend_color; break;
	case 3: P = fog_color; break;
	}
	switch( BL.m[0] )
	{
	case 0: M = CC.out; break;
	case 1: M = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
				    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
				    break;
	case 2: M = blend_color; break;
	case 3: M = fog_color; break;
	}
	float A=1, B=1;
	switch( BL.a[0] )
	{
	case 0: A = CC.out.a/255.f; break;
	case 1: A = fog_color.a/255.f; break;
	case 2: A = RS.shade_color.a/255.f; break;
	case 3: A = 0; break;
	}
	switch( BL.b[0] )
	{
	case 0: B = 1.f - A; break;
	case 1: B = 1; break;  //todo: coverage
	case 2: B = 1; break;
	case 3: B = 0; break;
	}
	
	BL.out = P;
	BL.out *= A;
	M *= B;
	BL.out += M;
	return true;
}






