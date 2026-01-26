#include <print>
#include <string>
#include "util.h"
#include "ps2.h"


u128 ps2::read(u32 addr, int sz)
{
	if( logall && addr != cpu.pc ) { std::println("EE Rd{} ${:X}", sz, addr); }
	if( addr >= 0x70000000u && addr < 0x80000000u ) return sized_read128(spad, addr&0x3fff, sz);	
	if( addr >= 0xFFFF8000u ) { addr = 0x78000 | (addr & 0x7fff); }	
	addr &= 0x1FFFffffu;	
	if( addr < 32*1024*1024 ) return sized_read128(RAM, addr, sz);
	if( addr >= 0x1FC00000u && addr < (0x1FC00000u+4_MB) ) return sized_read128(BIOS, addr&0x3fffff, sz);

	switch( addr )
	{
	case 0x1000E000: return eedma.D_CTRL;
	case 0x1000E010: std::println("${:X}: ee D_STAT(${:X})", cpu.pc, (u32)eedma.D_STAT.v);  return eedma.D_STAT.v;
	case 0x1000E040: return eedma.D_RBSR;
	case 0x1000E050: return eedma.D_RBOR;
	case 0x1000F130: return 0;
	
	case 0x1000F000: /*std::println("${:X}: ee INTC_STAT(${:X})", cpu.pc, eeint.INTC_STAT);*/ return eeint.INTC_STAT;
	case 0x1000F010: return eeint.INTC_MASK;
	
	case 0x1000F200: return sifmb.SIF_MSCOM;
	case 0x1000F210: std::println("EE Rd SIF_SMCOM ${:X}", sifmb.SIF_SMCOM); return sifmb.SIF_SMCOM;
	case 0x1000F220: std::println("EE Rd SIF_MSFLG ${:X}", sifmb.SIF_MSFLG); return sifmb.SIF_MSFLG;
	case 0x1000F230: /*std::println("EE Rd SIF_SMFLG ${:X}", sifmb.SIF_SMFLG);*/ return sifmb.SIF_SMFLG;
	
	case 0x1000f430: return 0;
	case 0x1000f440:
	{
		uint8_t SOP = (meminit.MCH_RICM >> 6) & 0xF;
		uint16_t SA = (meminit.MCH_RICM >> 16) & 0xFFF;
		if (!SOP)
		{
			switch (SA)
			{
			case 0x21:
			    if (meminit.rdram_sdevid < 2)
			    {
				meminit.rdram_sdevid++;
				return 0x1F;
			    }
			    return 0;
			case 0x23:
			    return 0x0D0D;
			case 0x24:
			    return 0x0090;
			case 0x40:
			    return meminit.MCH_RICM & 0x1F;
			}
		}
		return 0;
	}
	}
	
	if( (addr & 0xffFFff00u) == 0x10008000 )
	{ // VIF0 dma
		return eedma.chan[0][(addr>>4)&15];
	}
	
	if( (addr & 0xffFFff00u) == 0x10009000 )
	{ // VIF1 dma
		return eedma.chan[1][(addr>>4)&15];
	}	

	if( (addr & 0xffFFff00u) == 0x1000A000 )
	{ // GIF dma
		return eedma.chan[2][(addr>>4)&15];
	}

	if( (addr & 0xffFFff00u) == 0x1000B000 )
	{ // IPU_FROM dma
		return eedma.chan[3][(addr>>4)&15];
	}
	
	if( (addr & 0xffFFff00u) == 0x1000B400 )
	{ // IPU_TO dma
		return eedma.chan[4][(addr>>4)&15];
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C000 )
	{ // SIF0 dma
		return eedma.chan[5][(addr>>4)&15];
	}

	if( (addr & 0xffFFff00u) == 0x1000C400 )
	{ // SIF1 dma
		return eedma.chan[6][(addr>>4)&15];
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C800 )
	{ // SIF2 dma
		return eedma.chan[7][(addr>>4)&15];
	}

	if( (addr & 0xffFFff00u) == 0x1000D000 )
	{ // SPR_FROM dma
		return eedma.chan[8][(addr>>4)&15];
	}
	
	if( (addr & 0xffFFff00u) == 0x1000D800 )
	{ // SIF2 dma
		return eedma.chan[9][(addr>>4)&15];
	}
	
	std::println("EE Rd{} ${:X}", sz, addr);
	return 0;
}

