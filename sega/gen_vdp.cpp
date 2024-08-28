#include <cstdio>
#include <cstdlib>
#include "genesis.h"

static u32 ntsize[] = { 32, 64, 128, 128 };


void genesis::draw_line(u32 line)
{
	u32 ntA = ((vreg[2]>>3)&7) << 13;
	u32 ntB = (vreg[4]&7) << 13;
	
	const u32 nt_height= ntsize[(vreg[16]>>4)&3];
	const u32 nt_width = ntsize[vreg[16]&3];

	for(u32 px = 0; px < vdp_width; ++px)
	{
		u16 entryA = __builtin_bswap16(*(u16*)&VRAM[ntA + (((line/8)*nt_width) + (px/8))*2]);
		u16 entryB = __builtin_bswap16(*(u16*)&VRAM[ntB + (((line/8)*nt_width) + (px/8))*2]);
		
		u32 lYA = (line&7) ^ ((entryA&BIT(12))?7:0);
		u32 lXA = (px&7) ^ ((entryA&BIT(11))?7:0);
		u8 tA = VRAM[(entryA&0x7ff)*32 + lYA*4 + ((lXA>>1))] >> ((lXA&1)?0:4);
		tA &= 0xf;
		
		/*u32 lYB = (line&7) ^ ((entryB&BIT(12))?7:0);
		u32 lXB = (px&7) ^ ((entryB&BIT(11))?7:0);
		u8 tB = VRAM[((entryB&0x7ff)*32 + lYB*4 + (lXB>>1))] >> ((lXB&1)?4:0);
		tB &= 0xf;
		*/
		
		if( tA )
		{
			tA |= (entryA>>9)&0x30;
		} else {
			tA |= vreg[7]&0x3F;
		} 
		/*else if( tB ) {
			tB |= (entryB>>9)&0x30;
			tA = tB;
		}*/
		
		u32 c = __builtin_bswap16(*(u16*)&CRAM[tA<<1]);
		fbuf[line*vdp_width + px] = ((c&0xe)<<28)|((c&0xe0)<<16)|((c&0xe00)<<4);	
	}
	return;
}

void genesis::vdp_ctrl(u16 val)
{
	if( !vdp_latch && (val&0xC000) == 0x8000 )
	{
		printf("VDP: regwrite = $%X\n", val);
		u32 r = (val>>8)&0x1f;
		vreg[r] = val;
		return;	
	}
	
	if( vdp_latch )
	{	// second write
		vdp_addr &= ~0xC000;
		vdp_addr |= (val<<14);
		vdp_cd &= ~0x3c;
		vdp_cd |= (val&0xf0)>>2;
		//printf("%X: VDP: cd = $%X, addr = $%X\n", cpu.pc-2, vdp_cd, vdp_addr);
		
		
	} else {
		//first write
		vdp_addr &= ~0x3fff;
		vdp_addr |= (val&0x3fff);
		vdp_cd &= ~3;
		vdp_cd |= (val>>14);
	}

	vdp_latch = !vdp_latch;
	//printf("VDP: non-reg write = $%X\n", val);
}

void genesis::vdp_data(u16 val)
{
	if( fill_pending )
	{
		fill_pending = false;
		
		return;
	}

	u16 index = vdp_addr;
	vdp_addr += vreg[0xf];
	
	switch( vdp_cd & 0xf )
	{
	case 1:
		index &= 0xfffe;
		VRAM[index] = val>>8;
		VRAM[index+1] = val;	
		break;
	case 3:
		index = (index>>1)&0x3F;
		CRAM[index*2] = val>>8;
		CRAM[index*2 + 1] = val;
		break;
	case 5:
		//todo: VSRAM
		break;
	default:
		printf("GEN: VDP write $%X with cd = $%X\n", val, vdp_cd);
		exit(1);
	}
	
}

