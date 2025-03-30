#include <cstdio>
#include <cstring>
#include <cstdlib>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include <array>
#include "types.h"

u16 gfxregs[50] = {0};
u32 framebuffer[240*160] = { 0xff };

extern u8 palette[0x400];
extern u8 OAM[0x400];
extern u8 VRAM[0x18000];

inline u32 color16to32(u16 rgb16)
{
	u32 R = rgb16&0x1f;
	u32 G = (rgb16>>5)&0x1f;
	u32 B = (rgb16>>10)&0x1f;
	return (B<<19)|(G<<11)|(R<<3)|(0xff<<24);
}

void gfx_write16(u32 addr, u16 val)
{
	switch( addr )
	{
	case 0x4000000: gfxregs[0] = val&~8; return;
	case 0x4000004:
		gfxregs[2] = (gfxregs[2]&7) | (val&0xfff8);
		return;
	case 0x4000006: return;
	default:
		gfxregs[(addr&0x7f)>>1] = val;
	}
	
	//printf("Gfx reg write16 %X = %X\n", addr, val);
	return;
}

u16 gfx_read16(u32 addr)
{
	u16 temp = gfxregs[(addr&0x7f)>>1];
	//printf("Gfx reg read16 %X\n", (addr&0x7f)>>1);
	return temp;
}

const int rotsize[4] = { 128, 256, 512, 1024 };

int getPixelForROTBG(int bg, int x, int y)
{
	int regbase = (bg == 2) ? 0x10 : 0x20;
	u16 ctrl = gfxregs[4 + bg];
	u32 tile_base = ((ctrl>>2)&3) * (16*1024);
	u32 map_addr = ((ctrl>>8)&0x1f) * 2048;

	int base_x = (gfxregs[regbase+5]<<16)|gfxregs[regbase+4];
	int base_y = (gfxregs[regbase+7]<<16)|gfxregs[regbase+6];
	base_x <<= 4;
	base_x >>= 4;
	base_y <<= 4;
	base_y >>= 4;
	float bX = x;// (base_x/256.f);
	float bY = y;//(base_y/256.f);
	float PA = ((s16)gfxregs[regbase])/256.f;
	float PB = ((s16)gfxregs[regbase+1])/256.f;
	float PC = ((s16)gfxregs[regbase+2])/256.f;
	float PD = ((s16)gfxregs[regbase+3])/256.f;
	
	int scr_x = (PA*bX + PB*bY) + (base_x/256.f);
	int scr_y = (PC*bX + PD*bY) + (base_y/256.f);
	
	if( ctrl&(1u<<13) )
	{
		int s = rotsize[(ctrl>>14)&3];
		if( scr_x < 0 ) scr_x = s + (scr_x%s);
		else if( scr_x >= s ) scr_x %= s;
		if( scr_y < 0 ) scr_y = s + (scr_y%s);
		else if( scr_y >= s ) scr_y %= s;
	} else {
		if( scr_x < 0 || scr_y < 0 
			|| scr_x >= rotsize[(ctrl>>14)&3] || scr_y >= rotsize[(ctrl>>14)&3] ) return 0;
	}
	
	u32 tile_x = scr_x/8;
	u32 tile_y = scr_y/8;
	map_addr += ((tile_y*(rotsize[(ctrl>>14)&3]>>3)) + tile_x);
	u16 map_entry = VRAM[map_addr];

	u16 char_index = map_entry & 0xff;
	u32 tile_start = tile_base + (char_index * 64);
	u8 pal = 0;
	scr_y &= 7;
	scr_x &= 7;
	tile_start += scr_y*8 + scr_x;
	pal = VRAM[tile_start];
	
	return pal;	
}

