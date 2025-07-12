#include <print>
#include <bit>
#include <string>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <cstdlib>
#include "n64_rdp.h"

const u32 imgbpp[] = { 4, 8, 16, 32 };

const n64_rdp::dc white(0xff, 0xff, 0xff, 0xff);

#define field(f, d, a) (((f)>>(d))&(a))

void n64_rdp::run()
{
	//std::unique_lock gg(dplock);
	//if( cmdbuf.empty() ) return;
	
	u8 cmdbyte = (cmdbuf[0]>>56)&0x3F;
	u64 cmd = cmdbuf[0];
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
		{ // all the triangles
		u32 cmd_size =  u32(4 + ((cmdbyte&4)?8:0) + ((cmdbyte&2)?8:0) + ((cmdbyte&1)?2:0) );
		if( cmdbuf.size() < cmd_size ) return;
		RS.cmd = cmdbyte;
		// give everything 16 fractional bits
		RS.y3 = s32(((cmdbuf[0]>>32)&0x3FFF) << 18); RS.y3 >>= 4;
		RS.y2 = s32(((cmdbuf[0]>>16)&0x3FFF) << 18); RS.y2 >>= 4;
		RS.y1 = s32((cmdbuf[0]&0x3FFF) << 18); RS.y1 >>= 4;
		RS.xl = s32((cmdbuf[1]>>32)<<4); RS.xl >>= 4;
		RS.xh = s32((cmdbuf[2]>>32)<<4); RS.xh >>= 4;
		RS.xm = s32((cmdbuf[3]>>32)<<4); RS.xm >>= 4;
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
		cmdbuf.clear();
		//for(u32 nn = 0; nn < cmd_size; ++nn) cmdbuf.pop_front();	
		//gg.unlock();
		triangle();
		}return;
				
	case 0x24:{ // texture rectangle
		if( cmdbuf.size() < 2 ) return;
		u64 a = cmdbuf[0];
		u64 b = cmdbuf[1];
		//cmdbuf.pop_front();
		//cmdbuf.pop_front();	
		//gg.unlock();
		texture_rect(a, b);
		//}return;
		}break;
	case 0x25:{ // texture flipped rect
		if( cmdbuf.size() < 2 ) return;
		u64 a = cmdbuf[0];
		u64 b = cmdbuf[1];
		//cmdbuf.pop_front();
		//cmdbuf.pop_front();	
		//gg.unlock();
		texture_rect_flip(a, b);
		//}return;
		}break;
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
		other.cov_x_alpha = cmd&BITL(12);
		other.z_func = field(cmd, 10, 3);
		
		BL.p[0] = field(cmd, 30, 3);
		BL.p[1] = field(cmd, 28, 3);
		BL.a[0] = field(cmd, 26, 3);
		BL.a[1] = field(cmd, 24, 3);
		BL.m[0] = field(cmd, 22, 3);
		BL.m[1] = field(cmd, 20, 3);
		BL.b[0] = field(cmd, 18, 3);
		BL.b[1] = field(cmd, 16, 3);
		//fprintf(stderr, "set other modes = $%lX, zcomp = %i\n", cmd, other.z_func);
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
		teximg.size = ((cmd>>51)&3);
		teximg.bpp = imgbpp[teximg.size];
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

void n64_rdp::recv(u64 cmd)
{
	cmdbuf.push_back(cmd);
	run();
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
	uly = std::max(uly, scissor.ulY);
	ulx = std::max(ulx, scissor.ulX);
	lry = std::min(lry, scissor.lrY);
	lrx = std::min(lrx, scissor.lrX);
	//printf("fill rect (%i, %i) to (%i, %i)\n", ulx, uly, lrx, lry);
	if( cimg.bpp == 16 )
	{
		for(u32 y = uly; y < lry; ++y)
		{
			for(u32 x = ulx; x < lrx; ++x)
			{
				u16 fc = (x&1) ? (fill_color&0xffff) : (fill_color>>16);
				*(u16*)&rdram[cimg.addr + (y*cimg.width*2) + x*2] = __builtin_bswap16(fc);
				*(u16*)&b9[cimg.addr + (y*cimg.width*2) + x*2] = 0;
			}
		}
	} else {
		for(u32 y = uly; y < lry; ++y)
		{
			for(u32 x = ulx; x < lrx; ++x)
			{
				*(u32*)&rdram[cimg.addr + (y*cimg.width*4) + x*4] = __builtin_bswap32(fill_color);
				*(u32*)&b9[cimg.addr + (y*cimg.width*4) + x*4] = 0;
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
		for(u32 i = 0; i < rdram_stride; ++i) 
		{
			tmem[((i+T.addr*8+((Y-ulT)*linesize))&0xfff)^((Y&1)?4:0)] = rdram[(i+roffs+(Y*rdram_stride))&0x7fffff];
		}
		//memcpy(tmem+(T.addr*8+((Y-ulT)*linesize)), rdram+roffs+(Y*rdram_stride), rdram_stride);
	}
}

void n64_rdp::set_tile(u64 cmd)
{
	auto& T = tiles[(cmd>>24)&7];
	T.addr = (cmd>>32)&0x1ff;
	T.palette = (cmd>>20)&0xf;
	T.format = (cmd>>53)&7;
	if( T.format > 4 ) T.format = 4;
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
	if( T.maskT == 0xffffFFFFu ) T.maskT = 0;
	T.shiftT = (cmd>>10)&0xf;
	T.clampS = cmd&BITL(9);
	T.mirrorS = cmd&BITL(8);
	T.maskS = (1u<<((cmd>>4)&0xf))-1;
	if( T.maskS == 0xffffFFFFu ) T.maskS = 0;
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
	
	lrX = std::min(lrX, scissor.lrX);
	lrY = std::min(lrY, scissor.lrY);

	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			if( Y < scissor.ulY ) continue;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				if( X < scissor.ulX ) continue;
				TX.tex_sample = tex_sample(tile, Sl>>10, T>>10);
				if( other.cycle_type != CYCLE_TYPE_COPY )
				{
					color_combiner();
					if( !blender() ) continue;
				} else {
					BL.out = TX.tex_sample;
					if( other.alpha_compare_en && !(BL.out.to16()&1) ) continue;
				}
				u16 p = BL.out.to16();
				//if( other.alpha_compare_en && !(p&1) ) continue;
				//if( other.force_blend && !(p&1) ) continue;
				*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = __builtin_bswap16(p);	
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			if( Y < scissor.ulY ) continue;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				if( X < scissor.ulX ) continue;
				TX.tex_sample = tex_sample(tile, Sl>>10, T>>10);
				if( other.cycle_type != CYCLE_TYPE_COPY )
				{
					color_combiner();
					if( !blender() ) continue;
				} else {
					BL.out = TX.tex_sample;
					if( other.alpha_compare_en && !(BL.out.to16()&1) ) continue;
				}
				u32 p = BL.out.to32();
				//if( other.alpha_compare_en && !(p&1) ) continue;
				//if( other.force_blend && !(p&1) ) continue;
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = __builtin_bswap32(p);
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

	lrX = std::min(lrX, scissor.lrX);
	lrY = std::min(lrY, scissor.lrY);
	
	if( cimg.bpp == 16 )
	{
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			if( Y < scissor.ulY ) continue;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				if( X < scissor.ulX ) continue;
				TX.tex_sample = tex_sample(tile, T>>10, Sl>>10);
				if( other.cycle_type != CYCLE_TYPE_COPY )
				{
					color_combiner();
					if( !blender() ) continue;
				} else {
					BL.out = TX.tex_sample;
					if( other.alpha_compare_en && !(BL.out.to16()&1) ) continue;
				}
				u16 p = BL.out.to16();
				//if( other.alpha_compare_en && !(p&1) ) continue;
				//if( other.force_blend && !(p&1) ) continue;
				*(u16*)&rdram[cimg.addr + (Y*cimg.width*2) + X*2] = __builtin_bswap16(p);
			}
		}	
	} else if( cimg.bpp == 32 ) {
		for(u32 Y = ulY; Y < lrY; ++Y, T += DtDy)
		{
			s32 Sl = S;
			if( Y < scissor.ulY ) continue;
			for(u32 X = ulX; X < lrX; ++X, Sl += DsDx)
			{
				if( X < scissor.ulX ) continue;
				TX.tex_sample = tex_sample(tile, T>>10, Sl>>10);
				if( other.cycle_type != CYCLE_TYPE_COPY )
				{
					color_combiner();
					if( !blender() ) continue;
				} else {
					BL.out = TX.tex_sample;
					if( other.alpha_compare_en && !(BL.out.to16()&1) ) continue;
				}
				u32 p = BL.out.to32();
				//if( other.alpha_compare_en && !(p&1) ) continue;
				//if( other.force_blend && !(p&1) ) continue;
				*(u32*)&rdram[cimg.addr + (Y*cimg.width*4) + X*4] = __builtin_bswap32(p);
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
	
	s -= T.SL>>2;
	t -= T.TL>>2;
	
	if( other.cycle_type != CYCLE_TYPE_COPY )
	{
		//if( T.clampS || !T.maskS ) { s = std::clamp(s, s64(T.SL>>2), s64(T.SH>>2)); }
		//if( T.clampT || !T.maskT ) { t = std::clamp(t, s64(T.TL>>2), s64(T.TH>>2)); }
		if( T.clampS || !T.maskS ) { s = std::clamp(s, s64(0), s64(T.SH>>2)-s64(T.SL>>2)); }
		if( T.clampT || !T.maskT ) { t = std::clamp(t, s64(0), s64(T.TH>>2)-s64(T.TL>>2)); }
	}
	
	

	if( T.mirrorS )
	{
		u32 mbit = s & (T.maskS+1);
		if( mbit ) s ^= T.maskS;
		if( T.maskS )
		{
			s &= T.maskS;
		}
	} else if( T.maskS ) {
		s &= T.maskS;
	}
	if( T.mirrorT )
	{
		u32 mbit = t & (T.maskT+1);
		if( mbit ) t ^= T.maskT;
		if( T.maskT )
		{
			t &= T.maskT;		
		}
	} else if( T.maskT ) {
		t &= T.maskT;
	}
	
	u32 xorval = (t&1)?4:0;
	
	if( T.bpp == 16 ) 
	{
		if( T.format == 3 )
		{ // IA16
			u16 c = *(u16*)&tmem[((T.addr*8 + (t*T.line*8) + s*2)&0xffe)^xorval];
			u8 I = c;
			u8 A = c>>8;
			return dc(I,I,I,A);
		}
		return dc::from16(__builtin_bswap16( *(u16*)&tmem[((T.addr*8 + (t*T.line*8) + s*2)&0xffe)^xorval] ));
	} else if( T.bpp == 32 ) {
		//todo: put the rg/ba in the separate banks
		return dc::from32( __builtin_bswap32( *(u32*)&tmem[((T.addr*8 + (t*T.line*8) + s*4)&0xffc)^xorval] ));
	} else if( T.bpp == 8 ) {
		u8 v = tmem[((T.addr*8 + (t*T.line*8) + s)&0xfff)^xorval];
		if( T.format == 2 || T.format == 0 )
		{ // CI8
			if( other.tlut_type_ia16 )
			{
				u16 c = *(u16*)&tmem[((0x100+v)*8)];
				u8 I = c&0xff;
				u8 A = c>>8;
				return dc(I, I, I, A);
			}
			return dc::from16(__builtin_bswap16(*(u16*)&tmem[(0x100+v)*8]));
		} 
		if( T.format == 4 ) 
		{ // I8
			return dc(v,v,v,v);
		}
		if( T.format == 3 )
		{ // IA8
			u8 A = (v&15)|(v<<4);
			u8 I = (v&0xf0)|(v>>4);
			return dc(I,I,I,A);
		}
	} else if( T.bpp == 4 ) {
		u8 v = tmem[((T.addr*8 + (t*T.line*8) + (s>>1))&0xfff)^xorval];
		v >>= ((s^1)&1)*4;
		v &= 15;
		if( T.format == 2 )
		{  //CI4
			if( other.tlut_type_ia16 )
			{
				u16 c = *(u16*)&tmem[((0x100+T.palette*16+v)*8)&0xfff];
				u8 I = c&0xff;
				u8 A = c>>8;
				return dc(I, I, I, A);
			}
			return dc::from16(__builtin_bswap16(*(u16*)&tmem[((0x100+T.palette*16+v)*8)&0xfff]));
		}
		if( T.format == 3 )
		{  //IA4
			u8 I = (((v>>1)&7)/7.f)*255.f;
			u8 A = ((v&1)?0xff:0);
			return dc(I, I, I, A);		
		}
		if( T.format == 4 )
		{
			u8 I = v|(v<<4);
			return dc(I,I,I,I);
		}
	}
	
	//if( !(T.bpp == 4 && T.format == 4) )
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
	T /= (W ? W : 0x10000);

static struct {
    int shift;
    u32 add;
} z_format[8] = {
    6, 0x00000,
    5, 0x20000,
    4, 0x30000,
    3, 0x38000,
    2, 0x3c000,
    1, 0x3e000,
    0, 0x3f000,
    0, 0x3f800,
};

#define ZSHIFT 13 // 13 when trying to do the Z-compression stuff 16 otherwise

s64 decompress_z(u16 z)
{
	//return z;	
	z >>= 2;
	u64 exp = z>>11;
	u64 mant = z&0x7ff;
	u64 res = (mant<<z_format[exp].shift) + z_format[exp].add;
	return res;
}

u16 compress_z(s64 z)
{
	//return z>>16;	
	u32 Z = z>>ZSHIFT;
	Z &= 0x3ffff;
	//std::println("{:X}", Z);
	u32 exp = std::countl_one(Z<<14);
	if( exp > 7 ) exp = 7;
	return ((exp<<11)|((Z>>z_format[exp].shift)&0x7ff))<<2;
}

bool n64_rdp::z_compare(u32 sz, u32 oz, u32 dz)
{  // temporarily ripped from AngryLion until I match it with the manual and figure it out and complete it
	u32 olddz = b9[depth_image + (RS.cy*cimg.width*2) + RS.cx*2] & 3;
	olddz |= ( 3 & *(u16*)&rdram[depth_image + (RS.cy*cimg.width*2) + RS.cx*2] ) << 2;
	olddz = 1u << olddz;
	olddz <<= 3;
	dz <<= 3;
	
	u32 dznew = olddz > dz ? olddz : dz;

	uint32_t farther = 0 || ((sz + dznew) >= oz);
	
	switch( other.z_func & 3 )
	{
	case 0:
           { bool infront = sz < oz;
            auto diff = (int32_t)sz - (int32_t)dznew;
            bool nearer = 0 || (diff <= (int32_t)oz);
            bool max = (oz == 0x3ffff);
            return (max || (1 ? infront : nearer));
            }break;
        case 1:
           { bool infront = sz < oz;
            if (1) //!infront || !farther || 0)
            {
                auto diff = (int32_t)sz - (int32_t)dznew;
                bool nearer = 0 || (diff <= (int32_t)oz);
                bool max = (oz == 0x3ffff);
                return (max || (1 ? infront : nearer));
            }
            }//else
           if( 0 ) {
               // dzenc = dz_compress(dznotshift & 0xffff);
               // cvgcoeff = ((oz >> dzenc) - (sz >> dzenc)) & 0xf;
               // *curpixel_cvg = ((cvgcoeff * (*curpixel_cvg)) >> 3) & 0xf;
                return 1;
            }
            break;
        case 2:
            {bool infront = sz < oz;
            bool max = (oz == 0x3ffff);
            return (infront || max);
            }break;
        case 3:
            {auto diff = (int32_t)sz - (int32_t)dznew;
            bool nearer = 0 || (diff <= (int32_t)oz);
            bool max = (oz == 0x3ffff);
            return (farther && nearer && !max);
            }break;
        }
	return true;
	/*	
	switch( other.z_func )
	{
	case 0:
	
	
	case 1:
	
	
	case 2:
		return nz > oz;
	
	case 3:
		return nz != oz;	
	}
	return nz > oz;
	*/
}

void n64_rdp::triangle()
{
	const bool right = (cmdbuf[0] & BITL(55));
	RS.shade_color = blend_color;
	
	u32 deltaZ = (RS.DzDy>>16) & 0xffff;
	u32 temp = (RS.DzDx>>16);
	if( deltaZ & 0x8000 ) deltaZ ^= 0xffffu;
	deltaZ += ((temp&0x8000) ? ~temp : temp);

	if( !right )
	{
		for(s64 y = RS.y1>>16; y <= (RS.y3>>16) && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= scissor.ulY )
			for(s64 x = (RS.xh+0x0000)>>16; x >= (RS.xm-0x0000)>>16; --x)
			{
				if( x < scissor.ulX ) break;
				if( x > scissor.lrX ) { ATTR_XDEC; continue; }
				RS.cx = x;
				RS.cy = y;
				//u16 Z = (z>>16)&0xffff;
				if(other.z_compare&&
					!z_compare(z>>ZSHIFT, decompress_z(*(u16*)&rdram[depth_image+(cimg.width*2*y)+(x*2)]), deltaZ) )
				{					
					ATTR_XDEC;
					continue;
				}
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s+0x7fff; s64 T = t+0x7fff;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				if( other.cycle_type == CYCLE_TYPE_COPY ) std::println("copy mode tri");
				color_combiner();
				if( !blender() ) 
				{
					ATTR_XDEC;
					continue;
				}
				if( other.z_write ) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = compress_z(z);
				if( cimg.bpp == 16 )
				{
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
					*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(BL.out.to32());
				}
				ATTR_XDEC;
			}
			RS.xh += RS.DxhDy;
			ATTR_YINC;
			if( y == (RS.y2>>16) )
			{
				RS.xm = RS.xl;
				RS.DxmDy = RS.DxlDy;
			} else {
				RS.xm += RS.DxmDy;
			}			
		}
	} else {
		for(s64 y = RS.y1>>16; y <= (RS.y3>>16) && y < scissor.lrY; ++y)
		{
			s64 r = RS.r;
			s64 g = RS.g;
			s64 b = RS.b;
			s64 a = RS.a;
			s64 s = RS.s;
			s64 t = RS.t;
			s64 z = RS.z;
			s64 w = RS.w;
			if( y >= scissor.ulY )
			for(s64 x = (RS.xh-0x0000)>>16; x <= (RS.xm+0x0000)>>16; ++x)
			{
				if( x < scissor.ulX ) { ATTR_XINC; continue; }
				if( x > scissor.lrX ) break;
				RS.cx = x;
				RS.cy = y;
				if(other.z_compare&&
					!z_compare(z>>ZSHIFT, decompress_z(*(u16*)&rdram[depth_image+(cimg.width*2*y)+(x*2)]), deltaZ) )
				{
					ATTR_XINC;
					continue;
				}
				if( RS.cmd & 4 ) RS.shade_color = dc(r>>16,g>>16,b>>16,a>>16);
				if( RS.cmd & 2 ) 
				{
					s64 S = s; s64 T = t;
					PERSP
					TX.tex_sample = tex_sample(TX.tile, S>>5, T>>5);
				}
				if( other.cycle_type == CYCLE_TYPE_COPY ) std::println("copy mode tri");
				color_combiner();
				if( !blender() ) 
				{
					ATTR_XINC;
					continue;
				}
				if( other.z_write ) *(u16*)&rdram[depth_image + (cimg.width*2*y) + (x*2)] = compress_z(z);
				if( cimg.bpp == 16 )
				{
					*(u16*)&rdram[cimg.addr + (cimg.width*2*y) + (x*2)]= __builtin_bswap16(BL.out.to16());
				} else {
					*(u32*)&rdram[cimg.addr + (cimg.width*4*y) + (x*4)] = __builtin_bswap32(BL.out.to32());
				}
				ATTR_XINC;
			}
			RS.xh += RS.DxhDy;
			ATTR_YINC;
			if( y == (RS.y2>>16) )
			{
				RS.xm = RS.xl;
				RS.DxmDy = RS.DxlDy;
			} else {
				RS.xm += RS.DxmDy;
			}
		}
	}
}

void n64_rdp::load_block(u64 cmd)
{
	u32 dxt = cmd & 0xfff;
	u32 lrS = (cmd>>12)&0xfff;
	auto& T = tiles[(cmd>>24)&7];
	u32 ulT = (cmd>>32)&0xfff;
	u32 ulS = (cmd>>44)&0xfff;  // all 3 are u10.2
	//T.SL = ulS<<2;
	//T.TL = ulT<<2;
	u32 num_texels = lrS - ulS + 1;
	if( (num_texels/4) > 2048 ) 
	{
		printf("RDP: Num texels was >2048\n");
		//exit(1);
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
	u32 num_words = (((num_texels<<teximg.size)>>1)+7)>>3; 
	// was num_texels*8, hence: TIMES EIGHT?? WTF. yep, times8 makes both mario, starfox, and lcars work
	// the current expression was yoinked from MAME.
	u32 swpcnt = 0;//dxt;
	u32 ramoff = teximg.addr;// + ((ulS*T.bpp+7)/8);// + ((ulT*teximg.width*T.bpp+7)/8);
	u32 t = 0;
	u32 taddr = (T.addr + T.line*t)*8;
	for(u32 i = 0; i < num_words; ++i, taddr+=8, ramoff+=8)
	{
		u64 dw = *(u64*)&rdram[ramoff];
		if( swpcnt & BIT(11) ) dw = (dw<<32)|(dw>>32);
		*(u64*)&tmem[taddr&0xfff] = dw;
		swpcnt += dxt;
		if( swpcnt & BIT(11) ) { ulT+=1; }
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
		//std::println("ulT = ${:X}, lrT = ${:X}", ulT, lrT);
		//exit(1);
	}
	
	u32 num = (lrS - ulS + 1);
	for(u32 i = 0; i < num; ++i)
	{
		u64 p = (*(u16*)&rdram[teximg.addr + (ulT*teximg.width*2) + i*2]);
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
	case 2: return TX.tex_sample; //todo: tex1
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
	case 2: return TX.tex_sample; //todo: tex1
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
	case 2: return TX.tex_sample; //todo: tex1
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
	case 2: return TX.tex_sample; //todo: tex1
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
	case 2: return TX.tex_sample.a/255.f; //todo: tex1
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
	if( other.cycle_type == CYCLE_TYPE_1CYCLE )
	{
		dc res;
		{
			res = cc_a(1);
			dc b = cc_b(1);
			dc c = cc_c(1);
			dc d = cc_d(1);
			res -= b;
			res *= c;
			res += d;
		}
		
		float a = cc_alpha(CC.alpha_a[1]);
		float b = cc_alpha(CC.alpha_b[1]);
		float c = cc_alpha(CC.alpha_c[1]);
		float d = cc_alpha(CC.alpha_d[1]);
		res.a = std::clamp(((a - b) * c + d)*255.f, 0.f, 255.f);
		CC.out = res;
		return;
	}
	
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
	res.a = std::clamp(((a - b) * c + d)*255.f, 0.f, 255.f);
	CC.out = res;
	
	{
		res = cc_a(1);
		dc B = cc_b(1);
		dc C = cc_c(1);
		dc D = cc_d(1);
		res -= B;
		res *= C;
		res += D;
	}
	
	a = cc_alpha(CC.alpha_a[1]);
	b = cc_alpha(CC.alpha_b[1]);
	c = cc_alpha(CC.alpha_c[1]);
	d = cc_alpha(CC.alpha_d[1]);
	res.a = std::clamp(((a - b) * c + d)*255.f, 0.f, 255.f);
	CC.out = res;
	return;
}

bool n64_rdp::blender()
{
	//if( other.cov_x_alpha && CC.out.a == 0 ) return false; 
	
	if( other.cycle_type == CYCLE_TYPE_1CYCLE )
	{
		if( !other.force_blend )
		{
			if( other.cov_x_alpha && CC.out.a == 0 ) return false; 
			// ^ hack to get some decals&billboards to actually cut out, but I don't understand how coverage is calculated
			BL.out = CC.out;
			return true;
		}
		
		if( other.alpha_compare_en && CC.out.a <= blend_color.a ) return false; 
		if( CC.out.a == 0 ) return false; // ??? needed for yet a few more cut outs in sm64
	}
		
	dc P, M;
	float A=1, B=1;
	switch( BL.p[0] )
	{
	case 0: P = CC.out; break;
	case 1: P = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
				    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
				    break;
	case 2: P = blend_color; break;
	case 3: P = fog_color; break;
	default: std::println("BL.p error"); exit(1);
	}
	switch( BL.m[0] )
	{
	case 0: M = CC.out; break;
	case 1: M = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
				    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
				    break;
	case 2: M = blend_color; break;
	case 3: M = fog_color; break;
	default: std::println("BL.n error"); exit(1);
	}
	switch( BL.a[0] )
	{
	case 0: A = CC.out.a/255.f; break;
	case 1: A = fog_color.a/255.f; break;
	case 2: A = RS.shade_color.a/255.f; break;
	case 3: A = 0; break;
	default: std::println("BL.a error"); exit(1);
	}
	switch( BL.b[0] )
	{
	case 0: B = 1.f - A; break;
	case 1: B = 1; break; // some manuals say memory alpha. n64brew wiki says coverage
	//case 1: B = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])).a :
	//			    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])).a; 
	//			    break; //todo: coverage or memory alpha?
	case 2: B = 1; break;
	case 3: B = 0; break;
	default: std::println("BL.b error"); exit(1);
	}
	
	BL.out = P;
	BL.out *= A;
	M *= B;
	BL.out += M;

	if( other.cycle_type == CYCLE_TYPE_2CYCLE )
	{
		if( !other.force_blend )
		{
			if( other.cov_x_alpha && CC.out.a == 0 ) return false; 
			// ^ hack to get some decals&billboards to actually cut out, but I don't understand how coverage is calculated
			//BL.out = CC.out;
			return true;
		}
	
		//if( other.alpha_compare_en && CC.out.a <= blend_color.a ) return false; 
		//if( CC.out.a == 0 ) return false; // ??? needed for yet a few more cut outs in sm64
	
		switch( BL.p[1] )
		{
		case 0: P = BL.out; break;
		case 1: P = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
					    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
					    break;
		case 2: P = blend_color; break;
		case 3: P = fog_color; break;
		default: std::println("BL.p error"); exit(1);
		}
		switch( BL.m[1] )
		{
		case 0: M = BL.out; break;
		case 1: M = (cimg.bpp == 16) ? dc::from16(__builtin_bswap16(*(u16*)&rdram[cimg.addr + (cimg.width*RS.cy*2) + (RS.cx*2)])) :
					    dc::from32(__builtin_bswap32(*(u32*)&rdram[cimg.addr + (cimg.width*RS.cy*4) + (RS.cx*4)])); 
					    break;
		case 2: M = blend_color; break;
		case 3: M = fog_color; break;
		default: std::println("BL.n error"); exit(1);
		}
		switch( BL.a[1] )
		{
		case 0: A = CC.out.a/255.f; break;
		case 1: A = fog_color.a/255.f; break;
		case 2: A = RS.shade_color.a/255.f; break;
		case 3: A = 0; break;
		default: std::println("BL.a error"); exit(1);
		}
		switch( BL.b[1] )
		{
		case 0: B = 1.f - A; break;
		case 1: B = 1; break;  //todo: coverage
		case 2: B = 1; break;
		case 3: B = 0; break;
		default: std::println("BL.b error"); exit(1);
		}
		
		BL.out = P;
		BL.out *= A;
		M *= B;
		BL.out += M;
	}
	
	return true;
}






