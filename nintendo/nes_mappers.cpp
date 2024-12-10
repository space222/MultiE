#include <iostream>
#include <string>
#include "nes.h"

void mapper_cnrom_write(nes* NES, u16 addr, u8 val)
{
	if( addr >= 0x8000 )
	{
		val &= 3;
		for(int i = 0; i < 8; ++i) NES->chrbanks[i] = NES->CHR.data() + (val*0x2000) + (i*1024);	
	}
}

void mapper_unrom_write(nes* NES, u16 addr, u8 val)
{
	if( addr >= 0x8000 )
	{
		val &= 7;
		NES->prgbanks[0] = NES->PRG.data() + (val*0x4000);
		NES->prgbanks[1] = NES->PRG.data() + (val*0x4000) + 0x2000;	
	}
}


static u8 mmc1_v, mmc1_index, mmc1_regs[4];

void mapper_mmc1_write(nes* NES, u16 addr, u8 val)
{
	if( addr < 0x8000 ) return;
	//printf("mapper 1 write $%X = $%X\n", addr, val);
	if( val & 0x80 )
	{
		mmc1_index = 0;
		mmc1_regs[0] |= 0xc;
	} else {
		mmc1_v |= (val&1)<<mmc1_index;
		mmc1_index++;
		if( mmc1_index != 5 ) return;
		
		u16 reg = (addr>>13)&3;
		val = mmc1_v;
		mmc1_v = mmc1_index = 0;
		mmc1_regs[reg] = val;
		//printf("mmc1 reg %i = $%X\n", reg, val);
	}
	
	// prg mapping
	switch( (mmc1_regs[0]>>2) & 3 )
	{
	case 0:
	case 1:
		NES->prgbanks[0] = NES->PRG.data() + (mmc1_regs[3]&0xe)*32*1024;
		NES->prgbanks[1] = NES->prgbanks[0] + 8*1024;
		NES->prgbanks[2] = NES->prgbanks[1] + 8*1024;
		NES->prgbanks[3] = NES->prgbanks[2] + 8*1024;	
		break;
	case 2:
		NES->prgbanks[0] = NES->PRG.data();
		NES->prgbanks[1] = NES->prgbanks[0] + 8*1024;
		NES->prgbanks[2] = NES->PRG.data() + (mmc1_regs[3]&0xf)*16*1024;
		NES->prgbanks[3] = NES->prgbanks[2] + 8*1024;
		break;
	case 3:
		NES->prgbanks[2] = NES->PRG.data() + (NES->PRG.size() - 16*1024);
		NES->prgbanks[3] = NES->prgbanks[2] + 8*1024;
		NES->prgbanks[0] = NES->PRG.data() + (mmc1_regs[3]&0xf)*16*1024;
		NES->prgbanks[1] = NES->prgbanks[0] + 8*1024;
		break;
	}
	
	// chr mapping
	if( mmc1_regs[0] & 0x10 )
	{
		NES->chrbanks[0] = NES->CHR.data() + (mmc1_regs[1]*0x1000);
		NES->chrbanks[1] = NES->chrbanks[0] + 0x400;
		NES->chrbanks[2] = NES->chrbanks[1] + 0x400;
		NES->chrbanks[3] = NES->chrbanks[2] + 0x400;
		NES->chrbanks[4] = NES->CHR.data() + (mmc1_regs[2]*0x1000);
		NES->chrbanks[5] = NES->chrbanks[4] + 0x400;
		NES->chrbanks[6] = NES->chrbanks[5] + 0x400;
		NES->chrbanks[7] = NES->chrbanks[6] + 0x400;
	} else {
		NES->chrbanks[0] = NES->CHR.data() + ((mmc1_regs[1]&0x1E)*0x2000);
		NES->chrbanks[1] = NES->chrbanks[0] + 0x400;
		NES->chrbanks[2] = NES->chrbanks[1] + 0x400;
		NES->chrbanks[3] = NES->chrbanks[2] + 0x400;
		NES->chrbanks[4] = NES->chrbanks[3] + 0x400;
		NES->chrbanks[5] = NES->chrbanks[4] + 0x400;
		NES->chrbanks[6] = NES->chrbanks[5] + 0x400;
		NES->chrbanks[7] = NES->chrbanks[6] + 0x400;
	}
	
	//nametables
	switch( mmc1_regs[0] & 3 )
	{
	case 0: NES->nametable[0] = &NES->ntram[0];
		NES->nametable[1] = &NES->ntram[0];
		NES->nametable[2] = &NES->ntram[0];
		NES->nametable[3] = &NES->ntram[0];
		break;
	case 1:	NES->nametable[0] = &NES->ntram[0x400];
		NES->nametable[1] = &NES->ntram[0x400];
		NES->nametable[2] = &NES->ntram[0x400];
		NES->nametable[3] = &NES->ntram[0x400];
		break;
	case 2: NES->nametable[0] = &NES->ntram[0];
		NES->nametable[2] = NES->nametable[0];
		NES->nametable[1] = NES->nametable[3] = &NES->ntram[1024];
		break;
	case 3: NES->nametable[0] = &NES->ntram[0];
		NES->nametable[1] = NES->nametable[0];
		NES->nametable[2] = NES->nametable[3] = &NES->ntram[1024];
		break;
	}
}


static bool mmc3_irq_enabled = false, mmc3_irq_reload = false;
static u8 mmc3_irq_cnt, mmc3_irq_latch;
static u16 mmc3_last_addr = 0;
static u8 mmc3_R[8], mmc3_bank_select;