int getPixelForTXTBG(int bg, int x, int y)
{
	u16 ctrl = gfxregs[4 + bg];
	u32 scr_x = ((gfxregs[8 + (2*bg)] & 0x1ff) + x) & 0x1ff;
	u32 scr_y = ((gfxregs[9 + (2*bg)] & 0x1ff) + y) & 0x1ff;
	scr_x &= (ctrl&(1u<<14)) ? 0x1ff : 0xff;
	scr_y &= (ctrl&(1u<<15)) ? 0x1ff : 0xff;
	u32 tile_base = ((ctrl>>2)&3) * (16*1024);
	u32 map_addr = ((ctrl>>8)&0x1f) * 2048;

	// add some offsets, and then tweak the values so 
	// the rest of the code doesn't need to deal with screen base blocks
	if( scr_y > 0xff )
	{
		map_addr += (ctrl&(1u<<14)) ? 4096 : 2048; // width can be small when height is big
		scr_y &= 0xff;
	}
	if( scr_x > 0xff )
	{
		map_addr += 2048;
		scr_x &= 0xff;
	}
	
	// find the exact map entry address
	u32 tile_x = scr_x/8;
	u32 tile_y = scr_y/8;
	map_addr += ((tile_y*32) + tile_x) * 2; // values doubled because text backgrounds have 16bit tile entries
	u16 map_entry = 0;
	memcpy(&map_entry, VRAM+map_addr, 2);

	u16 char_index = map_entry & 0x3ff;
	u32 paltop = (map_entry >> 8) & 0xf0;  // might not get used if 256-color mode selected
	bool Hflip = (map_entry & (1u<<10) );
	bool Vflip = (map_entry & (1u<<11) );
	
	u32 tile_start = tile_base + (char_index * ((ctrl&0x80)? 64 : 32));
	u8 pal = 0;
	scr_y &= 7;
	scr_x &= 7;
	if( Hflip ) scr_x = 7 - scr_x;
	if( Vflip ) scr_y = 7 - scr_y;
	if( ctrl & 0x80 )
	{  // 256 color tiles
		tile_start += scr_y*8 + scr_x;
		pal = VRAM[tile_start];
	} else {
		tile_start += scr_y*4 + (scr_x>>1);
		pal = ((scr_x&1) ? (VRAM[tile_start]>>4) : (VRAM[tile_start]&0xf));
		if( pal ) pal |= paltop;
	}	

	return pal;
}

int x_sizes[] = { 8,16,32,64, 16,32,32,64, 8,8,16,32 };
int y_sizes[] = { 8,16,32,64, 8,8,16,32, 16,32,32,64 };

int getSpritesOnScanline(int scanline, std::array<int, 128>& slist)
{
	int num = 0;
	u16 addr = 0;
	for(int i = 0; i < 128; ++i)
	{
		u16 OAM0 = 0;
		u16 OAM1 = 0;
		memcpy(&OAM0, OAM+addr, 2);
		memcpy(&OAM1, OAM+addr+2, 2);
		addr += 8;
		if( !(OAM0 & 0x100) && (OAM0 & 0x200) ) continue; // non-affine sprite turned off
		
		u32 mode = (OAM0>>10)&3;
		if( mode == 3 ) continue;  // mode 3 makes sprite serve as a window, not supported here
		u32 shape = (OAM0>>14)&3;
		if( shape == 3 ) continue; // invalid shape setting, ignoring the sprite for now.

		int Y = (OAM0 & 0xff);
		if( Y > 159 ) Y = (s8)(OAM0 & 0xff);
		u32 size = (OAM1>>14)&3;
		size = (shape<<2)|size;
		int H = y_sizes[size];
		
		if( (OAM0 & 0x300) == 0x300 ) // double size affine sprite
		{
			H <<= 1;
		} 
		
		if( scanline >= Y && (Y+H) > scanline )  // used to be (Y+H)&0xff
		{
			slist[num++] = i;
		}		
	}
	
	return num;
}

