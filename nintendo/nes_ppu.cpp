#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "nes.h"

extern console* sys;
#define NES (*dynamic_cast<nes*>(sys))
#define PPUCTRL ppu_regs[0]
#define PPUMASK ppu_regs[1]
#define PPUSTAT ppu_regs[2]
#define PPUDATA ppu_regs[7]

extern u32 nes_palette[];

u8 nes::ppu_bus(u16 addr)
{
	addr &= 0x3fff;
	//if( addr < 0x3000 ) mapper_ppu_bus(this, addr);
	
	if( addr < 0x2000 )
	{
		return chrbanks[(addr>>10)&7][addr&0x3ff];
	}
	if( addr < 0x2400 ) return nametable[0][addr&0x3ff];
	if( addr < 0x2800 ) return nametable[1][addr&0x3ff];
	if( addr < 0x2C00 ) return nametable[2][addr&0x3ff];
	if( addr < 0x3000 ) return nametable[3][addr&0x3ff];
	if( addr < 0x3F00 ) return 0xff;
	return ppu_palmem[addr&0x1F];
}

void nes::ppu_bus(u16 addr, u8 val)
{
	addr &= 0x3fff;
	if( addr < 0x2000 )
	{
		if( !chrram ) return;
		chrbanks[(addr>>10)&7][addr&0x3ff] = val;
		return;
	}
	if( addr < 0x2400 ) { nametable[0][addr&0x3ff] = val; return; }
	if( addr < 0x2800 ) { nametable[1][addr&0x3ff] = val; return; }
	if( addr < 0x2C00 ) { nametable[2][addr&0x3ff] = val; return; }
	if( addr < 0x3000 ) { nametable[3][addr&0x3ff] = val; return; }
	if( addr < 0x3F00 ) return;
	val &= 0x3F;
	addr &= 0x1F;
	ppu_palmem[addr] = val;
	if( addr == 0 ) ppu_palmem[0x10] = val;
	else if( addr == 0x10 ) ppu_palmem[0] = val;
}

void nes::ppu_write(u8 addr, u8 val)
{
	addr &= 7;
	if( addr == 5 )
	{
		if( ppu_latch )
		{
			loopy_t &= ~0xE3E0;
			loopy_t |= (val&7)<<12;
			loopy_t |= (val&0xF8)<<2;
		} else {
			fine_x = (val&7);
			loopy_t &= ~0x1F;
			loopy_t |= (val>>3);
		}
		ppu_latch = !ppu_latch;
		return;
	}
	if( addr == 6 )
	{
		if( ppu_latch )
		{
			loopy_t &= 0xff00;
			loopy_t |= val;
			loopy_v = loopy_t;
		} else {
			loopy_t &= 0xff;
			loopy_t |= val<<8;
			loopy_t &=~0xc000;
		}
		ppu_latch = !ppu_latch;
		return;
	}
	if( addr == 7 )
	{
		ppu_bus(loopy_v, val);
		loopy_v += (PPUCTRL & 4) ? 32 : 1;
		//ppu_regs[7] = val;
		return;
	}
	if( addr == 0 )
	{
		ppu_regs[0] = val;
		loopy_t &= ~0xc00;
		loopy_t |= (val&3)<<10;
		return;	
	}
	
	ppu_regs[addr] = val;
	//printf("$%lX: PPU Write $%X = $%X\n", cpu.stamp, addr, val);
}

u8 nes::ppu_read(u8 addr)
{
	addr &= 7;
	//printf("$%lX:$%02X: PPU read $%X\n", cpu.stamp, cpu.PC, addr);
	if( addr == 2 )
	{
		u8 d = PPUSTAT;
		PPUSTAT &=~0x80;
		ppu_latch = false;
		return d;
	}
	if( addr == 7 )
	{
		u8 d = PPUDATA;
		PPUDATA = ppu_bus(loopy_v);
		loopy_v += (PPUCTRL & 4) ? 32 : 1;
		return d;	
	}
	return 0x20;
}

