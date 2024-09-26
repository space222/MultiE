#include <cstdio>
#include "TMS9918A.h"

static u32 tms9918a_pal[] = { 0, 0, 0x16c537, 0x55da70, 0x4b4aeb, 0x756dfc, 
	0xd14743, 0x3aeaf4, 0xfc4a4a, 0xff7070, 0xd1bd4a, 0xe5cb78, 0x16ac30, 0xc550b5, 0xc8c8c8, 0xffffff };

void TMS9918A::draw_scanline(u32 line)
{
	u32* FL = &fbuf[line*256];
	if( !(vdp_regs[1]&BIT(6)) )
	{
		memset(FL, 0, 256*4);
		return;
	}
	
	u8 M3 = (vdp_regs[0]&2);
	u8 M2 = (vdp_regs[1]&8);
	u8 M1 = (vdp_regs[1]&0x10);
	
	u8 spr[256];
	memset(spr, 0, 256);
	if( !M1 )
	{
		u16 sprtab = (vdp_regs[5])*0x80;
		u32 size = (vdp_regs[1]&2)?16:8;
		u32 mul = (vdp_regs[1]&1)?2:1;
		u32 num = 0;
		for(u32 i = 0; i < 32 && num < 4; ++i)
		{
			u8 Y = vram[sprtab + i*4];
			if( Y == 0xD0 ) break;
			Y += 1;
			if( line < Y || line >= Y + size*mul ) continue;
			num += 1;
			
			int X = vram[sprtab + i*4 + 1];
			if( vram[sprtab + i*4 + 3] & BIT(4) ) X -= 32;
			u16 sprpat = (vdp_regs[6])*0x800;
			sprpat += (vram[sprtab + i*4 + 2] + (((line-Y)/mul)>7 ? 1 : 0)) * 8;
			for(u32 sx = 0; sx < size*mul; ++sx, ++X)
			{
				if( X < 0 ) continue;
				u8 d = vram[sprpat + ((sx>7 && size*mul>8)?16:0) + ((line-Y)&7)];
				d >>= 7^(sx&7);
				d &= 1;
				if( !spr[X] ) spr[X] = d ? (vram[sprtab + i*4 + 3]&0xf) : 0;
			}
		}	
	}
	
	for(u32 X = 0; X < 256; ++X)
	{
		if( 0 ) //X < 8 && (vdp_regs[0]&BIT(5)) )
		{
			u32 c = tms9918a_pal[(vdp_regs[7]>>4)];
			FL[X] = c<<8;
			continue;
		}

		u32 y = line;
		u32 x = X;
		int tile_x = x / 8;
		int tile_y = y / 8;
		u8 p = 0;
		
		if( M3 )
		{
			u16 map_addr = ((vdp_regs[2]&0xf)*0x400) + (tile_y*32 + tile_x);
			u8 nt = vram[map_addr];	
			u16 tile_addr = ((vdp_regs[4]&4)*0x800) + ((line/64)*256*8) + nt*8 + (y&7);
			u8 d = vram[tile_addr];
			d >>= 7^(x&7);
			d &= 1;
			u8 ct = vram[((vdp_regs[3]&0x80)*0x40) + ((line/64)*256*8) + (nt*8) + (y&7)];
			p = ( d ? (ct>>4) : (ct&0xf) );	
		} else {
			u16 map_addr = ((vdp_regs[2]&0xf)*0x400) + (tile_y*32 + tile_x);
			u8 nt = vram[map_addr];	
			u16 tile_addr = ((vdp_regs[4]&7)*0x800) + (nt*8 + (y&7));
			u8 d = vram[tile_addr];
			d >>= 7^(x&7);
			d &= 1;
			p = (d ? (vram[(vdp_regs[3]*0x40) + (nt>>3)]>>4) : (vram[(vdp_regs[3]*0x40) + (nt>>3)]&0xf));
		}
		
		u32 c = tms9918a_pal[spr[x] ? spr[x] : p];
		FL[X] = c<<8;
	}
	return;
}

void TMS9918A::ctrl(u8 v)
{
	if( vdp_ctrl_latch )
	{
		vdp_addr = (v<<8)|(vdp_addr&0xff);
		vdp_cd = vdp_addr>>14;
		vdp_addr &= 0x3fff;
		if( vdp_cd == 2 ) 
		{
			vdp_regs[v&7] = vdp_addr;
			//printf("VDP Reg %i = $%X\n", v&7, vdp_regs[v&7]);
		} else if( vdp_cd == 3 ) {
			//rdbuf = cram[vdp_addr&0x1f];
		} else if( vdp_cd == 1 || vdp_cd == 0 ) {
			rdbuf = vram[vdp_addr&0x3fff];
		}
	} else {
		vdp_addr = (vdp_addr&0xff00) | v;			
	}
	vdp_ctrl_latch = !vdp_ctrl_latch;
	return;
}

void TMS9918A::data(u8 v)
{
	vdp_ctrl_latch = false;
	rdbuf = v;
	if( vdp_cd == 1 || vdp_cd == 0 ) 
	{
		vram[vdp_addr&0x3fff] = v;	
		vdp_addr++;
	}
	return;
}


