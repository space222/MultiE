#include <print>
#include "snes.h"

void snes::ppu_draw_scanline()
{
	u16* v16 = (u16*)ppu.vram;
	for(u32 i = 0; i < 256; ++i)
	{
		u16 mapentry = v16[(((ppu.bg1sc>>2)*1024) + (ppu.scanline/8)*32 + (i/8))&0x7fff];
		u16 tile = mapentry & 0x3ff;
		u16 chrbase1 = (ppu.bg12nba&15)<<12;
		u16 b1 = v16[chrbase1 + tile*8 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
		u8 pix = (i&7)^((mapentry&BIT(14))?0:7);
		u8 b2 = b1>>8;
		b2 = (b2>>pix)&1;
		u8 bpp2 = (b2<<1)|((b1>>pix)&1);
		u16 pal = ppu.cgram[((mapentry>>10)&7)*4 + bpp2];

		u8 R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
		fbuf[ppu.scanline*256 + i] = (B<<10)|(G<<5)|R;
	}
}

void snes::ppu_mc(s64 mc)
{
	ppu.master_cycles += mc;
	if( ppu.master_cycles < 1364 ) return;
	ppu.master_cycles -= 1364;

	if( ppu.scanline < 224 ) ppu_draw_scanline();
	ppu.scanline++;
	if( ppu.scanline == 224 )
	{
		//todo: vblank nmi
		ppu.rdnmi |= 0x80;
	}
	
	if( ppu.scanline >= 263 )
	{
		ppu.frame_complete = true;
		ppu.scanline = 0;
		ppu.rdnmi &= ~0x80;
	}
}

