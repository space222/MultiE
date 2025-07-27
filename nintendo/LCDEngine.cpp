#include <print>
#include <cstring>
#include "LCDEngine.h"

#define DISPCNT regs[0]
#define BG0CNT regs[4]
#define BG1CNT regs[5]
#define BG2CNT regs[6]
#define BG3CNT regs[7]

inline u32 c16to32(u16 c)
{
	u32 R = (c&0x1f)<<(24+3);
	u32 G = ((c>>5)&0x1f)<<(16+3);
	u32 B = ((c>>10)&0x1f)<<(8+3);
	return R|G|B;
}

void LCDEngine::draw_scanline(u32 L)
{
	if( ! (regs[0] & BIT(7)) )
	{
		memset(objwin, 0, 240);
		memset(spr, 0, 240);
		switch( regs[0] & 7 )
		{
		case 0: draw_mode_0(L); return;
		case 1: draw_mode_1(L); return;
		case 2: draw_mode_2(L); return;
		case 3: draw_mode_3(L); return;
		case 4: draw_mode_4(L); return;
		case 5: draw_mode_5(L); return;
		default: break;
		}
	}	
	memset(fbuf+L*240, 0, 240*4);
}

void LCDEngine::clear_bg(int ind)
{
	memset(&bg[ind][0], 0, 256);
}


static const int sprw[] = { 8,16,32,64 , 16,32,32,64 , 8,8,16,32 , 0,0,0,0 };
static const int sprh[] = { 8,16,32,64 , 8,8,16,32 ,  16,32,32,64, 0,0,0,0 };

void LCDEngine::render_sprites(int Y)
{
	if( !(DISPCNT & BIT(12)) ) return;
	memset(spr_pri, 0, 240);
	
	for(int S = 0; S < 128; ++S)
	{
		u16 attr0 = *(u16*)&oam[S*8];
		u16 attr1 = *(u16*)&oam[S*8 + 2];
		u16 attr2 = *(u16*)&oam[S*8 + 4];
		if( !(attr0 & BIT(8)) && (attr0 & BIT(9)) ) continue;
		if( (attr2&0x3ff)<=512 && (DISPCNT&7)>=3 ) continue;
		int sY = attr0 & 0xff;
		if( sY > 159 ) sY = (s8)(attr0 & 0xff);
		int H = sprh[((attr0>>14)&3)*4 + ((attr1>>14)&3)];
		//if( H == 0 ) continue;
		int sLine = Y - sY;
		if( sLine < 0 || sLine >= H ) continue;
		if( attr1 & BIT(13) ) sLine = H-sLine-1;

		int W = sprw[((attr0>>14)&3)*4 + ((attr1>>14)&3)];
		if( W == 0 ) continue;
		
		int sX = (attr1 & 0x1ff);
		if( attr1 & 0x100 ) sX |= ~0x1ff;
		
		int tilesize = ((attr0&BIT(13))?64:32);
		
		for(int x = 0; x < W; ++x)
		{
			if( sX+x < 0 || sX+x >= 240 || spr[sX+x] ) continue;
			int uX = (attr1 & BIT(12)) ? W-x-1 : x;
			u32 charbase = 0x10000 + ((attr2&0x3ff)*32);
			if( DISPCNT & BIT(6) )
			{
				charbase += (((sLine/8)*(W/8))*tilesize);
				//charbase += (uX/8)*tilesize;
			} else {
				charbase += ((sLine/8)*1024);
				//charbase += (uX/8)*tilesize;
			}
			charbase += (uX/8)*tilesize;
		
			u8 p = 0;
			
			if( (attr0&BIT(13)) )
			{
				p = VRAM[charbase + (sLine&7)*8 + (uX&7)];
			} else {
				p = VRAM[charbase + (sLine&7)*4 + ((uX&7)>>1)] >> (4*(uX&1));
				p &= 15;
				if( p ) p |= (attr2>>8)&0xf0;
			}
			if( ((attr0>>10)&3) == 2 )
			{
				objwin[x] = p?1:0;
				return;
			}
			spr[sX+x] = p;
			spr_pri[sX+x] = (attr2>>10)&3;
		}
	}
}

void LCDEngine::render_text_bg(u32 Y, u32 bgind)
{
	const u16 bgctrl = regs[4+bgind];
	u32 mapbase = ((bgctrl>>8)&0x1f)*0x800;
	u32 charbase = ((bgctrl>>2)&3)*0x4000;
	u32 tilesize = (bgctrl&BIT(7)) ? 64 : 32;
	
	const u32 scrollx = regs[8 + (bgind*2)] & 0x1ff;
	const u32 scrolly = regs[9 + (bgind*2)] & 0x1ff;
	const int W = (bgctrl&BIT(14)) ? 512 : 256;
	const int H = (bgctrl&BIT(15)) ? 512 : 256;
	Y += scrolly;
	Y &= (H-1);
	mapbase += (Y > 255) ? 0x800*2 : 0;
	Y &= 255;
	
	for(u32 x = 0; x < 240; ++x)
	{
		u32 mX = (x + scrollx) & (W-1);
		u32 mXoffs = ((mX > 255) ? 0x800 : 0);
		mX &= 255;	
		
		u16 entry = *(u16*)&VRAM[mapbase + mXoffs + ((Y/8)*32 + (mX/8))*2];
		u32 toffs = (entry&0x3ff)*tilesize;
		
		u8 tY = (Y&7) ^ ((entry&BIT(11)) ? 7 : 0);
		u8 tX = (mX&7) ^ ((entry&BIT(10)) ? 7 : 0);
			
		u8 p = 0;
		if( tilesize == 64 )
		{
			p = VRAM[charbase + toffs + (tY&7)*8 + (tX&7)];
		} else {
			p = VRAM[charbase + toffs + (tY&7)*4 + ((tX&7)>>1)];
			p = (p>>((tX&1)*4))&15;
			if( p ) p |= (entry>>8)&0xf0;
		}
		bg[bgind][x] = p;	
	}
}