bool getSpritePixel(std::array<int, 128>& slist, int num_sprites, int x, int scanline, u16& pixel, u32& pri)
{
	if( num_sprites == 0 ) return false;
	for(int i = 0; i < num_sprites; ++i)
	{
		u16 OAM0 = 0;
		u16 OAM1 = 0;
		u16 OAM2 = 0;
		memcpy(&OAM0, OAM+(slist[i]*8), 2);
		memcpy(&OAM1, OAM+(slist[i]*8)+2, 2);
		memcpy(&OAM2, OAM+(slist[i]*8)+4, 2);
		
		u32 curpri = (OAM2>>10)&3;
		if( curpri > pri ) continue;
		
		u32 shape = (OAM0>>14)&3;
		u32 size = (OAM1>>14)&3;
		size = (shape<<2)|size;
		int H = y_sizes[size];
		int W = x_sizes[size];
		
		int Y = (OAM0 & 0xff);
		if( Y > 159 ) Y = (s8)(OAM0 & 0xff);
		int X = (OAM1 & 0x1ff);
		X <<= 23;
		X >>= 23;
		int cW = W;
		if( (OAM0 & 0x300) == 0x300 ) // double size affine sprite
		{
			X += W/2;
			Y += H/2;
			cW <<= 1;
		}
		
		if( (OAM0 & 0x300) == 0x300 )
		{
			if( x < (X-(W/2)) || x >= (X+W+(W/2)) ) continue;
		} else {
			if( x < X || x >= (X+cW) ) continue;
		}
				
		int sprx = x-X;
		int spry = scanline-Y;
		
		if( OAM0 & 0x100 )
		{
			u32 pindex = (((OAM1>>9)&0x1f)<<5) + 6;
			s16 PA,PB,PC,PD;
			memcpy(&PA, OAM+pindex, 2);
			memcpy(&PB, OAM+pindex+8, 2);
			memcpy(&PC, OAM+pindex+16, 2);
			memcpy(&PD, OAM+pindex+24, 2);
			float fx = sprx - (W/2.f);
			float fy = spry - (H/2.f);
			float tX = (PA/256.f)*fx + (PB/256.f)*fy;
			float tY = (PC/256.f)*fx + (PD/256.f)*fy;
			fx = tX;
			fy = tY;
			sprx = (int)(fx + (W/2.f));
			spry = (int)(fy + (H/2.f));
			if( sprx >= W || spry >= H || sprx < 0 || spry < 0 ) continue;
		} else {
			if( OAM1 & (1u<<12) ) sprx = (W-1)-sprx;
			if( OAM1 & (1u<<13) ) spry = (H-1)-spry;
		}
		
		int tilex = sprx/8;
		int tiley = spry/8;
		int base_tile = (OAM2&0x3ff) * 32;		
		sprx &= 7;
		spry &= 7;
		const u32 tile_size = ((OAM0&(1u<<13)) ? 64 : 32);
		base_tile += tiley * ((gfxregs[0]&(1u<<6)) ? (tile_size * (W/8)) : 1024);
		base_tile += tile_size * tilex;
		base_tile += spry * ((OAM0&(1u<<13)) ? 8 : 4); // pixel row, 16 or 256 color
		base_tile += ((OAM0&(1u<<13)) ? sprx : (sprx>>1));
		u16 p = VRAM[base_tile + 0x10000];
		if( !(OAM0&(1u<<13)) )
		{
			p = (sprx&1) ? (p>>4) : (p&0xf);
			if( p ) p |= (OAM2>>8)&0xf0;
		}
		if( p == 0 ) continue;
		
		memcpy(&pixel, palette+512+(p<<1), 2);
		pri = curpri;
		
		return true;
	}

	return false;
}