inline u32 getpal(u32 v)
{
	u32 C = nes_palette[v];
	u8 R = C>>0;
	u8 G = C>>8;
	u8 B = C>>16;
	return (R<<24)|(G<<16)|(B<<8);
}

void nes::draw_scanline()
{
	u8 bgdata[256];
	memset(bgdata, 0, 256);
	if( PPUMASK&8 )
	{
		mapper_ppu_bus(this, ((PPUCTRL&0x10)?0x1000:0));
		for(int i = 0; i < 256; ++i)
		{
			u16 ntaddr = 0x2000 + (loopy_v & 0xfff); 
			//0x2000 + ((PPUCTRL&3)<<10) + (((scanline+scroll_y)&0xff)/8)*32 + ((i+scroll_x)&0xff)/8;
			
			//u16 attraddr = 0x23c0 + ((PPUCTRL&3)<<10) + (scanline/16)*8 + (i/16);
			u16 attraddr = 0x23C0 | (ntaddr & 0x0C00) | ((ntaddr >> 4) & 0x38) | ((ntaddr >> 2) & 0x07);
			u8 tile = ppu_bus(ntaddr);
			u8 attr = ppu_bus(attraddr);
			//u16 taddr = ((PPUCTRL&0x10)?0x1000:0) + (tile*16) + ((scanline+scroll_y)&7);
			//u8 b1 = (ppu_bus(taddr)>>(7-((i+scroll_x)&7))) & 1;
			//u8 b2 = (ppu_bus(taddr+8)>>(7-((i+scroll_x)&7))) & 1;
			u16 taddr = ((PPUCTRL&0x10)?0x1000:0) + (tile*16) + ((loopy_v>>12)&7);
			u8 b1 = (ppu_bus(taddr)>>(7-fine_x)) & 1;
			u8 b2 = (ppu_bus(taddr+8)>>(7-fine_x)) & 1;
			
			attr >>= ( ((ntaddr&2)) | ((ntaddr&0x40)>>4) ) ;
			attr &= 3;
			b1 |= b2<<1;
			if( b1 == 0 ) attr = 0;
			bgdata[i] = (attr<<2)|b1;
			if( !(PPUMASK&2) && i < 8 )
			{
				fbuf[scanline*256 + i] = getpal(ppu_palmem[0]);
			} else {
				fbuf[scanline*256 + i] = getpal(ppu_palmem[b1|(attr<<2)]);
			}
			
			fine_x++;
			if( fine_x == 8 )
			{
				fine_x = 0;
				if( (loopy_v & 0x1F) == 0x1F )
				{
					loopy_v &= ~0x1F;
					loopy_v ^= 0x400;
				} else {
					loopy_v++;
				}
			}
		}
	} else {
		const u32 c = getpal(ppu_palmem[0]);
		for(int i = 0; i < 256; ++i) fbuf[scanline*256+i] = c;
	}
	
	if( (PPUMASK&0x10) && scanline != 0 )
	{
		mapper_ppu_bus(this, ((PPUCTRL&8)?0x1000:0));
		u32 sprsize = (PPUCTRL&0x20) ? 16 : 8;
		int num_sprites = 0;
		for(u32 i = (PPUMASK&4)?0:8; i < 256 && num_sprites < 8; ++i)
		{
			for(u32 s = 0; s < 64; ++s)
			{
				if( OAM[s*4] >= 0xEF) continue;
				u32 Y = OAM[s*4] + 1;
				u32 line = u32(scanline);
				if( !(line >= Y && line - Y < sprsize) ) continue;
				//num_sprites++;
				
				u32 X = OAM[s*4+3];
				if( !(i >= X && i - X < 8) ) continue;
				
				u8 tile = OAM[s*4+1];
				
				u8 L = line-Y;
				u16 taddr = 0;
				if( PPUCTRL&0x20 )
				{
					taddr = (tile&1)?0x1000:0;
					tile &= 0xFE;
					if( OAM[s*4+2]&0x80 ) L = 15-L;
					if( L > 7 )
					{
						L -= 8;
						tile++;
					}
					taddr += tile*16 + L;			
				} else {
					if( OAM[s*4+2]&0x80 ) L = 7-L;
					taddr = ((PPUCTRL&8)?0x1000:0) + tile*16 + L;
				}
				
				u8 bit = 7 - (i-X);
				if( OAM[s*4+2]&0x40 ) bit = 7-bit;
				u8 b1 = ppu_bus(taddr)>>bit; b1&=1;
				u8 b2 = ppu_bus(taddr+8)>>bit; b2&=1;
				b1 |= b2<<1;
				if( !b1 ) continue;
				if( s == 0 && b1 && bgdata[i] ) PPUSTAT |= 0x40;
				
				b1 |= (OAM[s*4+2]&3)<<2;
				b1 += 0x10;
				
				if( OAM[s*4+2]&0x20 )
				{
					if( bgdata[i] == 0 ) fbuf[scanline*256+i] = getpal(ppu_palmem[b1]);
				} else {
					fbuf[scanline*256+i] = getpal(ppu_palmem[b1]);
				}
				break;
			}
		}
	}
	
	if( PPUMASK & 0x18 )
	{
		if ((loopy_v & 0x7000) != 0x7000)        // if fine Y < 7
	  	{
	  		loopy_v += 0x1000;
	  	} else {
			loopy_v &= ~0x7000;                   // fine Y = 0
			int y = (loopy_v & 0x03E0) >> 5;        // let y = coarse Y
			if (y == 29)
			{
				y = 0;                       // coarse Y = 0
				loopy_v ^= 0x0800;                   // switch loopy_vertical nametable
			} else if (y == 31) {
				y = 0;                          // coarse Y = 0, nametable not switched
			} else {
				y += 1;                         // increment coarse Y
			}
			loopy_v = (loopy_v & ~0x03E0) | (y << 5);     // put coarse Y back into loopy_v
	  	}
		
		loopy_v &= ~0x41f;
		loopy_v |= loopy_t & 0x41f;
	}
}

