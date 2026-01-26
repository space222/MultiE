#include <print>
#include "ps2.h"
#include "util.h"

void ps2::iop_write(u32 a, u32 v, int sz)
{
	if( a >= 0xFFFE0000u ) return;
	a &= 0x1FFFffffu;

	if( a < 2_MB ) { if( (iop.c[12] & 0x10000) ) { return; } else { return sized_write(iop_ram, a, v, sz); }}
	if( a >= 0x1F800000 && a < 0x1F8003FF ) return sized_write(iop_spad, a&0x3ff, v, sz);

	switch( a )
	{
	case 0x1F801070: iop_int.I_STAT &= v; return;
	case 0x1F801074: iop_int.I_MASK = v; return;
	case 0x1F801078: iop_int.I_CTRL = v&1; return;
	
	case 0x1F8010F4:
		std::println("IOP DICR1 Wr{} = ${:X}", sz, v);
		iop_dma.DICR.b.ctrl = v;
		iop_dma.DICR.b.mask = v>>16;
		iop_dma.DICR.b.ie = v>>23;
		iop_dma.DICR.b.flag &= ~(v>>24);
		iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		return;
	
	case 0x1F801574:
		std::println("IOP DICR2 Wr{} = ${:X}", sz, v);
		iop_dma.DICR2.b.tag = v;
		iop_dma.DICR2.b.mask = (v>>16);
		iop_dma.DICR2.b.flag &= ~(v>>24);
		iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		return;
	case 0x1F801570: iop_dma.DPCR2 = v; return;
	case 0x1F801578: iop_dma.DMACEN = v; return;
	case 0x1F80157C: iop_dma.DMACINTEN = v; return;
	
	case 0x1D000010: std::println("IOP SMCOM = ${:X}", v); sifmb.SIF_SMCOM = v; return;
	case 0x1D000020: std::println("IOP MSFLG &=~${:X}", v); sifmb.SIF_MSFLG &= ~v; return;
	case 0x1D000030: std::println("IOP SMFLG |= ${:X}", v); sifmb.SIF_SMFLG |= v; return;

	//undoc
	case 0x1F801450: return;
	}
	
	if( (a >= 0x1F801080 && a < 0x1F8010F0) || (a >= 0x1F801500 && a < 0x1F801560) )
	{
		u32 dma_channel = ((a>>4)&15);
		if( a >= 0x1F801500 )
		{
			dma_channel += 7;
		} else {
			dma_channel -= 8;
		}
		u32 reg = (a>>2)&3;
		iop_dma.chan[dma_channel][reg] = v;
		if( reg == 2 )
		{
			iop_dma_ctrl(dma_channel, v);
		}
		return;
	}
	
	std::println("IOP Unimpl Wr{} ${:X} = ${:X}", sz, a, v);
}


u32 ps2::iop_read(u32 a, int sz)
{
	if( a >= 0xFFFE0000u ) return 0;
	a &= 0x1FFFffffu;

	if( a < 2_MB ) return sized_read(iop_ram, a, sz);
	if( a >= 0x1FC00000 && a < 0x1FC00000+4_MB ) return sized_read(BIOS, a&0x3fffff, sz);
	if( a >= 0x1F800000 && a < 0x1F8003FF ) return sized_read(iop_spad, a&0x3ff, sz);

	switch( a )
	{
	case 0x1F801074: return iop_int.I_MASK;
	case 0x1F801070: return iop_int.I_STAT;
	case 0x1F801078: { u32 temp = iop_int.I_CTRL; iop_int.I_CTRL&=~1; return temp; }
	
	case 0x1F8010F4: return iop_dma.DICR.v;
	
	case 0x1F801570: return iop_dma.DPCR2;
	case 0x1F801574: return iop_dma.DICR2.v;
	case 0x1F801578: return iop_dma.DMACEN;
	case 0x1F80157C: return iop_dma.DMACINTEN;

	case 0x1D000000: return sifmb.SIF_MSCOM;
	case 0x1D000010: return sifmb.SIF_SMCOM;
	case 0x1D000020: return sifmb.SIF_MSFLG;
	case 0x1D000030: return sifmb.SIF_SMFLG;
	case 0x1D000060: return 0xff;


	case 0x1F402005: return 0x40; // CDVD drive status = READY
	
	//undoc
	case 0x1F801450: return 0;
	default: break;
	}
	
	if( (a >= 0x1F801080 && a < 0x1F8010F0) || (a >= 0x1F801500 && a < 0x1F801560) )
	{
		u32 dma_channel = ((a>>4)&15);
		if( a >= 0x1F801500 )
		{
			dma_channel += 7;
		} else {
			dma_channel -= 8;
		}
		u32 reg = (a>>2)&3;
		if( sz != 32 ) { std::println("warning: {}bit read ${:X}", sz, a); }
		return iop_dma.chan[dma_channel][reg];
	}
	
	if( a >= 0x1E000000u && a < 0x1EFF0000u ) return 0; //undoc?
	
	std::println("IOP Unimpl Rd{} ${:X}", sz, a);
	return 0;
}

