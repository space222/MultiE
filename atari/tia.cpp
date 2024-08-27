#include <cstdio>
#include <cstdlib>
#include "itypes.h"
#include "console.h"

extern console* sys;

#define VBLANK 1
#define WSYNC  2
#define PF0   0xD
#define PF1   0xE
#define PF2   0xF
#define CTRLPF  0xA
#define COLUBK 9
#define COLUPF 8
#define COLUP1 7
#define COLUP0 6
#define RESP0 0x10
#define RESP1 0x11
#define RESM0 0x12
#define RESM1 0x13
#define RESBL 0x14
#define RESMP0 0x28
#define RESMP1 0x29

#define HMP0 0x20
#define HMP1 0x21
#define HMM0 0x22
#define HMM1 0x23
#define HMBL 0x24
#define HMOVE 0x2A
#define HMCLR 0x2B
#define ENAM0 0x1D
#define ENAM1 0x1E
#define ENABL 0x1F

#define NUSIZ0 4
#define NUSIZ1 5
#define GRP0 0x1B
#define GRP1 0x1C
#define REFP0 0xB
#define REFP1 0xC

#define CXM0P 0x30
#define CXM1P 0x31
#define CXP0FB 0x32
#define CXP1FB 0x33
#define CXM0FB 0x34
#define CXM1FB 0x35
#define CXBLPF 0x36
#define CXPPMM 0x37

#define CXCLR 0x2C

#define INPT4 0x3C

extern u32 palette[];

bool tia_frame_ready = false;
bool tia_wsync = false;
bool tia_vblank_complete = false;

static int scanline = 0;
static int dot = 0;
static u8 TIA[0x100] = {0};

static u8 p0h, p1h, blh, m0h, m1h;

static u8 hmove_add = 0;

int playfield(int pix)
{
	int pf = 0;
	if( pix >= 20 )
	{
		int p = pix-20;
		if( TIA[CTRLPF] & 1 )
		{
			if( p < 8 )
			{
				pf = (TIA[PF2]>>(7-p))&1;
			} else if( p < 16 ) {
				pf = (TIA[PF1]>>(p-8))&1;
			} else {
				pf = (TIA[PF0]>>(3^(4+(p-16))))&1;
			}	
		} else {
			if( p < 4 )
			{
				pf = (TIA[PF0]>>(4+p))&1;
			} else if( p < 12 ) {
				pf = (TIA[PF1]>>(7-(p-4)))&1;
			} else {
				pf = (TIA[PF2]>>(p-12))&1;
			}
		}
	} else {
		if( pix < 4 )
		{
			pf = (TIA[PF0]>>(4+pix))&1;
		} else if( pix < 12 ) {
			pf = (TIA[PF1]>>(7-(pix-4)))&1;
		} else {
			pf = (TIA[PF2]>>(pix-12))&1;
		}	
	}
	return pf;
}

int missile0(int pos)
{
	if( (TIA[ENAM0]&2) == 0 ) return 0;
	int sz = 1<<((TIA[NUSIZ0]>>4)&3);
	int p = pos - m0h;
	return ( p >= 0 && p < sz ) ? 1 : 0;
}

int missile1(int pos)
{
	if( (TIA[ENAM1]&2) == 0 ) return 0;
	int sz = 1<<((TIA[NUSIZ1]>>4)&3);
	int p = pos - m1h;
	return ( p >= 0 && p < sz ) ? 1 : 0;
}

int ball(int pos)
{
	if( (TIA[ENABL]&2) == 0 ) return 0;
	int sz = 1<<((TIA[CTRLPF]>>4)&3);
	int p = pos - blh;
	return ( p >= 0 && p < sz ) ? 1 : 0;
}

int player0(int pos)
{
	u8 data = TIA[GRP0];
	int p = pos - p0h;
	if( p >= 0 && p < 8 )
	{
		if( TIA[REFP0] & 8 ) p ^= 7;
		return (data>>(7^p))&1;
	}
	return 0;
}