void LCDEngine::draw_mode_0(u32 Y)
{
	const u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;
	
	render_sprites(Y);
			
	if( DISPCNT & BIT(8) )  render_text_bg(Y, 0); else clear_bg(0);
	if( DISPCNT & BIT(9) ) 	render_text_bg(Y, 1); else clear_bg(1);
	if( DISPCNT & BIT(10) ) render_text_bg(Y, 2); else clear_bg(2);
	if( DISPCNT & BIT(11) ) render_text_bg(Y, 3); else clear_bg(3);
		
	for(u32 x = 0; x < 240; ++x)
	{
		u16 pris[10] = {0};
		pris[(spr_pri[x]+1)<<1] = spr[x] ? (512 + (spr[x]<<1)) : 0;
		if( bg[3][x] ) pris[(((BG3CNT&3)+1)<<1)|1] = bg[3][x]<<1;
		if( bg[2][x] ) pris[(((BG2CNT&3)+1)<<1)|1] = bg[2][x]<<1;
		if( bg[1][x] ) pris[(((BG1CNT&3)+1)<<1)|1] = bg[1][x]<<1;
		if( bg[0][x] ) pris[(((BG0CNT&3)+1)<<1)|1] = bg[0][x]<<1;
		
		for(u32 i = 2; i < 10; ++i)
		{
			if( pris[i] )
			{
				fbuf[240*Y + x] = c16to32(*(u16*)&palette[pris[i]]);
				break;
			}
		}
	}
}

void LCDEngine::draw_mode_1(u32 Y)
{
	u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;

	render_sprites(Y);
			
	if( DISPCNT & BIT(8) )  render_text_bg(Y, 0);
	if( DISPCNT & BIT(9) ) 	render_text_bg(Y, 1);
	//if( DISPCNT & BIT(10) ) render_affine_bg(Y, 2);
		
	for(u32 x = 0; x < 240; ++x)
	{
		u16 pris[10] = {0};
		pris[(spr_pri[x]+1)<<1] = spr[x] ? (512 + (spr[x]<<1)) : 0;
		//pris[(((BG2CNT&3)+1)<<1)|1] = bg[2][x]<<1;
		pris[(((BG1CNT&3)+1)<<1)|1] = bg[1][x]<<1;
		pris[(((BG0CNT&3)+1)<<1)|1] = bg[0][x]<<1;
		
		for(u32 i = 2; i < 10; ++i)
		{
			if( pris[i] )
			{
				fbuf[240*Y + x] = c16to32(*(u16*)&palette[pris[i]]);
				break;
			}
		}
	}
}

void LCDEngine::draw_mode_2(u32 Y)
{
	u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;

	render_sprites(Y);
	for(u32 x = 0; x < 240; ++x)
		if( spr[x] )
			fbuf[240*Y + x] = c16to32(*(u16*)&palette[512 + (spr[x]<<1)]);
}

void LCDEngine::draw_mode_3(u32 Y)
{
	//u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = 0;
	render_sprites(Y);
	
	for(u32 x = 0; x < 240; ++x)
	{
		if( spr[x] && spr_pri[x] <= 2 )
		{
			fbuf[240*Y + x] = c16to32(*(u16*)&palette[512 + (spr[x]<<1)]);
		} else {
			fbuf[240*Y + x] = c16to32(*(u16*)&VRAM[(240*Y + x)*2]);
		}
	}
}

void LCDEngine::draw_mode_4(u32 Y)
{
	u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;
	render_sprites(Y);

	u32 base = (regs[0]&BIT(4)) ? 0xA000 : 0;
	for(u32 x = 0; x < 240; ++x)
	{
		u8 bpri = BG2CNT&3;
		u8 spri = spr_pri[x];
		
		u32 bp = c16to32(*(u16*)&palette[VRAM[base + 240*Y + x]<<1]);
		u32 sp = c16to32(*(u16*)&palette[512 + (spr[x]<<1)]);
		if( spr[x] && (spri <= bpri) )
		{
			fbuf[240*Y + x] = sp;
		} else if( VRAM[base + 240*Y + x] ) {
			fbuf[240*Y + x] = bp;
		} else if( spr[x] ) {
			fbuf[240*Y + x] = sp;
		}
	}
}

void LCDEngine::draw_mode_5(u32)
{
}


