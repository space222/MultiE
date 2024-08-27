#include <iostream>
#include <string>
#include <SDL.h>
#include "a52.h"

#define DMACTL 0
#define CHACTL 1
#define DLISTL 2
#define DLISTH 3
#define HSCROL 4
#define VSCROL 5
#define PMBASE 7
#define CHBASE 9
#define WSYNC  0xA
#define VCOUNT 0xB
#define PENH   0xC
#define PENV   0xD
#define NMIEN  0xE
#define MNIRES 0xF
#define NMIST  0xF

#define COLPF0 0x16
#define COLPF1 0x17
#define COLPF2 0x18
#define COLPF3 0x19
#define COLBK  0x1A

#define OP_NEW 0x100

extern u8 palette5200[];
extern console* sys;
bool antic_wsync = false;

static u32 scanline = 0;
static u8 aregs[16] = {0};
static u8 cregs[32] = {0};
static u16 antic_op = OP_NEW;
static u8 op_lines = 0;
static u8 next_dmactl = 0;
static u16 LMS = 0;

void antic_reset()
{
	antic_op = OP_NEW;
	scanline = 0;
	for(int i = 0; i < 16; ++i) aregs[i] = 0;
}

void antic_mode_D()
{
	u32* fb =(u32*) ((a52*)sys)->framebuffer();
	for(int i = 0; i < 160; ++i)
	{
		u8 b = ((a52*)sys)->RAM[LMS+(i>>2)];
		b >>= 6 - ((i&3)*2);
		b &= 3;
		switch( b )
		{
		case 0: b = cregs[COLBK]; break;
		case 1: b = cregs[COLPF0]; break;
		case 2: b = cregs[COLPF1]; break;
		case 3: b = cregs[COLPF2]; break;
		}
		u8 R = palette5200[b*3];
		u8 G = palette5200[b*3+1];
		u8 B = palette5200[b*3+2];
		fb[scanline*320 + i*2] = fb[scanline*320 + i*2 + 1] = (R<<24)|(G<<16)|(B<<8);
	}
}

void antic_mode_7()
{
	u32* fb =(u32*) ((a52*)sys)->framebuffer();
	
	for(int i = 0; i < 160; ++i)
	{
		u8 b = ((a52*)sys)->RAM[LMS+(i>>3)];
		b = ((a52*)sys)->BIOS[(op_lines>>1) + ((b&0x3F)*8)];
		b >>= 7 - (i&7);
		
		if( b&1 ) b = cregs[COLPF1];
		else b = cregs[COLBK];
		
		u8 R = palette5200[b*3];
		u8 G = palette5200[b*3+1];
		u8 B = palette5200[b*3+2];
		fb[scanline*320 + i*2] = fb[scanline*320 + i*2 + 1] = (R<<24)|(G<<16)|(B<<8);
	}	
}

void antic_mode_6()
{
	u32* fb =(u32*) ((a52*)sys)->framebuffer();
	
	for(int i = 0; i < 160; ++i)
	{
		u8 b = ((a52*)sys)->RAM[LMS+(i>>3)];
		b = ((a52*)sys)->BIOS[op_lines + ((b&0x3F)*8)];
		b >>= 7 - (i&7);
		
		if( b&1 ) b = cregs[COLPF1];
		else b = cregs[COLBK];
		
		u8 R = palette5200[b*3];
		u8 G = palette5200[b*3+1];
		u8 B = palette5200[b*3+2];
		fb[scanline*320 + i*2] = fb[scanline*320 + i*2 + 1] = (R<<24)|(G<<16)|(B<<8);
	}	
}

void antic_mode_4()
{


}