void ps2::write(u32 addr, u128 v, int sz)
{
	if( addr >= 0x70000000u && addr < 0x80000000u ) return sized_write128(spad, addr&0x3fff, v, sz);
	if( addr >= 0xFFFF8000u ) { addr = 0x78000 | (addr & 0x7fff); }	
	addr &= 0x1FFFffffu;
	if( addr < 32*1024*1024 ) return sized_write128(RAM, addr, v, sz);
	
	if( addr == 0x1000F180 )
	{ // EE kernel writes logs to this addr
		fputc((u8)v, stderr);
		return;
	}
	
	switch( addr )
	{
	case 0x1000F000: 
		eeint.INTC_STAT &= ~v; 
		if( eeint.INTC_MASK & eeint.INTC_STAT ) 
		{
			cpu.cop0[13] |= BIT(10); 
		} else { 
			cpu.cop0[13] &=~BIT(10);
		}
		return;
	case 0x1000F010: 
		eeint.INTC_MASK ^= v;
		if( eeint.INTC_MASK & eeint.INTC_STAT ) 
		{
			cpu.cop0[13] |= BIT(10); 
		} else { 
			cpu.cop0[13] &=~BIT(10);
		}
		return;

	case 0x1000E000: eedma.D_CTRL = v; return;
	case 0x1000E010:
		eedma.D_STAT.b.stat &= ~v;
		eedma.D_STAT.b.dma_stall &= ((v&BIT(13)) ? 0:1);
		eedma.D_STAT.b.mfifo_empty &= ((v&BIT(14)) ? 0:1);
		eedma.D_STAT.b.buserr &= ((v&BIT(15)) ? 0:1);
		eedma.D_STAT.b.mask ^= (v>>16);
		eedma.D_STAT.b.dma_stall_mask ^= ((v&BIT(29))?1:0);
		eedma.D_STAT.b.mfifo_empty_mask ^= ((v&BIT(30))?1:0);
		
		if( (eedma.D_STAT.b.stat & eedma.D_STAT.b.mask) 
		 || (eedma.D_STAT.b.dma_stall & eedma.D_STAT.b.dma_stall_mask)
		 || (eedma.D_STAT.b.mfifo_empty & eedma.D_STAT.b.mfifo_empty_mask)
		  ) {
		  	cpu.cop0[13] |= BIT(11); // Cause.INT1
		  } else {
		  	cpu.cop0[13] &=~BIT(11);
		  }
		return;
	case 0x1000E040: eedma.D_RBSR = v&0x7fffFFFFu; return;
	case 0x1000E050: eedma.D_RBOR = v&0x7fffFFFFu; return;
	case 0x1000F200: std::println("EE MSCOM = ${:X}", v); sifmb.SIF_MSCOM = v; return;
	//case 0x1000F210: SIF_SMCOM not writable by EE
	case 0x1000F220: std::println("EE MSFLG |= ${:X}", v); sifmb.SIF_MSFLG |= v; return;
	case 0x1000F230: std::println("EE SMFLG &=~${:X}", v); sifmb.SIF_SMFLG &= ~v; return;
	case 0x1000f430:
	{
		uint16_t SA = (v >> 16) & 0xFFF;
		uint8_t SBC = (v >> 6) & 0xF;

		if (SA == 0x21 && SBC == 0x1 && ((meminit.MCH_DRD >> 7) & 1) == 0)
			meminit.rdram_sdevid = 0;

		meminit.MCH_RICM = v & ~0x80000000ul;
		return;
	}
	case 0x1000f440: meminit.MCH_DRD = v; return;
	default: break;
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C000 )
	{
		u32 reg = (addr>>4)&15;
		eedma.chan[5][reg] = v;
		if( reg == 0 && (v&BIT(8)) && !eedma.sif_fifo.empty() )
		{ // possibly need to be blocked by D_CTRL.0 and/or D_ENABLER.16
			ee_sif_dest_chain();
		}
		return;
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C400 )
	{ // SIF1 dma
		u32 reg = (addr>>4)&15;
		eedma.chan[6][reg] = v;
		std::println("EE SIF1 dma reg${:X} = ${:X}", reg, v);
		if( reg == 0 && (v&BIT(8)) )
		{ // possibly need to be blocked by D_CTRL.0 and/or D_ENABLER.16
			std::println("${:X} = ${:X}, EE SIF1->IOP start", addr, v);
			u32 mode = (v>>2)&3;
			if( mode == 1 )
			{
				ee_dma_chain(6, [&](u32 d) { iop_dma.sif_fifo.push_front(d); });
				if( iop_dma.chan[10][2] & BIT(24) )
				{
					std::println("IOP's DMA is active, sending fifo");
					iop_sif_dest_chain();
				}
				eedma.chan[6][0] &= ~BIT(8);
			}
		}
		return;
	}
	
	std::println("EE Wr{} ${:X} = ${:X}", sz, addr, v);

}

