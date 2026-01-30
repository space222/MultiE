#include <print>
#include <string>
#include "util.h"
#include "ps2.h"

#define DMACHANRD(c) \
		switch( (addr>>4)&15 ) \
		{ \
		case 0: return eedma.chan[c].chcr;\
		case 1: return eedma.chan[c].madr;\
		case 2: return eedma.chan[c].qwc;\
		case 3: return eedma.chan[c].tadr;\
		case 4: return eedma.chan[c].asr0;\
		case 5: return eedma.chan[c].asr1;\
		case 8: return eedma.chan[c].sadr;\
		}\
		return 0\
		
#define assert32 do { if( sz != 32 ) { std::println("Wr{} to ${:X}", sz, addr); exit(1); } } while(0)
#define assert16 do { if( sz != 16 ) { std::println("Wr{} to ${:X}", sz, addr); exit(1); } } while(0)


u128 ps2::read(u32 addr, int sz)
{
	if( logall && addr != cpu.pc ) { std::println("EE Rd{} ${:X}", sz, addr); }
	if( addr >= 0x70000000u && addr < 0x80000000u ) return sized_read128(spad, addr&0x3fff, sz);	
	if( addr >= 0xFFFF8000u ) { addr = 0x78000 | (addr & 0x7fff); }	
	addr &= 0x1FFFffffu;	
	if( addr < 32*1024*1024 ) return sized_read128(RAM, addr, sz);
	if( addr >= 0x1FC00000u && addr < (0x1FC00000u+4_MB) ) return sized_read128(BIOS, addr&0x3fffff, sz);

	if( sz < 32 && addr >= 0x10000000 && addr < 0x13000000 )
	{
		u32 val = read(addr&~3, 32);
		if( sz == 8 ) return (val>>((addr&3)*8))&0xff;
		return (val>>((addr&1)*16))&0xffff;
	}
	
	if( sz > 32 ) { std::println("EE IO Rd{} ${:X}!!", sz, addr);  }

	switch( addr )
	{
	case 0x12001000: return gs.CSR;
	
	case 0x1000E000: return eedma.D_CTRL;
	case 0x1000E010: std::println("${:X}: ee D_STAT(${:X})", cpu.pc, eedma.D_STAT);  return eedma.D_STAT;
	case 0x1000E020: return eedma.D_PCR;
	case 0x1000E030: return eedma.D_SQWC;
	case 0x1000E040: return eedma.D_RBSR;
	case 0x1000E050: return eedma.D_RBOR;
	case 0x1000E060: return eedma.D_STADR;
	
	case 0x1000F520: return eedma.D_ENABLE;
	
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
	} // end of switch
	
	if( (addr & 0xffFFff00u) == 0x10008000 ) { DMACHANRD(0); } // VIF0 dma
	if( (addr & 0xffFFff00u) == 0x10009000 ) { DMACHANRD(1); } // VIF1 dma
	if( (addr & 0xffFFff00u) == 0x1000A000 ) { DMACHANRD(2); } // GIF dma
	if( (addr & 0xffFFff00u) == 0x1000B000 ) { DMACHANRD(3); }// IPU_FROM dma
	if( (addr & 0xffFFff00u) == 0x1000B400 ) { DMACHANRD(4); }// IPU_TO dma
	if( (addr & 0xffFFff00u) == 0x1000C000 ) { DMACHANRD(5); } // SIF0 dma
	if( (addr & 0xffFFff00u) == 0x1000C400 ) { DMACHANRD(6); } // SIF1 dma
	if( (addr & 0xffFFff00u) == 0x1000C800 ) { DMACHANRD(7); } // SIF2 dma
	if( (addr & 0xffFFff00u) == 0x1000D000 ) { DMACHANRD(8); } // SPR_FROM dma
	if( (addr & 0xffFFff00u) == 0x1000D800 ) { DMACHANRD(9); } // SPR_TO dma
	
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
	
	//todo: handle IO writes of less than 32bits
	
	switch( addr )
	{
	case 0x12001000:
		gs.CSR = (v&~CSR_VBINT) | (gs.CSR&~v&CSR_VBINT);
		return;
	
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
		
	case 0x1000F590: eedma.D_ENABLE = v; return;

	case 0x1000E000: eedma.D_CTRL = v; return;
	case 0x1000E010:
		eedma.D_STAT &= ~(v&0xffffu);
		eedma.D_STAT ^= (v&0xffff0000u);
				
		if( (eedma.D_STAT>>16) & (eedma.D_STAT&0xffff) )
		{
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
	
	if( (addr & 0xffFFff00u) == 0x1000A000 )
	{ // GIF dma
		assert32;
		u32 reg = (addr>>4)&15;
		std::println("EE GIF dma reg{} = ${:X}", reg, v);
		switch( reg )
		{
		case 0: {
			if( !(v & BIT(8)) )
			{
				eedma.chan[2].chcr = v;
				return;
			}
			if( ((v>>2)&3) == 1 )
			{ // chain mode.. yay.
				std::println("GIF Chain xfer");
				exit(1);
				return;
			}
			if( ((v>>2)&3) == 0 )
			{
				eedma.chan[2].chcr = v & ~BIT(8);
				std::println("xfer with qwc = ${:X}", eedma.chan[2].qwc);
				for(; eedma.chan[2].qwc > 0; --eedma.chan[2].qwc)
				{
					u128 F = *(u64*)&RAM[eedma.chan[2].madr];
					F |= u128(*(u64*)&RAM[eedma.chan[2].madr+8])<<64;
						// ^ directly pulling a u128 into the fifo segfaults
					gs.fifo.push_front(F);
					eedma.chan[2].madr += 16;
				}
				gs_run_fifo();
				eedma.D_STAT |= BIT(2);
				if( (eedma.D_STAT>>16) & (eedma.D_STAT&0xffff) )
				{
					cpu.cop0[13] |= BIT(11);
				} else {
					cpu.cop0[13] &=~BIT(11);
				}
			}
			}return;
		case 1: eedma.chan[2].madr = v; return;
		case 2: eedma.chan[2].qwc = v & 0xffff; return;
		case 3: eedma.chan[2].tadr = v; return;
		case 4: eedma.chan[2].asr0 = v; return;
		case 5: eedma.chan[2].asr1 = v; return;
		case 8: eedma.chan[2].sadr = v; return;
		}
		return;
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C000 )
	{ // SIF0 dma
		assert32;
		u32 reg = (addr>>4)&15;
		std::println("EE SIF0 dma ${:X} = ${:X}", addr, v);
		switch( reg )
		{
		case 0: 
		
			return;
		case 1: eedma.chan[5].madr = v; return;
		case 2: eedma.chan[5].qwc = v; return;
		case 3: eedma.chan[5].tadr = v; return;
		case 4: eedma.chan[5].asr0 = v; return;
		case 5: eedma.chan[5].asr1 = v; return;
		case 8: eedma.chan[5].sadr = v; return;
		}
		return;
	}
	
	if( (addr & 0xffFFff00u) == 0x1000C400 )
	{ // SIF1 dma
		assert32;
		u32 reg = (addr>>4)&15;
		std::println("EE SIF1 dma ${:X} = ${:X}", addr, v);
		switch( reg )
		{
		case 0: 
		
			return;
		case 1: eedma.chan[6].madr = v; return;
		case 2: eedma.chan[6].qwc = v; return;
		case 3: eedma.chan[6].tadr = v; return;
		case 4: eedma.chan[6].asr0 = v; return;
		case 5: eedma.chan[6].asr1 = v; return;
		case 8: eedma.chan[6].sadr = v; return;
		}
		return;
	}
	
	std::println("EE Wr{} ${:X} = ${:X}", sz, addr, v);
}

void ps2::ee_dma_chain(u32 c, std::function<void(u32)> where)
{

}

void ps2::ee_sif_dest_chain()
{

}

bool logall = false;

void ps2::run_frame()
{
	u64 target = last_target + 18740*262;
	sched.add_event(last_target + 18740*240, EVENT_VBLANK);
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
		while( global_stamp >= sched.next_stamp() )
		{
			sched.run_event();
		}
	}
	
	//eeint.INTC_STAT = 2;
	
	last_target = target;

}

void ps2::event(u64 old_stamp, u32 code)
{
	if( code == EVENT_VBLANK )
	{
		gs.CSR |= CSR_VBINT;
		return;
	}

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
	
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	if( fgetc(fp) == 0x7F && fgetc(fp) == 'E' && fgetc(fp) == 'L' && fgetc(fp) == 'F' )
	{
		fseek(fp, 0, SEEK_SET);
		reset();
		iop.reset();
		iop.c[15] = 0x1F;
		while( cpu.pc != 0x82000 )
		{
			cpu.step();
			for(u32 i = 0; i < 8; ++i) iop.step();
		}
		cpu.pc = loadELF(fp, [&](u32 addr, u8 v) { RAM[addr&(32_MB-1)] = v; });
		cpu.npc = cpu.pc + 4;
		cpu.nnpc = cpu.npc + 4;
		std::println("MultiE: loaded ELF, pc=${:X}", cpu.pc);
		loaded_elf = true;
		elf_entry = cpu.pc;
		return true;
	}
	
	loaded_elf = false;
	return true;
}

void ps2::reset()
{
	cpu.pc = loaded_elf ? elf_entry : 0xbfc00000u;
	cpu.npc = cpu.pc + 4;
	cpu.nnpc = cpu.npc + 4;
	cpu.cop0[15] = 0x59;
	if( loaded_elf ) return;
	iop.reset();
	iop.c[15] = 0x1F;
	iop.pc = cpu.pc;
	iop.npc = cpu.npc;
	iop.nnpc = cpu.nnpc;
	sched.reset();
}


