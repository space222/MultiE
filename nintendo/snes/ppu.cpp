#include <print>
#include "snes.h"

void snes::ppu_draw_scanline()
{
	if( (ppu.bgmode & 7) > 1 ) 
	{
		std::println("Using mode {}", ppu.bgmode&7);
		exit(1);
	}
	
	{
		u16 bgc = ppu.cgram[0];
		u8 R = (bgc&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 G = ((bgc>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 B = ((bgc>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
		for(u32 x = 0; x < 256; ++x) fbuf[ppu.scanline*256 + x] = (B<<10)|(G<<5)|R;
	}
	
	if( (ppu.bgmode&7) == 1 )
	{
		u16* v16 = (u16*)ppu.vram;
		for(u32 i = 0; i < 256; ++i)
		{
			u16 mapentry = v16[(((ppu.bg3sc>>2)*1024) + (ppu.scanline/8)*32 + (i/8))&0x7fff];
			u16 tile = mapentry & 0x3ff;
			u16 chrbase1 = (ppu.bg34nba&15)<<12;
			u16 w1 = v16[chrbase1 + tile*8 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
			u8 pix = (i&7)^((mapentry&BIT(14))?0:7);
			u8 b1 = w1;
			u8 b2 = w1>>8;
			b2 = (b2>>pix)&1;
			u8 bpp2 = (b2<<1)|((b1>>pix)&1);
			u16 pal = ppu.cgram[((mapentry>>10)&7)*4 + bpp2];

			u8 R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
			u8 G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
			u8 B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
			if( bpp2 )
			{
				fbuf[ppu.scanline*256 + i] = (B<<10)|(G<<5)|R;
				continue;
			}
		
			if( 1 )
			{
				mapentry = v16[(((ppu.bg2sc>>2)*1024) + (ppu.scanline/8)*32 + (i/8))&0x7fff];
				tile = mapentry & 0x3ff;
				chrbase1 = (ppu.bg12nba>>4)<<12;
				w1 = v16[chrbase1 + tile*16 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
				u16 w2 = v16[chrbase1 + tile*16 + 8 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
				pix = (i&7)^((mapentry&BIT(14))?0:7);
				b1 = w1;
				b1 = (b1>>pix)&1;
				b2 = w1>>8;
				b2 = (b2>>pix)&1;
				u8 b3 = w2;
				b3 = (b3>>pix)&1;
				u8 b4 = w2>>8;
				b4 = (b4>>pix)&1;
				
				u8 bpp4 = (b4<<3)|(b3<<2)|(b2<<1)|b1;
				pal = ppu.cgram[(((mapentry>>10)&7)*16 + bpp4)&0xff];

				R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
				G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
				B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
				if( bpp4 )
				{
					fbuf[ppu.scanline*256 + i] = (B<<10)|(G<<5)|R;
					continue;
				}
			}

			mapentry = v16[(((ppu.bg1sc>>2)*1024) + (ppu.scanline/8)*32 + (i/8))&0x7fff];
			tile = mapentry & 0x3ff;
			chrbase1 = (ppu.bg12nba&15)<<12;
			w1 = v16[chrbase1 + tile*16 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
			u16 w2 = v16[chrbase1 + tile*16 + 8 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
			pix = (i&7)^((mapentry&BIT(14))?0:7);
			b1 = w1;
			b1 = (b1>>pix)&1;
			b2 = w1>>8;
			b2 = (b2>>pix)&1;
			u8 b3 = w2;
			b3 = (b3>>pix)&1;
			u8 b4 = w2>>8;
			b4 = (b4>>pix)&1;
			
			u8 bpp4 = (b4<<3)|(b3<<2)|(b2<<1)|b1;
			pal = ppu.cgram[(((mapentry>>10)&7)*16 + bpp4)&0xff];

			R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
			G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
			B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
			if(  1 )
			{
				if( bpp4 ) 
				{
					fbuf[ppu.scanline*256 + i] = (B<<10)|(G<<5)|R;
				}
			}
		}
		return;
	}
	
	u16* v16 = (u16*)ppu.vram;
	for(u32 i = 0; i < 256; ++i)
	{
		u16 mapentry = v16[(((ppu.bg2sc>>2)*1024) + (ppu.scanline/8)*32 + (i/8))&0x7fff];
		u16 tile = mapentry & 0x3ff;
		u16 chrbase1 = (ppu.bg12nba>>4)<<12;
		u16 b1 = v16[chrbase1 + tile*8 + ((ppu.scanline&7)^((mapentry&BIT(15))?7:0))];
		u8 pix = (i&7)^((mapentry&BIT(14))?0:7);
		u8 b2 = b1>>8;
		b2 = (b2>>pix)&1;
		u8 bpp2 = (b2<<1)|((b1>>pix)&1);
		u16 pal = ppu.cgram[((mapentry>>10)&7)*4 + bpp2];

		u8 R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
		u8 B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
		if( bpp2 ) fbuf[ppu.scanline*256 + i] = (B<<10)|(G<<5)|R;
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
		if( io.nmitimen & 0x80 ) { cpu.nmi_line = true; }
		
		ppu.rdnmi |= 0x80;
	}
	
	if( ppu.scanline >= 263 )
	{
		ppu.frame_complete = true;
		ppu.scanline = 0;
		ppu.rdnmi &= ~0x80;
	}
}

