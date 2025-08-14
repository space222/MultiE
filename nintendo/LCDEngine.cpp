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

struct layer
{
	u32 pri, n, c;
};

void LCDEngine::draw_scanline(u32 L)
{
	u32 Y = L;
	if( ! (regs[0] & BIT(7)) )
	{
		memset(objwin, 0, 240);
		memset(spr, 0, 240);
		memset(spr_pri, 9, 240);
		memset(spr_blend, 0, 240);
		switch( regs[0] & 7 )
		{
		case 0: draw_mode_0(L); break;
		case 1: draw_mode_1(L); break;
		case 2: draw_mode_2(L); break;
		case 3: draw_mode_3(L); return;
		case 4: draw_mode_4(L); break;
		case 5: draw_mode_5(L); return;
		default: return;
		}
		for(u32 x = 0; x < 240; ++x)
		{
			u32 layer_active = (DISPCNT >> 8) & 0x1f;
			u32 win0_y2 = regs[0x22]&0xff;
			u32 win0_y1 = regs[0x22]>>8;
			u32 win1_y2 = regs[0x23]&0xff;
			u32 win1_y1 = regs[0x23]>>8;
			if( win0_y2 > 160 || win0_y1 > win0_y2 ) win0_y2 = 160;
			if( win1_y2 > 160 || win1_y1 > win1_y2 ) win1_y2 = 160;
			
			u32 win0_x2 = regs[0x20]&0xff;
			u32 win0_x1 = regs[0x20]>>8;
			u32 win1_x2 = regs[0x21]&0xff;
			u32 win1_x1 = regs[0x21]>>8;
			if( win0_x2 > 240 || win0_x1 > win0_x2 ) win0_x2 = 240;
			if( win1_x2 > 240 || win1_x1 > win1_x2 ) win1_x2 = 240;
			bool do_blend = true;
			
			bool win0_in = (Y >= win0_y1) && (Y < win0_y2) && (x >= win0_x1) && (x < win0_x2);
			bool win1_in = (Y >= win1_y1) && (Y < win1_y2) && (x >= win1_x1) && (x < win1_x2);
			if( (DISPCNT&BIT(13)) && win0_in )
			{
				layer_active &= regs[0x24];
				do_blend = regs[0x24] & 0x20;
			} else if( (DISPCNT&BIT(14)) && win1_in ) {
				layer_active &= regs[0x24]>>8;
				do_blend = (regs[0x24]>>8)&0x20;	
			} else if( (DISPCNT&BIT(15)) && objwin[x] ) {
				layer_active &= regs[0x25]>>8;
				do_blend = (regs[0x25]>>8)&0x20;
			} else if( (DISPCNT&0xe000) ) {
				layer_active &= regs[0x25];
				do_blend = regs[0x25]&0x20;	
			}

			layer t[2] = {{.pri=15,.n=5,.c=0},{.pri=15,.n=5,.c=0}};
			if( (layer_active&8) && bg[3][x] ) 
			{
				u32 bgpri = BG3CNT&3;
				if( bgpri <= t[0].pri )
				{
					t[1] = t[0];
					t[0].pri = bgpri;
					t[0].c = bg[3][x]<<1;
					t[0].n = 3;
				} else if( bgpri <= t[1].pri ) {
					t[1].pri = bgpri;
					t[1].c = bg[3][x]<<1;
					t[1].n = 3;
				}
			}
			if( (layer_active&4) && bg[2][x] ) 
			{
				u32 bgpri = BG2CNT&3;
				if( bgpri <= t[0].pri )
				{
					t[1] = t[0];
					t[0].pri = bgpri;
					t[0].c = bg[2][x]<<1;
					t[0].n = 2;
				} else if( bgpri <= t[1].pri ) {
					t[1].pri = bgpri;
					t[1].c = bg[2][x]<<1;
					t[1].n = 2;
				}
			}
			if( (layer_active&2) && bg[1][x] ) 
			{
				u32 bgpri = BG1CNT&3;
				if( bgpri <= t[0].pri )
				{
					t[1] = t[0];
					t[0].pri = bgpri;
					t[0].c = bg[1][x]<<1;
					t[0].n = 1;
				} else if( bgpri <= t[1].pri ) {
					t[1].pri = bgpri;
					t[1].c = bg[1][x]<<1;
					t[1].n = 1;
				}
			}
			if( (layer_active&1) && bg[0][x] ) 
			{
				u32 bgpri = BG0CNT&3;
				if( bgpri <= t[0].pri )
				{
					t[1] = t[0];
					t[0].pri = bgpri;
					t[0].c = bg[0][x]<<1;
					t[0].n = 0;
				} else if( bgpri <= t[1].pri ) {
					t[1].pri = bgpri;
					t[1].c = bg[0][x]<<1;
					t[1].n = 0;
				}
			}
			if( (layer_active&0x10) && spr[x] )
			{
				u32 pri = spr_pri[x]&3;
				if( pri <= t[0].pri )
				{
					t[1] = t[0];
					t[0].pri = pri;
					t[0].c = 512 + (spr[x]<<1);
					t[0].n = 4;
				} else if( pri <= t[1].pri ) {
					t[1].pri = pri;
					t[1].c = 512 + (spr[x]<<1);
					t[1].n = 4;
				}
			}
			
			bool blended_sprite = ( t[0].n == 4 && spr_blend[x] && (regs[0x28]&BIT(8+t[1].n)) );
			u32 blend_mode = (blended_sprite ? 1 : ((regs[0x28]>>6)&3));
			
			if( !blended_sprite && (!do_blend || blend_mode==0 || !(regs[0x28]&BIT(t[0].n))) )
			{
				fbuf[L*240+x] = c16to32(*(u16*)&palette[t[0].c]);
				continue;
			}
			
			if( blend_mode == 2 )
			{
				u32 RGB = c16to32(*(u16*)&palette[t[0].c]);
				u8 R = RGB>>24;
				u8 G = RGB>>16;
				u8 B = RGB>>8;
				u32 cf = regs[0x2A]&0x1f;
				if( cf > 16 ) cf = 16;
				float coef = cf/16.f;
				R = R + (0xff-R)*coef;
				G = G + (0xff-G)*coef;
				B = B + (0xff-B)*coef;
				fbuf[L*240+x] = (R<<24)|(G<<16)|(B<<8);
				continue;
			}
			
			if( blend_mode == 3 )
			{
				u32 RGB = c16to32(*(u16*)&palette[t[0].c]);
				u8 R = RGB>>24;
				u8 G = RGB>>16;
				u8 B = RGB>>8;
				u32 cf = regs[0x2A]&0x1f;
				if( cf > 16 ) cf = 16;
				float coef = cf/16.f;
				R = R - R*coef;
				G = G - G*coef;
				B = B - B*coef;
				fbuf[L*240+x] = (R<<24)|(G<<16)|(B<<8);
				continue;
			}
			
			//got to blend_mode == 1
			if( !(regs[0x28]&BIT(8+t[1].n)) || (t[0].n == 5) )
			{ // if top layer is backdrop or second layer is not a targetB, output top pixel
				fbuf[L*240+x] = c16to32(*(u16*)&palette[t[0].c]);
				continue;
			}
			
			u32 c1RGB = c16to32(*(u16*)&palette[t[0].c]);
			u32 c2RGB = c16to32(*(u16*)&palette[t[1].c]);
			
			u8 R1 = c1RGB>>24;
			u8 G1 = c1RGB>>16;
			u8 B1 = c1RGB>>8;
			u8 R2 = c2RGB>>24;
			u8 G2 = c2RGB>>16;
			u8 B2 = c2RGB>>8;
			
			u32 cfa = regs[0x29] & 0x1f;
			if( cfa > 16 ) cfa = 16;
			float EVA = cfa/16.f;
			u32 cfb = (regs[0x29]>>8)&0x1f;
			if( cfb > 16 ) cfb = 16;
			float EVB = cfb/16.f;
			
			u8 R3 = std::min(255.f, R1*EVA + R2*EVB);
			u8 G3 = std::min(255.f, G1*EVA + G2*EVB);
			u8 B3 = std::min(255.f, B1*EVA + B2*EVB);
			
			fbuf[L*240+x] = (R3<<24)|(G3<<16)|(B3<<8);
		} // end of compositing for loop
		return;
	}
	memset(fbuf+L*240, 0, 240*4);
}