int player1(int pos)
{
	u8 data = TIA[GRP1];
	int p = pos - p1h;
	if( p >= 0 && p < 8 )
	{
		if( TIA[REFP1] & 8 ) p ^= 7;
		return (data>>(7^p))&1;
	}
	return 0;
}

void tia_draw()
{
	int pos = dot-68;
	int pix = pos>>2;
	
	int pf = playfield(pix);
	int m0 = missile0(pos);
	int m1 = missile1(pos);
	int bl = ball(pos);
	int p0 = player0(pos);
	int p1 = player1(pos);
	
	if( m0 && p1 ) TIA[CXM0P] |= 0x80;
	if( m0 && p0 ) TIA[CXM0P] |= 0x40;
	if( m1 && p0 ) TIA[CXM1P] |= 0x80;
	if( m1 && p1 ) TIA[CXM1P] |= 0x40;
	if( p0 && pf ) TIA[CXP0FB]|= 0x80;
	if( p0 && bl ) TIA[CXP0FB]|= 0x40;
	if( p1 && pf ) TIA[CXP1FB]|= 0x80;
	if( p1 && bl ) TIA[CXP1FB]|= 0x40;
	if( m0 && pf ) TIA[CXM0FB]|= 0x80;
	if( m0 && bl ) TIA[CXM0FB]|= 0x40;
	if( m1 && pf ) TIA[CXM1FB]|= 0x80;
	if( m1 && bl ) TIA[CXM1FB]|= 0x40;
	if( p0 && p1 ) TIA[CXPPMM]|= 0x80;
	if( m0 && m1 ) TIA[CXPPMM]|= 0x40;
	if( bl && pf ) TIA[CXBLPF]|= 0x80;
	
	u32 color = 0;
	if( TIA[CTRLPF] & 4 )
	{
		if( pf || bl )
		{
			color = palette[TIA[COLUPF]>>1]<<8;
		} else if( p0 || m0 ) {
			color = palette[TIA[COLUP0]>>1]<<8;
		} else if( p1 || m1 ) {
			color = palette[TIA[COLUP1]>>1]<<8;
		} else {
			color = palette[TIA[COLUBK]>>1]<<8;
		}
	} else {
		if( m0 || p0 )
		{
			color = palette[TIA[COLUP0]>>1]<<8;
		} else if( m1 || p1 ) {
			color = palette[TIA[COLUP1]>>1]<<8;
		} else if( pf || bl ) {
			color = palette[TIA[COLUPF]>>1]<<8;
		} else {
			color = palette[TIA[COLUBK]>>1]<<8;
		}
	}

	((u32*)sys->framebuffer())[scanline*320 + pos*2] =
	((u32*)sys->framebuffer())[scanline*320 + pos*2 + 1] = color;
}

void hmove()
{
	m0h += -(s8(TIA[HMM0])>>4); if( m0h >= 160 ) m0h -= 160;
	m1h += -(s8(TIA[HMM1])>>4); if( m1h >= 160 ) m1h -= 160;
	p0h += -(s8(TIA[HMP0])>>4); if( p0h >= 160 ) p0h -= 160;
	p1h += -(s8(TIA[HMP1])>>4); if( p1h >= 160 ) p1h -= 160;
	blh += -(s8(TIA[HMBL])>>4); if( blh >= 160 ) blh -= 160;
}