void antic_scanline()
{
	u16 dlist = aregs[DLISTL]|(aregs[DLISTH]<<8);
	if( (aregs[0] & 3) == 0 ) 
	{
		scanline++;
		if( scanline == 242 && (aregs[NMIEN]&0x40) )
		{
			aregs[NMIST] |= 0x40;
			((a52*)sys)->cpu.nmi();
		}
		if( scanline == 262 ) 
		{
			scanline = 0;
			if( antic_op == 0x41 ) antic_op = OP_NEW;
			aregs[DMACTL] = next_dmactl;
		}
		aregs[VCOUNT] = scanline>>1;
		return;
	}
	
	//printf("OP = $%X\n", antic_op);
	if( antic_op == OP_NEW )
	{
		op_lines = 0;
		bool got_one = false;
		while( ! got_one )
		{
			antic_op = ((a52*)sys)->RAM[dlist++];
			switch( antic_op & 0xF )
			{
			case 1:
				dlist = ((a52*)sys)->RAM[dlist]|(((a52*)sys)->RAM[dlist+1]<<8);
				//printf("dlist jump to $%X\n", dlist);
				if( antic_op & 0x40 )
				{
					//printf("new frame from scanline %i\n", scanline);
					got_one = true;
				}
				break;
			default:
				if( (antic_op & 0xF) != 1 )
				{
					if( antic_op & 0x40 )
					{
						LMS = ((a52*)sys)->RAM[dlist++];
						LMS |= ((a52*)sys)->RAM[dlist++]<<8;
					}
					if( antic_op & 0x80 )
					{ // todo: this belongs on the last line of the mode render
						if( aregs[NMIEN]&0x80 )
						{
							((a52*)sys)->cpu.nmi();
							aregs[NMIST] |= 0x80;
						}
					}
				}
				got_one = true; 
				break;
			}
		}
	}
	
	aregs[DLISTH] = dlist>>8;
	aregs[DLISTL] = dlist;
	
	if( scanline < 240 )
	{
	//printf("op = [$%X] $%X\n", dlist-1, antic_op);
		switch( antic_op & 0xF )
		{
		case 0: 
			op_lines++;
			for(int i = 0; i < 320; ++i) ((a52*)sys)->fbuf[scanline*320 + i] = 0;
			if( op_lines == ((antic_op>>4)&7)+1 ) 
			{
				antic_op = OP_NEW;
			}
			break;
		case 4:
			antic_mode_4();
			op_lines++;
			if( op_lines == 8 )
			{
				LMS += 40;
				antic_op = OP_NEW;
			}
			break;
		case 6: 
			antic_mode_6();
			op_lines++;
			if( op_lines == 8 )
			{
				LMS += 20;
				antic_op = OP_NEW;
			}
			break;
		case 7: 
			antic_mode_7();
			op_lines++;
			if( op_lines == 16 )
			{
				LMS += 20;
				antic_op = OP_NEW;
			}
			break;
		case 0xD: 
			op_lines++;
			antic_mode_D();
			if( op_lines == 2 )
			{
				LMS += 40;
				antic_op = OP_NEW;
			}
			break;
		case 0xE: // E is the same as just one scanline of D
			antic_mode_D();
			LMS += 40;
			antic_op = OP_NEW;
			break;
		default: if( antic_op == 0x41 ) break;
			 printf("line[%i]: op was $%X\n", scanline, antic_op);
			 exit(1);
			 break;
		}
	}
	
	scanline++;
	if( scanline == 242 && (aregs[NMIEN]&0x40) )
	{
		aregs[NMIST] |= 0x40;
		((a52*)sys)->cpu.nmi();
	}
	if( scanline == 262 )
	{
		scanline = 0;
		if( antic_op == 0x41 ) antic_op = OP_NEW;
		aregs[DMACTL] = next_dmactl;
	}
	aregs[VCOUNT] = scanline>>1;	
	return;
}

void antic_write(u16 addr, u8 val)
{
	addr &= 0xf;
	//printf("ANTIC[%i] = $%02X\n", addr, val);
	if( addr == 0xB || addr == 0xC || addr == 0xD ) return;	
	
	if( addr == DMACTL )
	{
		next_dmactl = val;
		return;
	}
	
	aregs[addr] = val;
	
	switch( addr )
	{
	case WSYNC: antic_wsync = true; break;
	case NMIST: aregs[NMIST] = 0; break;
	}
}

u8 antic_read(u16 addr)
{
	addr &= 0xf;
	return aregs[addr];
}

void ctia_write(u16 addr, u8 val)
{
	addr &= 0x1f;
	cregs[addr] = val;
	//printf("%X: CTIA $%X = $%X\n", ((a52*)sys)->cpu.PC, addr, val);
}