void ps2::iop_dma_ctrl(u32 c,u32 v) 
{
	switch( c )
	{
	case 9:
		if( v & BIT(24) )
		{
			std::println("IOP->EE SIF0 DMA Start! ctrl=${:X}", v);
			u32 &TADR = iop_dma.chan[9][3];
			std::println("TADR = ${:X}", TADR);
			bool end = false;
			bool do_irq = false;
			while( !end )
			{
				u32 start = *(u32*)&iop_ram[TADR & 0x1fffff];
				u32 len =   *(u32*)&iop_ram[(TADR& 0x1fffff)+4];
				end = start & BIT(31);
				start &= 0x3fffFFFFu;
				iop_dma.chan[9][0] = start + len*4;
				if( iop_dma.chan[9][2] & BIT(8) )
				{
					std::println("xfer ${:X}", *(u32*)&iop_ram[(TADR& 0x1fffff)+8]);
					eedma.sif_fifo.push_front(*(u32*)&iop_ram[(TADR& 0x1fffff)+8]);
					std::println("xfer ${:X}", *(u32*)&iop_ram[(TADR& 0x1fffff)+12]);
					eedma.sif_fifo.push_front(*(u32*)&iop_ram[(TADR& 0x1fffff)+12]);
				}
				TADR += 16;
				for(u32 i = 0; i < len; ++i)
				{
					u32 p = *(u32*)&iop_ram[(start&0x1fffff)+i*4];
					std::println("xfer ${:X}", p);
					eedma.sif_fifo.push_front(p);
				}
			}
			if( eedma.chan[5][0] & BIT(8) )
			{
				ee_sif_dest_chain();
			}
			iop_dma.chan[9][2] &= ~BIT(24);
			if( iop_dma.DICR.b.ie && (iop_dma.DICR2.b.mask & BIT(2)) ) { iop_dma.DICR2.b.flag |= BIT(2); }
			if( do_irq ) { iop_dma.DICR2.b.tag |= BIT(9); iop_int.I_STAT |= BIT(3); }
			if( iop_dma.DICR.b.ie )
			{
				auto oldflag = iop_dma.DICR.b.mflag;
				iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
				if( oldflag==0 && iop_dma.DICR.b.mflag==1 ) iop_int.I_STAT |= BIT(3);
			}
		}
		return;
	case 10:
		if( v & BIT(24) )
		{
			std::println("IOP SIF1 DMA Active! ctrl=${:X}", v);
			if( !iop_dma.sif_fifo.empty() )
			{
				iop_sif_dest_chain();
			}
		}	
		return;
	}
	std::println("iop dma {} ctrl = ${:X}", c, v);
}

void ps2::iop_sif_dest_chain()
{
	bool end = false;
	bool do_irq = false;
	while( !end )
	{
		u32 start = iop_dma.sif_fifo.back();
		end = start & BIT(31);
		do_irq = do_irq || (start & BIT(30));
		start &= 0xFFffFF;
		iop_dma.sif_fifo.pop_back();
		u32 size = iop_dma.sif_fifo.back();
		iop_dma.sif_fifo.pop_back();
		iop_dma.sif_fifo.pop_back();
		iop_dma.sif_fifo.pop_back();
		for(u32 i = 0; i < size; ++i)
		{
			u32 p = iop_dma.sif_fifo.back(); iop_dma.sif_fifo.pop_back();
			std::println("IOP SIF writing ${:X} = ${:X}", start, p);
			*(u32*)&iop_ram[start] = p;
			start += 4;
		}
	}
	iop_dma.chan[10][2] &= ~BIT(24);
	if( do_irq ) { iop_int.I_STAT |= BIT(3); }
	if( iop_dma.DICR.b.ie && (iop_dma.DICR2.b.mask & BIT(3)) ) { iop_dma.DICR2.b.flag |= BIT(3); }
	if( iop_dma.DICR.b.ie )
	{
		auto oldflag = iop_dma.DICR.b.mflag;
		iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		if( oldflag==0 && iop_dma.DICR.b.mflag==1 ) iop_int.I_STAT |= BIT(3);
	}
}