void gfx_draw_line_mode_zero(const int scanline)
{
	std::array<int, 128> slist;
	u16 C = 0;
	int num_sprites = (gfxregs[0]&(1u<<12)) ? getSpritesOnScanline(scanline, slist) : 0;
	
	for(int x = 0; x < 240; ++x)
	{
		u8 bg_pal = 0;
		u32 bgpri = 4;
		u32 gfxctrl = gfxregs[0];
		//todo: a single impl. of windows can just mess with gfxctrl
		if( ((gfxctrl>>13)&3) )  // would be &7 when supporting objwin
		{
			int win0x1 = gfxregs[32]>>8;
			int win0x2 = (gfxregs[32]&0xff);
			if( win0x1 > win0x2 || win0x2 > 240 ) win0x2 = 240;
			int win0y1 = gfxregs[34]>>8;
			int win0y2 = (gfxregs[34]&0xff);
			if( win0y1 > win0y2 || win0y2 > 160 ) win0y2 = 160;
		
			int win1x1 = gfxregs[33]>>8;
			int win1x2 = (gfxregs[33]&0xff);
			if( win1x1 > win1x2 || win1x2 > 240 ) win1x2 = 240;
			int win1y1 = gfxregs[35]>>8;
			int win1y2 = (gfxregs[35]&0xff);
			if( win1y1 > win1y2 || win1y2 > 160 ) win1y2 = 160;
		
			if( (gfxctrl&(1u<<13)) && x >= win0x1 && x < win0x2
					     && scanline >= win0y1 && scanline < win0y2 )
			{
				gfxctrl &= (gfxregs[36]&0x1f)<<8;				
			} else if( (gfxctrl&(1u<<14)) && x >= win1x1 && x < win1x2
						&& scanline >= win1y1 && scanline < win1y2 ) {
				gfxctrl &= (gfxregs[36]&0x1f00);						
			} else {
				gfxctrl &= (gfxregs[37]&0x1f)<<8; //winout			
			}
		}
		if( gfxctrl&(1u<<8) )
		{
			u8 val = getPixelForTXTBG(0, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = gfxregs[4]&3;
			}
		}
		if( (gfxctrl&(1u<<9)) && (bg_pal == 0 || bgpri > (gfxregs[5]&3)) )
		{
			u8 val = getPixelForTXTBG(1, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = (gfxregs[5]&3);
			}
		}
		if( (gfxctrl&(1u<<10)) && (bg_pal == 0 || bgpri > (gfxregs[6]&3)) ) 
		{
			u8 val = getPixelForTXTBG(2, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = (gfxregs[6]&3);
			}
		}
		if( (gfxctrl&(1u<<11)) && (bg_pal == 0 || bgpri > (gfxregs[7]&3)) ) 
		{
			u8 val = getPixelForTXTBG(3, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = (gfxregs[7]&3);
			}
		}
		memcpy(&C, palette+(bg_pal<<1), 2);
		u16 Spixel = 0;
		u32 Spri = bgpri;
		if( num_sprites && (gfxctrl&(1u<<12)) && getSpritePixel(slist, num_sprites, x, scanline, Spixel, Spri) )
		{
			C = Spixel;
		}
		framebuffer[scanline*240 + x] = color16to32(C);
	}
	
	return;
}