void ps2::ee_dma_chain(u32 c, std::function<void(u32)> where)
{
	std::println("Chain start at ${:X}", eedma.chan[c][3]);
	std::println("QWC = ${:X}", eedma.chan[c][2]);
	
	bool ended = false;
	u32& TADR = eedma.chan[c][3];
	u32& MADR = eedma.chan[c][1];
	u32& QWC  = eedma.chan[c][2];
	
	while( !ended )
	{
		u32 ptr = TADR&(32_MB-1);
		u128 tag = *(u128*)&RAM[ptr];
		u32 id = (tag>>28)&3;
		
		switch( id )
		{
		case 3:
		case 0:
			MADR = tag>>32;
			if( MADR & BIT(31) ) { std::println("FROM SPAD!"); exit(1); }
			TADR += 16;
			QWC = tag&0xffff;
			std::println("xfer start, M=${:X}, Q=${:X}", MADR, QWC);
			std::println("T now ${:X}", TADR);
			for(; QWC>0; --QWC, MADR+=16) 
			{
				std::println("DMA sending ${:X}", *(u32*)&RAM[MADR]);
				std::println("DMA sending ${:X}", *(u32*)&RAM[MADR+4]);
				std::println("DMA sending ${:X}", *(u32*)&RAM[MADR+8]);
				std::println("DMA sending ${:X}", *(u32*)&RAM[MADR+12]);
				where(*(u32*)&RAM[MADR]);
				where(*(u32*)&RAM[MADR+4]);
				where(*(u32*)&RAM[MADR+8]);
				where(*(u32*)&RAM[MADR+12]);
			}
			if( id==0 ) { ended = true; }
			break;
		default:
			std::println("EE: Unimpl chain id = {}", id);
			exit(1);
		}
	}
}

void ps2::ee_sif_dest_chain()
{
	bool end = false;
	bool do_irq = false;
	//while( !end )
	{
		u64 tag = eedma.popd();
		std::println("EE TAG ${:X}", tag);
		u32 addr = tag>>32;
		u32 size = tag&0xffff;
		eedma.chan[5][1] = addr+size*16;

		do_irq = tag & BIT(31);
		end = end || (do_irq && (eedma.chan[5][0]&BIT(7)));
		for(u32 i = 0; i < size; ++i)
		{
			*(u128*)&RAM[addr + i*16] = eedma.popq();
		}
	}
	eedma.chan[5][0] &= ~BIT(8);
	eedma.chan[5][2] = 0;
	
	if( do_irq && (eedma.chan[5][0]&BIT(7)) )
	{
		
	}
	
	eedma.D_STAT.b.stat |= BIT(5);
	if( (eedma.D_STAT.b.stat & eedma.D_STAT.b.mask) 
	 || (eedma.D_STAT.b.dma_stall & eedma.D_STAT.b.dma_stall_mask)
	 || (eedma.D_STAT.b.mfifo_empty & eedma.D_STAT.b.mfifo_empty_mask)
	  ) {
	  	cpu.cop0[13] |= BIT(11); // Cause.INT1
	  } else {
	  	cpu.cop0[13] &=~BIT(11);
	  }
}

bool logall = false;

void ps2::run_frame()
{
	u64 target = last_target + 18740*262;
	
	while( global_stamp < target )
	{
		while( cpu.stamp < global_stamp )
		{
			if( (cpu.pc&0x7FFFFF) == 0xb1fc0 )
			{
				std::println("EE reached B1FC0(EENULL)");
				exit(1);
			}
			//if( logall ) std::println("ee pc ${:X}", cpu.pc);
			cpu.step();
		}
		
		while( iop_stamp < global_stamp )
		{
			if (iop.pc == 0x12C48 || iop.pc == 0x1420C || iop.pc == 0x1430C)
			{
				uint32_t pointer = iop.r[5];
				uint32_t text_size = iop.r[6];
				while (text_size)
				{
					auto c = (pointer>2_MB) ? (char)BIOS[pointer&0x3fffff] : (char)iop_ram[pointer & 0x1FFFFF];
					fputc(c, stderr);
					pointer++;
					text_size--;
				}
			}
		
			if( (iop_int.I_MASK & iop_int.I_STAT) && iop_int.I_CTRL )
			{
				iop.c[13] |= BIT(10);
			} else {
				iop.c[13] &=~BIT(10);
			}
			iop.check_irqs();
			iop.step();
			iop_stamp += 8;
		}
		
		global_stamp += 1;
	}
	
	//eeint.INTC_STAT = 2;
	
	last_target = target;

}

bool ps2::loadROM(std::string fname)
{
	if( !freadall(BIOS, fopen("./bios/PS2.BIOS", "rb"), 4_MB) )
	{
		std::println("Need PS2.BIOS");
		return false;
	}
	
	iop.read = [&](u32 a, int sz)->u32 { return iop_read(a,sz); };
	iop.write = [&](u32 a, u32 v, int sz) { iop_write(a,v,sz); };
	
	cpu.read = [&](u32 a, int sz)->u128 { return read(a,sz); };
	cpu.write =[&](u32 a, u128 v, int sz) { write(a,v,sz); };
	return true;
}

void ps2::reset()
{
	cpu.pc = 0xbfc00000u;
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	cpu.cop0[15] = 0x59;
	iop.reset();
	iop.c[15] = 0x1F;
	iop.pc = cpu.pc;
	iop.npc = cpu.npc;
	iop.nnpc = cpu.nnpc;

}