void tia_write(u16 addr, u8 val)
{
	addr &= 0xff;
	//printf("TIA $%02X = $%02X\n", addr, val);

	u8 old = TIA[addr];
	TIA[addr] = val;
	switch( addr )
	{
	case VBLANK: 
		if( (old&2) == 2 && (val&2) == 0 ) 
		{
			tia_frame_ready = true; 
			tia_vblank_complete = true;	
		}
		break;
	case WSYNC: tia_wsync = true; break;
	
	
	case HMCLR: TIA[HMP0] = TIA[HMP1] = TIA[HMM0] = TIA[HMM1] = TIA[HMBL] = 0;
    		break;
    
	case RESP0:
		if( dot < 68 ) p0h = 3;
		else p0h = dot-68 + 8;
		break;
	case RESP1:
		if( dot < 68 ) p1h = 3;
		else p1h = dot-68 + 8;
		break;
	case RESM0:
		if( dot < 68 ) m0h = 2;
		else m0h = dot-68 + 8;
		break;
	case RESM1:
		if( dot < 68 ) m1h = 2;
		else m1h = dot-68 + 8;
		break;
	case RESBL:
		if( dot < 68 ) blh = 2;
		else blh = dot-68 + 8;
		break;

	case HMOVE:
		hmove_add = 8;
		hmove();
		break;
		
	case CXCLR:
		for(int i = CXM0P; i <= CXPPMM; ++i) TIA[i] = 0;	
		break;
	}
}

u8 tia_read(u16 addr)
{
	if( addr >= CXM0P && addr <= CXPPMM ) return TIA[addr];
	if( addr < 0xE ) return TIA[addr+0x30];
	if( addr >= 0x30 && addr <= 0x3D ) return TIA[addr];
	printf("Unimpl TIA Read $%02X\n", addr);
	return 0x00;//TIA[addr];
}

void tia_set_button(u8 p0, u8 p1)
{
	TIA[INPT4] = p0;
}

void tia_dot()
{
	if( scanline < 192 && dot > 67 )
	{
		tia_draw();	
	}
	
	dot++;
	if( dot == 228 )
	{
		if( tia_vblank_complete )
		{
			scanline = 0;
			tia_vblank_complete = false;
		} else {
			scanline++;
		}
		dot = 0;
		hmove_add = 0;
		tia_wsync = false;
	}
}

u32 palette[] = {
0x000000,0x404040,0x6c6c6c,0x909090,0xb0b0b0,0xc8c8c8,0xdcdcdc,0xf4f4f4,
0x444400,0x646410,0x848424,0xa0a034,0xb8b840,0xd0d050,0xe8e85c,0xfcfc68,
0x702800,0x844414,0x985c28,0xac783c,0xbc8c4c,0xcca05c,0xdcb468,0xecc878, 
0x841800,0x983418,0xac5030,0xc06848,0xd0805c,0xe09470,0xeca880,0xfcbc94, 
0x880000,0x9c2020,0xb03c3c,0xc05858,0xd07070,0xe08888,0xeca0a0,0xfcb4b4, 
0x78005c,0x8c2074,0xa03c88,0xb0589c,0xc070b0,0xd084c0,0xdc9cd0,0xecb0e0, 
0x480078,0x602090,0x783ca4,0x8c58b8,0xa070cc,0xb484dc,0xc49cec,0xd4b0fc, 
0x140084,0x302098,0x4c3cac,0x6858c0,0x7c70d0,0x9488e0,0xa8a0ec,0xbcb4fc, 
0x000088,0x1c209c,0x3840b0,0x505cc0,0x6874d0,0x7c8ce0,0x90a4ec,0xa4b8fc, 
0x00187c,0x1c3890,0x3854a8,0x5070bc,0x6888cc,0x7c9cdc,0x90b4ec,0xa4c8fc, 
0x002c5c,0x1c4c78,0x386890,0x5084ac,0x689cc0,0x7cb4d4,0x90cce8,0xa4e0fc, 
0x003c2c,0x1c5c48,0x387c64,0x509c80,0x68b494,0x7cd0ac,0x90e4c0,0xa4fcd4, 
0x003c00,0x205c20,0x407c40,0x5c9c5c,0x74b474,0x8cd08c,0xa4e4a4,0xb8fcb8, 
0x143800,0x345c1c,0x507c38,0x6c9850,0x84b468,0x9ccc7c,0xb4e490,0xc8fca4, 
0x2c3000,0x4c501c,0x687034,0x848c4c,0x9ca864,0xb4c078,0xccd488,0xe0ec9c, 
0x442800,0x644818,0x846830,0xa08444,0xb89c58,0xd0b46c,0xe8cc7c,0xfce08c
};






