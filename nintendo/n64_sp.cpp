#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "n64.h"

void n64::sp_read_dma()
{	// to RSP
	u32 skip = (sp_regs[2]>>20)&0xff8;
	u32 count = ((sp_regs[2]>>12)&0xff)+1;
	u32 rdlen = (sp_regs[2]&0xfff);
	rdlen = (rdlen + 7) & ~7;
	
	u8* rmem = ((sp_regs[0]&BIT(12)) ? IMEM : DMEM);
	u32 ram_offset = sp_regs[1] & 0x3ffFFf8;
	u32 sp_offset = sp_regs[0] & 0xff8;
	
	for(u32 c = 0; c < count; ++c)
	{
		for(u32 i = 0; i < rdlen; ++i, ++sp_offset, ++ram_offset) 
		{
			rmem[sp_offset&0xfff] = (ram_offset < 0x800000 ) ? mem[ram_offset] : 0;
			sp_regs[0] = (sp_regs[0]&~0xfff) | ((sp_regs[0]+1)&0xfff); 
		}
		ram_offset += skip;
	}
	sp_regs[1] = ram_offset;	
	sp_regs[0] &= ~7;
	sp_regs[2] |= 0xff8;
	sp_regs[2] &= ~0xff007;
}

void n64::sp_write_dma()
{	// to RDRAM
	u32 skip = (sp_regs[3]>>20)&0xff8;
	u32 count = ((sp_regs[3]>>12)&0xff)+1;
	u32 wrlen = (sp_regs[3]&0xfff);
	wrlen = (wrlen + 7) & ~7;
	
	u8* rmem = ((sp_regs[0]&BIT(12)) ? IMEM : DMEM);
	u32 ram_offset = sp_regs[1] & 0x3ffFFf8;
	u32 sp_offset = sp_regs[0] & 0xff8;
	
	for(u32 c = 0; c < count; ++c)
	{
		for(u32 i = 0; i < wrlen; ++i, ++sp_offset, ++ram_offset) 
		{
			if( ram_offset < 0x800000 ) mem[ram_offset] = rmem[sp_offset&0xfff];
			sp_regs[0] = (sp_regs[0]&~0xfff) | ((sp_regs[0]+1)&0xfff);
		}
		ram_offset += skip;
	}
	sp_regs[1] = ram_offset;
	sp_regs[0] &= ~7;
	sp_regs[3] |= 0xff8;
	sp_regs[3] &= ~0xff007;
}

void n64::sp_write(u32 addr, u32 v)
{
	if( addr == 0x4080000 )
	{
		RSP.pc = v & 0xffc;
		RSP.npc = RSP.pc + 4;
		RSP.nnpc = RSP.npc + 4;
		return;
	}
	//printf("SP Write $%X = $%X\n", addr, v);
	const u32 r = (addr&0x1C)>>2;
	
	if( r < 4 )
	{
		sp_regs[r] = v;
		if( r == 2 )
		{
			sp_read_dma();
		} else if( r == 3 ) {
			sp_write_dma();
		}
		return;
	}
	
	if( r == 4 )
	{  // SP_STATUS
		u32 hlt = (v&3);
		if( hlt == 1 ) { SP_STATUS &= ~1; } else if( hlt == 2 ) { SP_STATUS |= 1; }
		if( v & BIT(2) )
		{
			SP_STATUS &= ~BIT(1);
		}
		u32 intr = (v>>3)&3;
		if( intr == 1 )
		{
			clear_mi_bit(MI_INTR_SP_BIT);
		} else if( intr == 2 ) {
			raise_mi_bit(MI_INTR_SP_BIT);
		}
		u32 sstep = (v>>5)&3;
		if( sstep == 1 )
		{
			SP_STATUS &= ~BIT(5);
		} else if( sstep == 2 ) {
			SP_STATUS |= BIT(5);
		}
		u32 ib = (v>>7)&3;
		if( ib == 1 )
		{
			SP_STATUS &= ~BIT(6);
		} else if( ib == 2 ) {
			SP_STATUS |= BIT(6);
		}
		u32 sig0 = (v>>9)&3;
		if( sig0 == 1 )
		{
			SP_STATUS &= ~BIT(7);
		} else if( sig0 == 2 ) {
			SP_STATUS |= BIT(7);
		}
		u32 sig1 = (v>>11)&3;
		if( sig1 == 1 )
		{
			SP_STATUS &= ~BIT(8);
		} else if( sig1 == 2 ) {
			SP_STATUS |= BIT(8);
		}
		u32 sig2 = (v>>13)&3;
		if( sig2 == 1 )
		{
			SP_STATUS &= ~BIT(9);
		} else if( sig2 == 2 ) {
			SP_STATUS |= BIT(9);
		}
		u32 sig3 = (v>>15)&3;
		if( sig3 == 1 )
		{
			SP_STATUS &= ~BIT(10);
		} else if( sig3 == 2 ) {
			SP_STATUS |= BIT(10);
		}
		u32 sig4 = (v>>17)&3;
		if( sig4 == 1 )
		{
			SP_STATUS &= ~BIT(11);
		} else if( sig4 == 2 ) {
			SP_STATUS |= BIT(11);
		}
		u32 sig5 = (v>>19)&3;
		if( sig5 == 1 )
		{
			SP_STATUS &= ~BIT(12);
		} else if( sig5 == 2 ) {
			SP_STATUS |= BIT(12);
		}
		u32 sig6 = (v>>21)&3;
		if( sig6 == 1 )
		{
			SP_STATUS &= ~BIT(13);
		} else if( sig6 == 2 ) {
			SP_STATUS |= BIT(13);
		}
		u32 sig7 = (v>>23)&3;
		if( sig7 == 1 )
		{
			SP_STATUS &= ~BIT(14);
		} else if( sig7 == 2 ) {
			SP_STATUS |= BIT(14);
		}
		return;
	}
	
	if( r == 7 )
	{
		// the wiki says writes work normally, but systemtest wants writes to result in zero.
		SP_SEMA = 0;
		return;
	}
}

u32 n64::sp_read(u32 addr)
{
	if( addr == 0x4080000 ) return RSP.pc;
	//printf("SP Read <$%X\n", addr);
	const u32 r = (addr&0x1C)>>2;
	u32 result = sp_regs[r];
	if( r == 7 ) SP_SEMA = 1;
	return result;
}