void mapper_mmc3_write(nes* NES, u16 addr, u8 val)
{
	bool need_rebank = false;
	if( addr < 0xA000 )
	{
		if( addr & 1 )
		{
			mmc3_R[mmc3_bank_select&7] = val;
		} else {
			mmc3_bank_select = val;
		}
		need_rebank = true;
	} else if( addr < 0xC000 ) {
		if( addr & 1 )
		{
		
		} else {
			if( !(val & 1) )
			{
				NES->nametable[0] = &NES->ntram[0];
				NES->nametable[2] = NES->nametable[0];
				NES->nametable[1] = NES->nametable[3] = &NES->ntram[1024];
			} else {
				NES->nametable[0] = &NES->ntram[0];
				NES->nametable[1] = NES->nametable[0];
				NES->nametable[2] = NES->nametable[3] = &NES->ntram[1024];
			}
		}	
	} else if( addr < 0xE000 ) {
		if( addr & 1 )
		{
			mmc3_irq_cnt = 0xff;
			mmc3_irq_reload = true;
		} else {
			mmc3_irq_latch = val;
		}	
	} else {
		if( addr & 1 )
		{
			mmc3_irq_enabled = true;
		} else {
			mmc3_irq_enabled = NES->cpu.irq_line = false;
		}	
	}
	if( !need_rebank ) return;
	
	NES->prgbanks[3] = NES->PRG.data() + (NES->PRG.size() - 8*1024);
	NES->prgbanks[1] = NES->PRG.data() + ((mmc3_R[7]&0x3F)*8*1024);
	
	if( mmc3_bank_select & 0x40 )
	{
		NES->prgbanks[0] = NES->PRG.data() + (NES->PRG.size() - 16*1024);
		NES->prgbanks[2] = NES->PRG.data() + ((mmc3_R[6]&0x3F)*8*1024);	
	} else {
		NES->prgbanks[2] = NES->PRG.data() + (NES->PRG.size() - 16*1024);
		NES->prgbanks[0] = NES->PRG.data() + ((mmc3_R[6]&0x3F)*8*1024);
	}
	
	if( mmc3_bank_select & 0x80 )
	{
		NES->chrbanks[0] = NES->CHR.data() + (mmc3_R[2]*1024);
		NES->chrbanks[1] = NES->CHR.data() + (mmc3_R[3]*1024);
		NES->chrbanks[2] = NES->CHR.data() + (mmc3_R[4]*1024);
		NES->chrbanks[3] = NES->CHR.data() + (mmc3_R[5]*1024);
		NES->chrbanks[4] = NES->CHR.data() + ((mmc3_R[0]&0xFE)*1024);
		NES->chrbanks[5] = NES->chrbanks[4] + 1024;
		NES->chrbanks[6] = NES->CHR.data() + ((mmc3_R[1]&0xFE)*1024);
		NES->chrbanks[7] = NES->chrbanks[6] + 1024;
	} else {
		NES->chrbanks[4] = NES->CHR.data() + (mmc3_R[2]*1024);
		NES->chrbanks[5] = NES->CHR.data() + (mmc3_R[3]*1024);
		NES->chrbanks[6] = NES->CHR.data() + (mmc3_R[4]*1024);
		NES->chrbanks[7] = NES->CHR.data() + (mmc3_R[5]*1024);
		NES->chrbanks[0] = NES->CHR.data() + ((mmc3_R[0]&0xFE)*1024);
		NES->chrbanks[1] = NES->chrbanks[0] + 1024;
		NES->chrbanks[2] = NES->CHR.data() + ((mmc3_R[1]&0xFE)*1024);
		NES->chrbanks[3] = NES->chrbanks[2] + 1024;	
	}
}

void mapper_mmc3_ppu_bus(nes* NES, u16 addr)
{
	u16 temp = mmc3_last_addr;
	mmc3_last_addr = addr;
	if( ! (!(temp & 0x1000) && (addr & 0x1000)) )  return;
	//printf("got here scanline = $%X, temp = $%X, addr = $%X\n", NES->scanline, temp&0x1000, addr&0x1000);
	
	if( mmc3_irq_reload || mmc3_irq_cnt == 0 )
	{
		if( mmc3_irq_enabled && mmc3_irq_cnt == 0 ) NES->cpu.irq_line = true;
		//if( NES->scanline >= 2 )
		//	for(int i = 0; i < 256; ++i) NES->fbuf[(NES->scanline-2)*256 + i] = 0x00ff000;
		mmc3_irq_cnt = mmc3_irq_latch;
		mmc3_irq_reload = false;
		//return;
	}
	
	
	mmc3_irq_cnt--;
}

void mapper_zero_write(nes*, u16, u8) {}
void mapper_zero_ppu_bus(nes*, u16) {}

void mapper_setup(nes* NES)
{
	NES->mapper_ppu_bus = mapper_zero_ppu_bus;
	
	switch(NES->mapper)
	{
	case 0: NES->mapper_write = mapper_zero_write; break;
	case 1: NES->mapper_write = mapper_mmc1_write; break;
	case 2: NES->mapper_write = mapper_unrom_write; break;
	case 3: NES->mapper_write = mapper_cnrom_write; break;
	case 4: NES->mapper_write = mapper_mmc3_write; NES->mapper_ppu_bus = mapper_mmc3_ppu_bus; break;
	default: printf("Unimpl mapper #%i\n", NES->mapper); exit(1);
	}
}