void gfx_draw_line_mode_one(const int scanline)
{
	std::array<int, 128> slist;
	u16 C = 0;
	int num_sprites = (gfxregs[0]&(1u<<12)) ? getSpritesOnScanline(scanline, slist) : 0;

	for(int x = 0; x < 240; ++x)
	{
		u8 bg_pal = 0;
		u32 bgpri = 4;
		u32 gfxctrl = gfxregs[0];
		//todo: a single impl. of windows can just mess with gfxctrl
		if( ((gfxctrl>>13)&3) )  // would be &7 when supporting objwin
		{
			int win0x1 = gfxregs[32]>>8;
			int win0x2 = (gfxregs[32]&0xff);
			if( win0x1 > win0x2 || win0x2 > 240 ) win0x2 = 240;
			int win0y1 = gfxregs[34]>>8;
			int win0y2 = (gfxregs[34]&0xff);
			if( win0y1 > win0y2 || win0y2 > 160 ) win0y2 = 160;
		
			int win1x1 = gfxregs[33]>>8;
			int win1x2 = (gfxregs[33]&0xff);
			if( win1x1 > win1x2 || win1x2 > 240 ) win1x2 = 240;
			int win1y1 = gfxregs[35]>>8;
			int win1y2 = (gfxregs[35]&0xff);
			if( win1y1 > win1y2 || win1y2 > 160 ) win1y2 = 160;
		
			if( (gfxctrl&(1u<<13)) && x >= win0x1 && x < win0x2
					     && scanline >= win0y1 && scanline < win0y2 )
			{
				gfxctrl &= (gfxregs[36]&0x1f)<<8;				
			} else if( (gfxctrl&(1u<<14)) && x >= win1x1 && x < win1x2
						&& scanline >= win1y1 && scanline < win1y2 ) {
				gfxctrl &= (gfxregs[36]&0x1f00);						
			} else {
				gfxctrl &= (gfxregs[37]&0x1f)<<8; //winout			
			}
		}
		if( gfxctrl&(1u<<8) )
		{
			u8 val = getPixelForTXTBG(0, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = gfxregs[4]&3;
			}
		}
		if( (gfxctrl&(1u<<9)) && (bg_pal == 0 || bgpri > (gfxregs[5]&3)) )
		{
			u8 val = getPixelForTXTBG(1, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = (gfxregs[5]&3);
			}
		}
		if( (gfxctrl&(1u<<10)) && (bg_pal == 0 || bgpri > (gfxregs[6]&3)) )
		{
			u8 val = getPixelForROTBG(2, x, scanline);
			if( val )
			{
				bg_pal = val;
				bgpri = (gfxregs[6]&3);
			}
		}
		memcpy(&C, palette+(bg_pal<<1), 2);
		u16 Spixel = 0;
		u32 Spri = bgpri;
		if( (gfxctrl&(1u<<11)) && num_sprites && getSpritePixel(slist, num_sprites, x, scanline, Spixel, Spri) )
		{
			C = Spixel;
		}
		framebuffer[scanline*240 + x] = color16to32(C);
	}
	
	return;
}

void gfx_draw_scanline(const int scanline)
{
	//if( gfxregs[0] & 0x80 ) return; //forced blank
	u32 mode = gfxregs[0]&7;
	
	if( mode == 0 )
	{
		gfx_draw_line_mode_zero(scanline);
		return;
	}
	
	if( mode == 1 )
	{
		gfx_draw_line_mode_one(scanline);
		return;
	}
	
	if( mode == 2 )
	{
		std::array<int, 128> slist;
		u16 C = 0;
		int num_sprites = (gfxregs[0]&(1u<<12)) ? getSpritesOnScanline(scanline, slist) : 0;
		u16 BGC = 0;
		memcpy(&BGC, palette, 2);
		for(int x = 0; x < 240; ++x)
		{
			u16 Spixel = 0;
			u32 Spri = 4;
			C = BGC;
			if( num_sprites && getSpritePixel(slist, num_sprites, x, scanline, Spixel, Spri) )
			{
				C = Spixel;
			}
			framebuffer[scanline*240 + x] = color16to32(C);
		}
		return;
	}
	
	if( mode == 4 )
	{
		std::array<int, 128> slist;
		int num_sprites = (gfxregs[0]&(1u<<12)) ? getSpritesOnScanline(scanline, slist) : 0;

		u32 addr = (scanline*240) + ((gfxregs[0]&0x10) ? 0xA000 : 0);
		for(int x = 0; x < 240; ++x)
		{
			u8 P = VRAM[addr++];
			//if( P == 0 ) continue;
			u16 C = 0;
			memcpy(&C, palette+(P<<1), 2);
			u16 Spixel = 0;
			u32 Spri = 4;
			if( num_sprites && getSpritePixel(slist, num_sprites, x, scanline, Spixel, Spri) )
			{
				C = Spixel;
			}
			framebuffer[scanline*240 + x] = color16to32(C);
		}
		return;
	}
	if( mode == 3 )
	{
		u32 addr = scanline*480;
		std::array<int, 128> slist;
		int num_sprites = (gfxregs[0]&(1u<<12)) ? getSpritesOnScanline(scanline, slist) : 0;

		for(int x = 0; x < 240; ++x)
		{
			u16 C = 0;
			memcpy(&C, VRAM+addr, 2);
			addr += 2;
			u16 Spixel = 0;
			u32 Spri = 3;
			if( num_sprites && getSpritePixel(slist, num_sprites, x, scanline, Spixel, Spri) )
			{
				C = Spixel;
			}
			framebuffer[scanline*240 + x] = color16to32(C);
		}
		return;
	}
	if( mode == 5 )
	{
		if( scanline > 128 ) return;
		u32 addr = (scanline*320) + ((gfxregs[0]&0x10) ? 0xA000 : 0);
		u16 C = 0;
		memcpy(&C, palette, 2);
		u16 BGC = color16to32(C);
		for(int x = 0; x < 160; ++x)
		{
			memcpy(&C, VRAM+addr, 2);
			addr += 2;
			framebuffer[scanline*240 + x] = color16to32(C);			
		}
		for(int x = 160; x < 240; ++x) framebuffer[scanline*240 + x] = BGC;
		return;
	}	
	
	//for(int i = 0; i < 240; ++i) framebuffer[scanline*240 + i] = 0xff000000;
	return;
}