void LCDEngine::clear_bg(int ind)
{
	memset(&bg[ind][0], 0, 256);
}


static const int sprw[] = { 8,16,32,64 , 16,32,32,64 , 8,8,16,32 , 0,0,0,0 };
static const int sprh[] = { 8,16,32,64 , 8,8,16,32 ,  16,32,32,64, 0,0,0,0 };

void LCDEngine::render_affine_sprite(int Y, int S)
{
	u16 attr0 = *(u16*)&oam[S*8];
	u16 attr1 = *(u16*)&oam[S*8 + 2];
	u16 attr2 = *(u16*)&oam[S*8 + 4];
	u16 param_index = (attr1>>9)&0x1f;
	
	int sY = attr0 & 0xff;
	if( sY > 159 ) sY = (s8)(attr0 & 0xff);
	int H = sprh[((attr0>>14)&3)*4 + ((attr1>>14)&3)];
	if( H == 0 ) return;
	int dH = H * ((attr0&BIT(9)) ? 2: 1);
	int sLine = Y - sY;
	if( sLine < 0 || sLine >= dH ) return;
	
	int W = sprw[((attr0>>14)&3)*4 + ((attr1>>14)&3)];
	if( W == 0 ) return;
	int dW = W * ((attr0&BIT(9)) ? 2: 1);
	
	int sX = (attr1 & 0x1ff);
	if( attr1 & 0x100 ) sX |= ~0x1ff;
		
	int tilesize = ((attr0&BIT(13))?64:32);
	
	s32 PA = *(s16*)&oam[6 + param_index*32 + 0];
	s32 PB = *(s16*)&oam[6 + param_index*32 + 8];
	s32 PC = *(s16*)&oam[6 + param_index*32 + 16];
	s32 PD = *(s16*)&oam[6 + param_index*32 + 24];
	
	s32 sprx = (PB*(sLine - (dH/2)));
	s32 spry = (PD*(sLine - (dH/2)));
	
	u32 sprite_mode = ((attr0>>10)&3);
	u32 spri = (attr2>>10)&3;

	for(int x = 0; x < dW; ++x)
	{
		if( x+sX >= 240 ) break;
		if( x+sX < 0 || ( sprite_mode<2 && spr[x+sX] && spr_pri[sX+x]<=spri ) ) continue;
		
		s32 tx = (sprx + PA*(x-(dW/2)))>>8;
		s32 ty = (spry + PC*(x-(dW/2)))>>8;
		tx += W/2;
		ty += H/2;
		if( tx < 0 || ty < 0 || tx >= W || ty >= H ) continue;
		
		u32 charbase = 0x10000 + ((attr2&0x3ff)*32);
		if( DISPCNT & BIT(6) )
		{
			charbase += (((ty/8)*(W/8))*tilesize);
		} else {
			charbase += ((ty/8)*1024);
		}
		charbase += (tx/8)*tilesize;
	
		u8 p = 0;
		
		if( (attr0&BIT(13)) )
		{
			p = VRAM[charbase + (ty&7)*8 + (tx&7)];
		} else {
			p = VRAM[charbase + (ty&7)*4 + ((tx&7)>>1)] >> (4*(tx&1));
			p &= 15;
			if( p ) p |= (attr2>>8)&0xf0;
		}
		if( sprite_mode >= 2 )
		{
			objwin[sX+x] |= (p?1:0);
			continue;
		}
		if( p )
		{
			spr[sX+x] = p;
			spr_blend[sX+x] = ((sprite_mode==1) ? 1:0);
			spr_pri[sX+x] = spri;
		}
	}
}

