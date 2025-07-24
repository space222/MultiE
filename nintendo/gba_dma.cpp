#include <print>
#include "gba.h"

void gba::exec_dma(int chan)
{
	const int base = chan*6;
	const u16 ctrl = dmaregs[base + 5];
	u16 len = dmaregs[base + 4];
	if( len == 0 ) { len = ((chan==3) ? 0x10000 : 0x4000); }
	int srcaddr = 0;
	int dstaddr = 0;
	memcpy(&srcaddr, &dmaregs[base + 0], 4);
	memcpy(&dstaddr, &dmaregs[base + 2], 4);
	
	//std::println("DMA{} from ${:X} to ${:X}, {} units", chan, srcaddr, dstaddr, len);
	
	const bool B32 = ctrl & BIT(10);
	int src_inc = B32?4:2;
	int dst_inc = B32?4:2;
	if( ((ctrl>>7)&3) == 1 ) src_inc = -src_inc;
	else if( ((ctrl>>7)&3) == 2 ) src_inc = 0;
	if( ((ctrl>>5)&3) == 1 ) dst_inc = -dst_inc;
	else if( ((ctrl>>5)&3) == 2 ) dst_inc = 0;
	
	for(u32 i = 0; i < len; ++i)
	{
		u32 val = read(srcaddr, B32?32:16, ARM_CYCLE::X);
		write(dstaddr, val&(B32?0xffffFFFFu:0xffffu), B32?32:16, ARM_CYCLE::X);
		
		dstaddr += dst_inc;
		srcaddr += src_inc;
	}
	
	if( ctrl & BIT(14) )
	{
		ISTAT |= BIT(8 + chan);
		check_irqs();
	}
}

void gba::write_dma_io(u32 addr, u32 v)
{
	//std::println("DMA ${:X} = ${:X}", addr, v);
	u16 oldval = dmaregs[(addr - 0x040000B0u)>>1];
	dmaregs[(addr - 0x040000B0u)>>1] = v;
	
	if( addr == 0x40000BA && (v & 0xB000) == 0x8000 ) { exec_dma(0); }
	else if( addr == 0x40000C6 && (v & 0xB000) == 0x8000 ) { exec_dma(1); }
	else if( addr == 0x40000D2 && (v & 0xB000) == 0x8000 ) { exec_dma(2); }
	else if( addr == 0x40000DE && (v & 0xB000) == 0x8000 ) { exec_dma(3); }
	//std::println("DMA ${:X} = ${:X}", addr, v);
}

u32 gba::read_dma_io(u32 addr)
{
	addr -= 0x040000B0u;
	return dmaregs[addr>>1];
}

