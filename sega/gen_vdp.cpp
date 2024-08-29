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
	
	const u32 hs_base = (vreg[0xD]&0x3F)<<10;
	u32 hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base]);
	u32 hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base+2]);
	
	for(u32 px = 0; px < vdp_width; ++px)
	{
		if( (vreg[0xB] & 3) == 3 )
		{
			hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base + line*4]);
			hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base + line*4 + 2]);		
		} else if( (vreg[0xB] & 3) == 2 ) {
			hsA = __builtin_bswap16(*(u16*)&VRAM[hs_base + (line&~7)*4]);
			hsB = __builtin_bswap16(*(u16*)&VRAM[hs_base + (line&~7)*4 + 2]);		
		}
		
		hsA &= 0x3ff;
		hsB &= 0x3ff;
	
		u16 entryA = __builtin_bswap16(*(u16*)&VRAM[ntA + (((line/8)*nt_width) + (((px-hsA)&((nt_width*8)-1))/8))*2]);
		u16 entryB = __builtin_bswap16(*(u16*)&VRAM[ntB + (((line/8)*nt_width) + (((px-hsB)&((nt_width*8)-1))/8))*2]);
		
		u32 lYA = (line&7) ^ ((entryA&BIT(12))?7:0);
		u32 lXA = ((px-hsA)&7) ^ ((entryA&BIT(11))?7:0);
		u8 tA = VRAM[(entryA&0x7ff)*32 + lYA*4 + ((lXA>>1))] >> ((lXA&1)?0:4);
		tA &= 0xf;
		
		u32 lYB = (line&7) ^ ((entryB&BIT(12))?7:0);
		u32 lXB = ((px-hsB)&7) ^ ((entryB&BIT(11))?7:0);
		u8 tB = VRAM[((entryB&0x7ff)*32 + lYB*4 + (lXB>>1))] >> ((lXB&1)?0:4);
		tB &= 0xf;
				
		if( tA )
		{
			tA |= (entryA>>9)&0x30;
		} else if( tB ) {
			tA = tB | ((entryB>>9)&0x30);
		} else {
			tA |= vreg[7]&0x3F;
		}
		
		u32 c = __builtin_bswap16(*(u16*)&CRAM[tA<<1]);
		fbuf[line*vdp_width + px] = ((c&0xe)<<28)|((c&0xe0)<<16)|((c&0xe00)<<4);	
	}
	return;
}

void genesis::vdp_ctrl(u16 val)
{
	if( !vdp_latch && (val&0xC000) == 0x8000 )
	{
		//printf("VDP: regwrite = $%X\n", val);
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
		if( vdp_cd & BIT(5) )
		{
			if( (vreg[0x17] & 0xc0) == 0x80 )
			{
				fill_pending = true;
			} else if( (vreg[0x17] & 0xc0) == 0xc0 ) {
				vdp_vram2vram();
			} else {
				u32 len = (vreg[0x14]<<8)|vreg[0x13];
				if( len == 0 ) len = 0x10000;
				//len >>= 1;
				u32 src = (vreg[0x17]<<16)|(vreg[0x16]<<8)|vreg[0x15];
				src <<= 1;
				for(u32 i = 0; i < len; ++i)
				{
					vdp_data(read(src,16));
					src += 2;
				}
			}
		}
		
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
		u32 len = (vreg[0x14]<<8)|vreg[0x13];
		if( len == 0 ) len = 0x10000;
		//len >>= 1;
		for(u32 i = 0; i < len; ++i) VRAM[(vdp_addr + i)&0xffff] = val;
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
		//exit(1);
	}
	
}

void genesis::vdp_vram2vram()
{



}