u8 ctia_read(u16 addr)
{
	//printf("ctia <$%X\n", addr);
	addr &= 0x1F;
	if( addr == 0x1F )
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		if( keys[SDL_SCANCODE_S] ) return 5;
		else return 7;
	}
	return cregs[addr&0x1F];
}

u8 palette5200[] ={
0,0,0,0,0,0,29,29,29,29,29,29,59,59,59,59,59,59,89,89,89,89,89,89,119,119,119,
  119,119,119,149,149,149,149,149,149,179,179,179,179,179,179,209,209,209,209,
  209,209,48,20,0,48,20,0,77,49,0,77,49,0,107,79,0,107,79,0,137,109,21,137,109,
  21,167,139,51,167,139,51,197,169,81,197,169,81,227,199,111,227,199,111,225,229,
  141,225,229,141,65,0,0,65,0,0,94,18,0,94,18,0,124,48,5,124,48,5,154,78,35,154,
  78,35,184,108,65,184,108,65,214,138,95,214,138,95,244,168,125,244,168,125,255,
  198,155,255,198,155,69,0,0,69,0,0,98,0,0,98,0,0,128,19,30,128,19,30,158,49,60,
  158,49,60,188,79,90,188,79,90,218,109,120,218,109,120,248,139,150,248,139,150,
  255,169,180,255,169,180,60,0,1,60,0,1,89,0,30,89,0,30,119,0,60,119,0,60,149,28,
  90,149,28,90,179,58,120,179,58,120,209,88,150,209,88,150,239,118,180,239,118,
  180,255,148,210,255,148,210,38,0,31,38,0,31,67,0,60,67,0,60,97,0,90,97,0,90,127,
  20,120,127,20,120,157,50,150,157,50,150,187,80,180,187,80,180,217,110,210,217,
  110,210,247,140,240,247,140,240,9,0,55,9,0,55,38,0,84,38,0,84,68,0,114,68,0,
  114,98,25,144,98,25,144,128,55,174,128,55,174,158,85,204,158,85,204,188,115,
  234,188,115,234,218,145,255,218,145,255,0,0,68,0,0,68,8,0,97,8,0,97,38,12,127,
  38,12,127,68,42,157,68,42,157,98,72,187,98,72,187,128,102,217,128,102,217,158,
  132,247,158,132,247,188,162,255,188,162,255,0,0,68,0,0,68,0,9,97,0,9,97,11,39,
  127,11,39,127,41,69,157,41,69,157,71,99,187,71,99,187,101,129,217,101,129,217,
  131,159,247,131,159,247,161,189,255,161,189,255,0,11,54,0,11,54,0,40,83,0,40,
  83,0,70,113,0,70,113,24,100,143,24,100,143,54,130,173,54,130,173,84,160,203,84,
  160,203,114,190,233,114,190,233,144,220,255,144,220,255,0,40,29,0,40,29,0,69,
  58,0,69,58,0,99,88,0,99,88,20,129,118,20,129,118,50,159,148,50,159,148,80,189,
  178,80,189,178,110,219,208,110,219,208,140,249,238,140,249,238,0,61,0,0,61,0,
  0,90,28,0,90,28,0,120,58,0,120,58,29,150,88,29,150,88,59,180,118,59,180,118,89,
  210,148,89,210,148,119,240,178,119,240,178,149,255,208,149,255,208,0,69,0,0,69,
  0,0,98,0,0,98,0,21,128,28,21,128,28,51,158,58,51,158,58,81,188,88,81,188,88,111,
  218,118,111,218,118,141,248,148,141,248,148,171,255,178,171,255,178,0,64,0,0,
  64,0,20,93,0,20,93,0,50,123,4,50,123,4,80,153,34,80,153,34,110,183,64,110,183,
  64,140,213,94,140,213,94,170,243,124,170,243,124,200,255,154,200,255,154,21,47,
  0,21,47,0,50,76,0,50,76,0,80,106,0,80,106,0,110,136,21,110,136,21,140,166,51,140,
  166,51,170,196,81,170,196,81,200,226,111,200,226,111,230,255,141,230,255,141,30,
  14,0,30,14,0,77,49,0,77,49,0,107,79,0,107,79,0,137,109,21,137,109,21,167,139,
  51,167,139,51,197,169,81,197,169,81,227,199,111,227,199,111,255,229,141,255,
  229,141
};



