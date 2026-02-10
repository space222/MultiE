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
		iop_dma.DICR.b.mflag = (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		return;
	
	case 0x1F801574:
		std::println("IOP DICR2 Wr{} = ${:X}", sz, v);
		iop_dma.DICR2.b.tag = v;
		iop_dma.DICR2.b.mask = (v>>16);
		iop_dma.DICR2.b.flag &= ~(v>>24);
		iop_dma.DICR.b.mflag = (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		return;
	case 0x1F801570: iop_dma.DPCR2 = v; return;
	case 0x1F801578: iop_dma.DMACEN = v; return;
	case 0x1F80157C: std::println("IOP dma DMACINTEN=${:X}", v); iop_dma.DMACINTEN = v; return;
	
	case 0x1D000010: std::println("IOP SMCOM = ${:X}", v); sifmb.SIF_SMCOM = v; return;
	case 0x1D000020: std::println("IOP MSFLG &=~${:X}", v); sifmb.SIF_MSFLG &= ~v; return;
	case 0x1D000030: std::println("IOP SMFLG |= ${:X}", v); sifmb.SIF_SMFLG |= v; return;

	case 0x1F402017: cdvd.Sparam.push_front(u8(v)); return;
	case 0x1F402016: cdvd_cmd(u8(v)); return;

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

	case 0x1F900744: return 0x80;
	
	case 0x1F402005: return 0x40; // N cmd stat ready. 
		// $4C causes very early OpenConfig + same as just $40
		// $40 results in ForbidDVD after OSDSND late
		// either way, after handling ForbidDVD, a few more SIF xfers then spin
	case 0x1F402017: return (cdvd.Sres.empty() ? 0x40:0); // S command status
	case 0x1F402018: if( !cdvd.Sres.empty() ) { u8 t = cdvd.Sres.back(); cdvd.Sres.pop_back(); return t; } return 0;
	
	case 0x1F8014A4: return BIT(11);
	
	//undoc
	case 0x1F801450: return 0;
	default: 
		if( sz < 32 )
		{
			u32 v = iop_read(a&~3, 32);
			if( sz == 8 ) return (v>>((a&3)*8));
			return (v>>((a&1)*16));
		}
		break;
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

void ps2::iop_dma_ctrl(u32 c, u32 v) 
{
	switch( c )
	{
	case 4:{ // SPU1
		iop_dma.chan[4][2] &= ~BIT(24);
		
		if( iop_dma.DICR.b.ie && (iop_dma.DICR.b.mask&BIT(4)) ) iop_dma.DICR.b.flag |= BIT(4);
		u32 oldflag = iop_dma.DICR.b.mflag;
		iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		if( iop_dma.DICR.b.mflag == 1 )
		{
			iop_int.I_STAT |= BIT(3);
		}
	
		}return;
	case 7:{ // SPU2
		iop_dma.chan[7][2] &= ~BIT(24);
		
		if( iop_dma.DICR.b.ie && (iop_dma.DICR2.b.mask&BIT(0)) ) iop_dma.DICR2.b.flag |= BIT(0);
		u32 oldflag = iop_dma.DICR.b.mflag;
		iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
		if( iop_dma.DICR.b.mflag == 1 )
		{
			iop_int.I_STAT |= BIT(3);
		}
	
		}return;
	case 9: // SIF0 (IOP->EE)
		std::println("SIF0 ctrl = ${:X}", v);
		if( v & BIT(24) )
		{
			bool end = false;
			bool irq = false;
			while( !end )
			{
				u32 addr = *(u32*)&iop_ram[iop_dma.chan[9][3]&0x1fffff];
				iop_dma.chan[9][3] += 4;
				u32 size = *(u32*)&iop_ram[iop_dma.chan[9][3]&0x1fffff];
				iop_dma.chan[9][3] += 4;
				size &= 0xffFFff;
				std::println("SIF0 size = {} words", size);
				end = end || addr & BIT(31);
				//irq = irq || addr & BIT(30);
				addr &= 0x1fffff;
				if( iop_dma.chan[9][2] & BIT(8) )
				{
					u128 F = *(u32*)&iop_ram[iop_dma.chan[9][3]&0x1fffff];
					iop_dma.chan[9][3] += 4;
					F |= u64(*(u32*)&iop_ram[iop_dma.chan[9][3]&0x1fffff])<<32;
					iop_dma.chan[9][3] += 4;
					std::println("IOP->EE SIF tte ${:X}", F);
					eedma.sif_fifo.push_front(F);
				}
				for(u32 i = 0; i < (size+3)/4; ++i, addr+=16)
				{
					u128 F = *(u32*)&iop_ram[addr];
					F |= u128(*(u32*)&iop_ram[addr+4])<<32;
					F |= u128(*(u32*)&iop_ram[addr+8])<<64;
					F |= u128(*(u32*)&iop_ram[addr+12])<<96;
					std::println("IOP->EE SIF ${:X}", F);
					eedma.sif_fifo.push_front(F);
				}
			}
			if( eedma.sif_active ) { ee_sif_dest_chain(); }
			iop_dma.chan[9][2] &= ~BIT(24);
			
			if( iop_dma.DICR.b.ie && (iop_dma.DICR2.b.mask&BIT(2)) ) iop_dma.DICR2.b.flag |= BIT(2);
			u32 oldflag = iop_dma.DICR.b.mflag;
			iop_dma.DICR.b.mflag = (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
			if( iop_dma.DICR.b.mflag == 1 )
			{
				std::println("iop sif0 dma compl. irq");
				iop_int.I_STAT |= BIT(3);
			}
		}
		return;
	case 10: // SIF1 (EE->IOP)
		iop_dma.sif_active = v & BIT(24);
		if( iop_dma.sif_active )
		{
			iop_sif_dest_chain();
		}
		return;
	}
	std::println("iop dma {} ctrl = ${:X}", c, v);
	if( v & BIT(24) ) exit(1);
}

void ps2::iop_sif_dest_chain()
{
	bool end = false;
	bool irq = false;
	while( !end )
	{
		if( iop_dma.sif_fifo.size() < 2 ) return;
		if( iop_dma.sif_fifo.size() < ((iop_dma.sif_fifo[iop_dma.sif_fifo.size()-2]&0xffFFff)+1) ) return;
		u32 tag = iop_dma.sif_fifo.back(); iop_dma.sif_fifo.pop_back();
		irq = irq || (tag&BIT(30));
		end = end || (tag&BIT(31));
		u32 size = iop_dma.sif_fifo.back(); iop_dma.sif_fifo.pop_back();
		iop_dma.sif_fifo.pop_back();
		iop_dma.sif_fifo.pop_back(); //todo: IOP dest tags always a QWORD, but top 64bits is junk?
		for(u32 i = 0; i < size; ++i)
		{
			std::println("iop sif dest [${:X}] = ${:X}", tag&0x1fffff, iop_dma.sif_fifo.back());
			*(u32*)&iop_ram[(tag&0x1fffff)] = iop_dma.sif_fifo.back();
			iop_dma.sif_fifo.pop_back();
			tag+=4;
		}
	}
	iop_dma.sif_active = false;
	iop_dma.chan[10][2] &= ~BIT(24);
	
	if( (iop_dma.DICR2.b.tag & BIT(10)) && irq ) 
	{
		iop_int.I_STAT |= BIT(3);
		std::println("iop sif1 dma irq bit");
	}
	if( iop_dma.DICR.b.ie && (iop_dma.DICR2.b.mask&BIT(3)) ) iop_dma.DICR2.b.flag |= BIT(3);
	u32 oldflag = iop_dma.DICR.b.mflag;
	iop_dma.DICR.b.mflag = iop_dma.DICR.b.ie && (iop_dma.DICR.b.flag || iop_dma.DICR2.b.flag);
	if( iop_dma.DICR.b.mflag == 1 )
	{
		iop_int.I_STAT |= BIT(3);
		std::println("iop sif1 dma compl. irq");
	}
}

void ps2::cdvd_cmd(u8 cmd)
{
	switch( cmd )
	{
	case 0x15: // ForbidDVD
		std::println("CDVD: ForbidDVD({})", cdvd.Sparam);
		cdvd.Sparam.clear();
		cdvd.Sres.push_front(5);
		break;
	case 0x40: // OpenConfig
		std::println("CDVD: OpenConfig({})", cdvd.Sparam);
		cdvd.Sparam.clear();
		cdvd.Sres.push_front(0);
		break;
	case 0x41: // ReadConfig
		std::println("CDVD: ReadConfig({})", cdvd.Sparam);
		cdvd.Sparam.clear();
		for(u32 i = 0; i < 2; ++i)
		{
				cdvd.Sres.push_front(0);
		}
		break;
	case 0x43: // CloseConfig
		std::println("CDVD: CloseConfig({})", cdvd.Sparam);
		cdvd.Sparam.clear();
		cdvd.Sres.push_front(0);
		break;
	default:
		std::println("CDVD: Unimpl cmd = ${:X}", cmd);
		exit(1);
	}


}







