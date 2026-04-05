#include <print>
#include "snes.h"

void snes::render_sprites(u16* spr)
{
	if( ppu.scanline == 0 ) return;
	u32 num_sprites = 0;
	u8 sprind[32]={0};
	for(u32 i = 0; i < 128 && num_sprites<32; ++i)
	{
		u8 SI = (ppu.oam_highest_pri + i)&0x7f;
		int Y = ppu.oam[SI*4 + 1] + 1;
		int X = ppu.oam[SI*4 + 0];
		u32 upperX = ppu.oamhi[SI/4]>>((SI&3)*2);
		if( upperX&1 ) X -= 256;
		int w=8, h=8;
		switch( (ppu.objsel>>5)&7 )
		{
		case 0: if(upperX&2) w=h=16; break;
		case 1: if(upperX&2) w=h=32; break;
		case 2: if(upperX&2) w=h=64; break;
		case 3: if(upperX&2) w=h=32; else w=h=16; break;
		case 4: if(upperX&2) w=h=64; else w=h=16; break;
		case 5: if(upperX&2) w=h=64; else w=h=32; break;
		case 6: if(upperX&2) { w=32;h=64; } else { w=16;h=32; } break;
		case 7: if(upperX&2) w=h=32; else { w=16;h=32; } break;			
		}
		
		if( X < -w ) continue;
		if( ( (Y + h - 1 > 255) && ppu.scanline <= ((Y+h-1)&0xff) ) || (ppu.scanline >= Y && ppu.scanline < (Y + h)) )
		{
			sprind[num_sprites++] = SI;
		}
	}
	
	//u32 pixels_drawn = 0;

	for(u32 i = 0; i < num_sprites; ++i)
	{
		int Y = ppu.oam[sprind[i]*4 + 1] + 1;
		int X = ppu.oam[sprind[i]*4 + 0];
		u32 upperX = ppu.oamhi[sprind[i]/4]>>((sprind[i]&3)*2);
		if( upperX&1 ) X -= 256;
		int w=8, h=8;
		switch( (ppu.objsel>>5)&7 )
		{
		case 0: if(upperX&2) w=h=16; break;
		case 1: if(upperX&2) w=h=32; break;
		case 2: if(upperX&2) w=h=64; break;
		case 3: if(upperX&2) w=h=32; else w=h=16; break;
		case 4: if(upperX&2) w=h=64; else w=h=16; break;
		case 5: if(upperX&2) w=h=64; else w=h=32; break;
		case 6: if(upperX&2) { w=32;h=64; } else { w=16;h=32; } break;
		case 7: if(upperX&2) w=h=32; else { w=16;h=32; } break;
		}
		
		for(int px = 0; px < w && X+px < 256; ++px)
		{
			//pixels_drawn += 1;
			//if( pixels_drawn >= 34*8 ) return;
			if( X+px < 0 || spr[X+px] ) continue;
			bool flipY = (ppu.oam[sprind[i]*4 + 3] & 0x80);
			bool flipX = (ppu.oam[sprind[i]*4 + 3] & 0x40);
			int relY = (Y+h-1>255) ? h - (((Y+h-1)&255) - ppu.scanline) - 1 : ppu.scanline - Y;
			if( flipY ) { relY = h - relY - 1; }
			int relX = (flipX ? px : w - px - 1);
			int tile = ppu.oam[sprind[i]*4 + 2];
			tile += (relY/8)*16;
			tile += ((flipX ? w - px - 1 : px)/8);
			u32 offset = tile*16 + ((ppu.oam[sprind[i]*4 + 3]&1) ? ((((ppu.objsel>>3)&3)+1)<<12) :0);
			relY &= 7;
			//if( flipY ) { relY ^= 7; }
			relX &= 7;
			
			u16 w1 = ((u16*)ppu.vram)[((ppu.objsel&3)<<13) + offset + relY];
			u16 w2 = ((u16*)ppu.vram)[((ppu.objsel&3)<<13) + offset + 8 + relY];
			u8 b1 = (w1>>relX)&1;
			u8 b2 = ((w1>>8)>>relX)&1;
			u8 b3 = (w2>>relX)&1;
			u8 b4 = ((w2>>8)>>relX)&1;
			u16 pal_ind = (b4<<3)|(b3<<2)|(b2<<1)|b1;
			if( pal_ind )
			{
				spr[X+px] = 0x80 + ((ppu.oam[sprind[i]*4 + 3]>>1)&7)*16 + pal_ind + ((ppu.oam[sprind[i]*4 + 3]&0x30)<<4);
			}
		}		
	}
}