void nes::ppu_dot()
{
	dot++;
	if( dot == 341 )
	{
		dot = 0;
		if( scanline < 240 ) 
		{
			if( PPUMASK & 0x18 ) 
			{
				draw_scanline();
			} else {
				u32 c = getpal(ppu_palmem[0]);
				for(int i = 0; i < 256; ++i) fbuf[scanline*256 + i] = c;
			}
		} else if( scanline == 262 ) {
			scanline = 0;
			if( PPUMASK & 0x18 )
			{
				loopy_v &= ~0x7be0;
				loopy_v |= loopy_t&0x7be0;
			}
			return;
		} else if( scanline == 241 ) {
			PPUSTAT |= 0x80;
			if( PPUCTRL & 0x80 ) cpu.nmi();
			frame_complete = true;
		} else if( scanline == 261 ) {
			PPUSTAT = 0;
		}
		scanline++;
	}
}

u32 nes_palette[] = {
0x464646, 0x540100, 0x700000, 0x6b0007, 0x480028, 0xe003c, 0x3e, 0x2c, 0x30d, 0x1500, 
0x1f00, 0x1f00, 0x201400, 0x0, 0x0, 0x0, 0x9d9d9d, 0xb04100, 0xd52518, 0xcf0d4a, 
0x9f0075, 0x530190, 0xf92, 0x287b, 0x4451, 0x5c20, 0x6900, 0x166900, 0x6a5a00, 0x0, 
0x0, 0x0, 0xfffffe, 0xff9648, 0xff6d62, 0xff5b8e, 0xff5ed4, 0xb460f1, 0x5e6ff3, 0x1788dc, 
0xa4b2, 0xbd7f, 0x28ca53, 0x76ca38, 0xcbbb36, 0x2b2b2b, 0x0, 0x0, 0xfffffe, 0xffd2b0, 
0xffbbb6, 0xffb4cb, 0xffbced, 0xe0bdf9, 0xbdc3fa, 0x9fcef0, 0x90d9df, 0x93e3ca, 0xa6e9b8, 
0xc6e9ad, 0xe9e3ac, 0xa7a7a7, 0x0, 0x0,
};