/*void gfx_draw_frame()
{
	//TODO: 1) sprites. 2) let the display be disabled. 
	u32 mode = gfxregs[0]&7;
	
	if( mode == 0 )
	{
		//gfx_draw_frame_mode_zero();
		return;
	}
	
	if( mode == 4 )
	{
		u32 addr = (gfxregs[0]&0x10) ? 0xA000 : 0;
		for(int y = 0; y < 160; ++y)
		{
			for(int x = 0; x < 240; ++x)
			{
				u8 P = VRAM[addr++];
				//if( P == 0 ) continue;
				u16 C = 0;
				memcpy(&C, palette+(P<<1), 2);
				u32 R = C&0x1f;
				u32 G = (C>>5)&0x1f;
				u32 B = (C>>10)&0x1f;
				framebuffer[y*240 + x] = (B<<19)|(G<<11)|(R<<3)|(0xff<<24);
			}
		}
		return;
	}
	if( mode == 3 )
	{
		u32 addr = 0;
		for(int y = 0; y < 160; ++y)
		{
			for(int x = 0; x < 240; ++x)
			{
				u16 C = 0;
				memcpy(&C, VRAM+addr, 2);
				addr += 2;
				u32 R = C&0x1f;
				u32 G = (C>>5)&0x1f;
				u32 B = (C>>10)&0x1f;
				framebuffer[y*240 + x] = (B<<19)|(G<<11)|(R<<3)|(0xff<<24);
			}
		}
		return;
	}
	if( mode == 5 )
	{
		u32 addr = (gfxregs[0]&0x10) ? 0xA000 : 0;
		u16 C = 0;
		memcpy(&C, palette, 2);
		u32 R = C&0x1f;
		u32 G = (C>>5)&0x1f;
		u32 B = (C>>10)&0x1f;
		u16 BGC = (B<<19)|(G<<11)|(R<<3)|(0xff<<24);
		for(int y = 0; y < 128; ++y)
		{
			for(int x = 0; x < 160; ++x)
			{
				C = 0;
				memcpy(&C, VRAM+addr, 2);
				addr += 2;
				R = C&0x1f;
				G = (C>>5)&0x1f;
				B = (C>>10)&0x1f;
				framebuffer[y*240 + x] = (B<<19)|(G<<11)|(R<<3)|(0xff<<24);			
			}
			for(int x = 160; x < 240; ++x) framebuffer[y*240 + x] = BGC;
		}
		for(int y = 128; y < 160; ++y)
		{
			for(int x = 0; x < 240; ++x) framebuffer[y*240+x] = BGC;
		}
		return;
	}
	
	
	for(int i = 0; i < 240*160; ++i) framebuffer[i] = 0xff000000;
	return;
}
*/