u16 snes::pal2c16(u8 p)
{
	u16 pal = ppu.cgram[p];
	u8 R = (pal&0x1f) * ((ppu.inidisp&15)/15.f);
	u8 G = ((pal>>5)&0x1f) * ((ppu.inidisp&15)/15.f);
	u8 B = ((pal>>10)&0x1f) * ((ppu.inidisp&15)/15.f);
	return (B<<10)|(G<<5)|R;
}

void snes::render_bg(u16* res, u32 bpp, u32 index)
{
	u16* v16 = (u16*)ppu.vram;
	u32 yscroll, xscroll, charbase;
	u32 bgsc;
	switch( index )
	{
	case 1: charbase = (ppu.bg12nba&15)<<12; bgsc = ppu.bg1sc; xscroll = ppu.bg1hofs; yscroll = ppu.bg1vofs; break;
	case 2: charbase = (ppu.bg12nba>>4)<<12; bgsc = ppu.bg2sc; xscroll = ppu.bg2hofs; yscroll = ppu.bg2vofs; break;
	case 3: charbase = (ppu.bg34nba&15)<<12; bgsc = ppu.bg3sc; xscroll = ppu.bg3hofs; yscroll = ppu.bg3vofs; break;
	case 4: charbase = (ppu.bg34nba>>4)<<12; bgsc = ppu.bg4sc; xscroll = ppu.bg4hofs; yscroll = ppu.bg4vofs; break;
	}
	
	for(u32 i = 0; i < 256; ++i)
	{
		u32 X = i + (xscroll&0x3ff);
		u32 Y = ppu.scanline + (yscroll&0x3ff);
		u32 mapaddr = ((bgsc>>2)*1024);
		u32 tilesize = (bpp==2) ? 8 : ((bpp==4) ? 16 : 32);
		u32 palmult =  (bpp==2) ? 4 : ((bpp==4) ? 16 : 0);
		
		switch( bgsc&3 )
		{
		case 0: break; // one screen wrapping
		case 1: X &= 0x1ff; if( X > 255 ) mapaddr += 0x400; break;
		case 2: Y &= 0x1ff; if( Y > 255 ) mapaddr += 0x400; break;
		case 3:
			Y &= 0x1ff; if( Y > 255 ) mapaddr += 0x800;
			X &= 0x1ff; if( X > 255 ) mapaddr += 0x400;		
			break;
		}
		
		u16 w1=0, w2=0, w3=0, w4=0, mapentry=0;
		if( (ppu.bgmode&(1u<<(index+3))) )
		{ //todo: find better tests for 16x16 mode than the konami beam intro
			Y &= 0xff;
			X &= 0xff;
			mapentry = v16[(mapaddr + (Y/16)*32 + (X/16))&0x7fff];
			u16 tile = mapentry & 0x3ff;
			if( (X&8) ) tile += 1;
			if( (Y&8) ) tile += 16;
			w1 = v16[(charbase + tile*tilesize + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			if( bpp >= 4 )
			{
				w2 = v16[(charbase + tile*tilesize + 8 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			}
			if( bpp == 8 )
			{
				w3 = v16[(charbase + tile*tilesize + 16 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
				w4 = v16[(charbase + tile*tilesize + 24 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			}
		} else {
			Y &= 0xff;
			X &= 0xff;
			mapentry = v16[(mapaddr + (Y/8)*32 + (X/8))&0x7fff];
			u16 tile = mapentry & 0x3ff;
			w1 = v16[(charbase + tile*tilesize + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			if( bpp >= 4 )
			{
				w2 = v16[(charbase + tile*tilesize + 8 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			}
			if( bpp == 8 )
			{
				w3 = v16[(charbase + tile*tilesize + 16 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
				w4 = v16[(charbase + tile*tilesize + 24 + ((Y&7)^((mapentry&BIT(15))?7:0)))&0x7fff];
			}
		}
		u8 pix = (X&7)^((mapentry&BIT(14))?0:7);
		u8 b1 = (w1>>pix)&1;
		u8 b2 = ((w1>>8)>>pix)&1;
		u8 b3 = (w2>>pix)&1;
		u8 b4 = ((w2>>8)>>pix)&1;
		u8 b5 = (w3>>pix)&1;
		u8 b6 = ((w3>>8)>>pix)&1;
		u8 b7 = (w4>>pix)&1;
		u8 b8 = ((w4>>8)>>pix)&1;
		u16 pal_ind = (b8<<7)|(b7<<6)|(b6<<5)|(b5<<4)|(b4<<3)|(b3<<2)|(b2<<1)|b1;
		if( pal_ind )
		{
			if( (ppu.bgmode&7)==0 ) pal_ind += ((index-1)*32);
			res[i] = (((mapentry>>10)&7)*palmult + pal_ind) | (mapentry & BIT(13));
		} else {
			res[i] = 0;
		}
	}		
}

void snes::ppu_draw_scanline()
{
	{
		u16 bgc = ppu.fixed_color;
		u8 R = (bgc&0x1f);
		u8 G = ((bgc>>5)&0x1f);
		u8 B = ((bgc>>10)&0x1f);
		R *= ((ppu.inidisp&15)/15.f);
		G *= ((ppu.inidisp&15)/15.f);
		B *= ((ppu.inidisp&15)/15.f);
		for(u32 x = 0; x < 256; ++x) fbuf[ppu.scanline*256 + x] = (B<<10)|(G<<5)|R;
	}
	u16 bg1[256]={0};
	u16 bg2[256]={0};
	u16 bg3[256]={0};
	u16 bg4[256]={0};
	u16 spr[256]={0};
			
	if( (ppu.bgmode&7) == 1 )
	{
		u8 re = ppu.tm|ppu.ts;
		if( re & 1 ) render_bg(bg1, 4, 1);
		if( re & 2 ) render_bg(bg2, 4, 2);
		if( re & 4 ) render_bg(bg3, 2, 3);
		if( re & 16 ) render_sprites(spr);
		
		bool bg3hi = ppu.bgmode & 8;
		
		u32 scoffs = ppu.scanline*256;
		for(u32 i = 0; i < 256; ++i, ++scoffs)
		{ 
			if( ppu.tm & 4 )if( bg3hi && (bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( ppu.tm & 16 )if( (spr[i]&0x300) == 0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 )if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.tm & 2 )if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.tm & 16 )if( (spr[i]&0x300) == 0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 )if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.tm & 2 )if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.tm & 16 )if( (spr[i]&0x300) == 0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 4 )if( !bg3hi && (bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( ppu.tm & 16 )if( spr[i] && (spr[i]&0x300) == 0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 4 )if( bg3[i] ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }

			if( ppu.ts & 4 )if( bg3hi && (bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( ppu.ts & 16 )if( (spr[i]&0x300) == 0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 )if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.ts & 2 )if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.ts & 16 )if( (spr[i]&0x300) == 0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 )if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.ts & 2 )if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.ts & 16 )if( (spr[i]&0x300) == 0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 4 )if( !bg3hi && (bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( ppu.ts & 16 )if( spr[i] && (spr[i]&0x300) == 0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 4 )if( bg3[i] ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
		}
		return;
	}
	
	if( (ppu.bgmode&7) == 3 )
	{
		u8 re = ppu.tm | ppu.ts;
		if( re & 1 ) render_bg(bg1, 8, 1);
		if( re & 2 ) render_bg(bg2, 4, 2);
		render_sprites(spr);
		
		u32 scoffs = ppu.scanline*256;
		for(u32 i = 0; i < 256; ++i, ++scoffs)
		{
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }

			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }		
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.ts & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
		}
		return;
	}
	
	if( (ppu.bgmode&7) == 2 )
	{
		u8 re = ppu.tm | ppu.ts;
		if( re & 1 ) render_bg(bg1, 4, 1);
		if( re & 2 ) render_bg(bg2, 4, 2);
		render_sprites(spr);
		
		u32 scoffs = ppu.scanline*256;
		for(u32 i = 0; i < 256; ++i, ++scoffs)
		{
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }

			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }		
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.ts & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
		}
		return;
	}
	
	if( (ppu.bgmode&7) == 0 )
	{
		u8 re = ppu.tm|ppu.ts;
		if( re & 1 ) render_bg(bg1, 2, 1);
		if( re & 2 ) render_bg(bg2, 2, 2);
		if( re & 4 ) render_bg(bg3, 2, 3);
		if( re & 8 ) render_bg(bg4, 2, 4);
		if( re & 16 ) render_sprites(spr);
		
		u32 scoffs = ppu.scanline*256;	
		for(u32 i = 0; i < 256; ++i, ++scoffs)
		{
			if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( bg1[i] && (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( bg2[i] && (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }

			if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			
			if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( bg3[i] && (bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( bg4[i] && (bg4[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg4[i]); continue; }

			if( spr[i] && (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( bg3[i] && !(bg3[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg3[i]); continue; }
			if( bg4[i] && !(bg4[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg4[i]); continue; }
		}
		return;
	}
	
	if( (ppu.bgmode&7) == 4 )
	{
		u8 re = ppu.tm | ppu.ts;
		if( re & 1 ) render_bg(bg1, 8, 1);
		if( re & 2 ) render_bg(bg2, 2, 2);
		render_sprites(spr);
		
		u32 scoffs = ppu.scanline*256;
		for(u32 i = 0; i < 256; ++i, ++scoffs)
		{
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			
			if( ppu.tm & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			
			if( ppu.tm & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.tm & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }

			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x300 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( (bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }		
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x200 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( (bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
			if( ppu.ts & 16 ) if( (spr[i]&0x300)==0x100 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 1 ) if( bg1[i] && !(bg1[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg1[i]); continue; }
			if( ppu.ts & 16 ) if( spr[i] && (spr[i]&0x300)==0x000 ) { fbuf[scoffs] = pal2c16(spr[i]); continue; }
			if( ppu.ts & 2 ) if( bg2[i] && !(bg2[i]&BIT(13)) ) { fbuf[scoffs] = pal2c16(bg2[i]); continue; }
		}
		return;
	}

	
	if( (ppu.bgmode&7) == 7 )
	{
		if( (ppu.tm|ppu.ts)&16 ) render_sprites(spr);
	
		int m7x = s16(ppu.m7x<<3)>>3;
		int m7y = s16(ppu.m7y<<3)>>3;
	
		u16* v16 = (u16*)&ppu.vram[0];
		for(u32 x = 0; x < 256; ++x)
		{
			s16 oX = x + ppu.m7hofs - m7x;
			s16 oY = ppu.scanline + ppu.m7vofs - m7y;
			
			s16 X = ((ppu.m7a*oX + ppu.m7b*oY)>>8) + m7x;
			s16 Y = ((ppu.m7c*oX + ppu.m7d*oY)>>8) + m7y;
			
			u8 mapentry = 0;
			if( ! ( (X<0||X>0x3ff||Y<0||Y>0x3ff) && (ppu.m7sel & 0x80) ) )
			{
				X &= 0x3ff;
				Y &= 0x3ff;
				mapentry = v16[((X/8) + (Y/8)*128)&0x7fff];
			}
			u8 p = v16[(mapentry*64 + (Y&7)*8 + (X&7))&0x7fff] >> 8;
			if( ((ppu.tm|ppu.ts)&16) && spr[x] ) fbuf[ppu.scanline*256 + x] = pal2c16(spr[x]);
			else 
				fbuf[ppu.scanline*256 + x] = pal2c16(p);
		}
		return;
	}
	
	std::println("Unimpl bg mode {}", ppu.bgmode&7);
	//exit(1);	
}

void snes::ppu_mc(s64 mc)
{
	ppu.master_cycles += mc;
	if( ppu.master_cycles < 1364 ) return;
	ppu.master_cycles -= 1364;

	if( ppu.scanline == 0 )
	{
		for(u32 b = 0; b < 8; ++b) 
		{ // setup the hdma regs for the next frame, not clear what should happen here
			if( !(io.hdmaen & BIT(b)) ) continue;
			const u8 rb = b<<4;
			ppu.dmaregs[rb|8] = ppu.dmaregs[rb|2];
			ppu.dmaregs[rb|9] = ppu.dmaregs[rb|3];
			ppu.dmaregs[rb|0xA] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
			ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
			if( ppu.dmaregs[rb|0] & BIT(6) )
			{
				ppu.dmaregs[rb|5] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
				ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
				ppu.dmaregs[rb|6] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
				ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
			}
		}
	}

	if( ppu.scanline < 224 ) 
	{
		for(u32 b = 0; b < 8; ++b) { if( io.hdmaen & BIT(b) ) { hdma_run(b); } }
		ppu_draw_scanline();
	}
		
	if( (io.nmitimen&0x20) && ((io.vtimeh<<8)|io.vtimel)==ppu.scanline )
	{
		io.timeup |= 0x80;
		cpu.irq_line = true;
		//std::println("irq raised on vtime {} == scanline {}", ((io.vtimeh<<8)|io.vtimel), ppu.scanline);
	}
	
	if( ppu.scanline == 224 )
	{
		if( io.nmitimen & 0x80 ) { cpu.nmi_line = true; }
		ppu.rdnmi |= 0x80;
		ppu.internal_oamadd = ppu.oamadd<<1;
	}
	
	if( ppu.scanline == 261 )
	{
		//std::println("---");
		ppu.frame_complete = true;
		ppu.scanline = 0;
		ppu.oam_highest_pri = ((ppu.oamaddh&0x80) ? ((ppu.oamaddl>>1)&0x7f) : 0);
		//if( ppu.oam_highest_pri ) std::println("hipri=${:X}", ppu.oam_highest_pri);
		//std::println("BgMode = {}", ppu.bgmode&7);
		ppu.rdnmi &= ~0x80;
	} else {
		ppu.scanline++;
	}
	
}

static const u32 bytes_in_unit[] = { 1,2,2,4,4,4,2,4 };

void snes::hdma_xfer(u32 chan)
{
	const u8 rb = chan<<4;
	u8 bank = (ppu.dmaregs[rb|0]&BIT(6)) ? ppu.dmaregs[rb|7] : ppu.dmaregs[rb|4];
	u16 src_addr = (ppu.dmaregs[rb|0]&BIT(6)) ? (ppu.dmaregs[rb|6]<<8)|ppu.dmaregs[rb|5] : (ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8];
	
	for(u32 i = 0; i < bytes_in_unit[ppu.dmaregs[rb|0]&7]; ++i)
	{
		u8 bbus = ppu.dmaregs[rb|1];
		u8 v = read((bank<<16)|src_addr);
		//std::println("hdma ${:X}(i{}) = ${:X}", 0x2100+bbus, i, v);
		switch( ppu.dmaregs[rb|0]&7 )
		{
		case 2: case 6: case 0: io_write(0,0x2100+bbus, v); break;
		case 1: io_write(0,0x2100+u8(bbus+i), v); break;
		case 3: case 7: io_write(0,0x2100+u8(bbus+((i>>1)&1)), v); break;
		case 4: io_write(0,0x2100+u8(bbus+(i&3)), v); break;
		case 5: io_write(0,0x2100+u8(bbus+(i&1)), v); break;
		}
		src_addr += 1;
	}
	
	if( ppu.dmaregs[rb|0] & BIT(6) )
	{
		ppu.dmaregs[rb|5] = src_addr;
		ppu.dmaregs[rb|6] = src_addr>>8;
	} else {
		ppu.dmaregs[rb|8] = src_addr;
		ppu.dmaregs[rb|9] = src_addr>>8;
	}
}

void snes::hdma_run(u32 chan)
{
	const u8 rb = chan<<4;
	if( ppu.dmaregs[rb|0xA] == 0 ) return; // seeing a zero from the table means this channel is done for the frame
	ppu.dmaregs[rb|0xA] -= 1;
	
	if( ppu.scanline == 0 || (ppu.dmaregs[rb|0xA]&0x80) )
	{ // first scanline always transfers, as does repeat mode
		hdma_xfer(chan);
	}		
	
	// both modes only grab next item from hdma table upon hitting zero
	if( (ppu.dmaregs[rb|0xA]&0x7F) != 0 ) return;
	
	// grab next line count / repeat indicator
	ppu.dmaregs[rb|0xA] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
	ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
	// if indirect mode, grab pointer
	if( ppu.dmaregs[rb|0] & BIT(6) )
	{
		ppu.dmaregs[rb|5] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
		ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
		ppu.dmaregs[rb|6] = read((ppu.dmaregs[rb|4]<<16)|(ppu.dmaregs[rb|9]<<8)|ppu.dmaregs[rb|8]);
		ppu.dmaregs[rb|8]+=1; if( ppu.dmaregs[rb|8] == 0 ) ppu.dmaregs[rb|9]+=1;
	}
	
	// if this is not repeat mode, do transfer. won't get here again until line count is zero
	if( (ppu.dmaregs[rb|0xA] & 0x80) == 0 )
	{
		hdma_xfer(chan);
		ppu.dmaregs[rb|0xA] -= 1; // fixes F-Zero and the star sprites in krom's Star Wars demo, but breaks latter's text crawl
	}

}



