void LCDEngine::render_sprites(int Y)
{
	if( !(DISPCNT & BIT(12)) ) return;
	
	for(int S = 0; S < 128; ++S)
	{
		u16 attr0 = *(u16*)&oam[S*8];
		u16 attr1 = *(u16*)&oam[S*8 + 2];
		u16 attr2 = *(u16*)&oam[S*8 + 4];
		u32 sprite_mode = ((attr0>>10)&3);
		if( sprite_mode == 3 ) continue;

		if( /*sprite_mode < 2 &&*/ !(attr0 & BIT(8)) && (attr0 & BIT(9)) ) continue;
		if( (attr2&0x3ff)<512 && (DISPCNT&7)>=3 ) continue;
		
		if( attr0 & BIT(8) )
		{
			render_affine_sprite(Y, S);
			continue;
		}
		
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
		u32 spri = (attr2>>10)&3;
		
		for(int x = 0; x < W; ++x)
		{
			if( sX+x >= 240 ) break;
			if( sX+x < 0 || (sprite_mode<2 && spr[sX+x] && spr_pri[sX+x]<=spri ) ) continue;
			int uX = (attr1 & BIT(12)) ? W-x-1 : x;
			u32 charbase = 0x10000 + ((attr2&0x3ff)*32);
			if( DISPCNT & BIT(6) )
			{
				charbase += (((sLine/8)*(W/8))*tilesize);
			} else {
				charbase += ((sLine/8)*1024);
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
			if( sprite_mode >= 2 )
			{
				objwin[sX+x] |= (p?1:0);
				continue;
			}
			if( p )
			{
				spr[sX+x] = p;
				spr_blend[sX+x] = ((sprite_mode==1) ? 1:0);
				spr_pri[sX+x] = spri;
			}
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
	mapbase += ((Y > 255) ? (0x800*(W==512?2:1)) : 0);
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

void LCDEngine::render_affine_bg(u32, u32 bgind)
{
	const u16 bgctrl = regs[4+bgind];
	u32 mapbase = ((bgctrl>>8)&0x1f)*0x800;
	u32 charbase = ((bgctrl>>2)&3)*0x4000;
	u32 tilesize = 64;
	
	int size = 0;
	switch( bgctrl>>14 )
	{
	case 0: size = 128; break;
	case 1: size = 256; break;
	case 2: size = 512; break;
	case 3: size = 1024; break;
	}
	
	s32 Y = ((bgind == 2) ? bg2y : bg3y);
	s32 X = ((bgind == 2) ? bg2x : bg3x);
	
	s16 deltaX = ((bgind == 2) ? regs[0x10] : regs[0x18]);
	s16 deltaY = ((bgind == 2) ? regs[0x12] : regs[0x20]);
	
	for(u32 x = 0; x < 240; ++x, X+=deltaX, Y+=deltaY)
	{
		bg[bgind][x] = 0;
		
		if( !(bgctrl & BIT(13)) && ((Y<0)||(X<0)||(Y>=size*256)||(X>=size*256)) ) continue;
		
		u32 mX = (X>>8) & (size-1);
		u32 mY = (Y>>8) & (size-1);
		
		u8 entry = VRAM[mapbase + (mY/8)*(size/8) + (mX/8)];
		
		u8 tY = (mY&7);
		u8 tX = (mX&7);
			
		bg[bgind][x] = VRAM[charbase + entry*tilesize + (tY&7)*8 + (tX&7)];	
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
}

void LCDEngine::draw_mode_1(u32 Y)
{
	const u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;
	
	render_sprites(Y);
			
	if( DISPCNT & BIT(8) )  render_text_bg(Y, 0); else clear_bg(0);
	if( DISPCNT & BIT(9) ) 	render_text_bg(Y, 1); else clear_bg(1);
	if( DISPCNT & BIT(10) ) render_affine_bg(Y, 2); else clear_bg(2);
	clear_bg(3);
}

void LCDEngine::draw_mode_2(u32 Y)
{
	const u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[Y*240+x] = backdrop;

	render_sprites(Y);
	clear_bg(0);
	clear_bg(1);
	if( DISPCNT & BIT(10) ) render_affine_bg(Y, 2); else clear_bg(2);
	if( DISPCNT & BIT(11) ) render_affine_bg(Y, 3); else clear_bg(3);
}

void LCDEngine::draw_mode_3(u32 Y)
{
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

void LCDEngine::draw_mode_4(u32 line)
{
	const u32 backdrop = c16to32(*(u16*)&palette[0]);
	for(u32 x = 0; x < 240; ++x) fbuf[line*240+x] = backdrop;
	render_sprites(line);
	
	s32 Y = bg2y; //((bgind == 2) ? bg2y : bg3y);
	s32 X = bg2x; //((bgind == 2) ? bg2x : bg3x);
	
	s16 deltaX = regs[0x10];// ((bgind == 2) ? regs[0x10] : regs[0x18]);
	s16 deltaY = regs[0x12];//((bgind == 2) ? regs[0x12] : regs[0x20]);
	u32 base = (regs[0]&BIT(4)) ? 0xA000 : 0;
	
	clear_bg(0);
	clear_bg(1);
	clear_bg(3);
	if( !(DISPCNT & BIT(10)) ) { clear_bg(2); return; }
	
	for(u32 x = 0; x < 240; ++x, X+=deltaX, Y+=deltaY)
	{
		bg[2][x] = 0;
		
		if( ((Y<0)||(X<0)||(Y>=160*256)||(X>=240*256)) ) continue;
		
		u32 mX = (X>>8);
		u32 mY = (Y>>8);
		
		bg[2][x] = VRAM[base + mY*240 + mX];
	}

/*	for(u32 x = 0; x < 240; ++x)
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
*/
}

void LCDEngine::draw_mode_5(u32)
{
}


