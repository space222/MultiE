#include <print>
#include "gba.h"


void gba::write_dma_io(u32 addr, u32 v)
{
	addr -= 0x040000B0u;
	u16 oldval = dmaregs[addr>>1];
	dmaregs[addr>>1] = v;
	
	if( (v&0x8000) == 0x8000 )
	{
		if( addr == 0xA  )
		{
			u32 srcaddr = 0;
			u32 dstaddr = 0;
			memcpy(&srcaddr, &dmaregs[0], 4);
			memcpy(&dstaddr, &dmaregs[2], 4);
			std::println("DMA0 src = ${:X}, dst = ${:X}", srcaddr, dstaddr);
			if( v&0x4000 )
			{
				//ISTAT |= BIT(8);
				//check_irqs();
			}
			dmaregs[addr>>1] &= ~BIT(15);
		} else if( addr == 0x16 ) {
			u32 srcaddr = 0;
			u32 dstaddr = 0;
			memcpy(&srcaddr, &dmaregs[6], 4);
			memcpy(&dstaddr, &dmaregs[8], 4);
			if( dstaddr != 0x040000A0 ) std::println("DMA1 src = ${:X}, dst = ${:X}", srcaddr, dstaddr);
		} else if( addr == (0xDE - 0xb0)/2 ) {
			u32 srcaddr = 0;
			u32 dstaddr = 0;
			memcpy(&srcaddr, &dmaregs[18], 4);
			memcpy(&dstaddr, &dmaregs[20], 4);
			std::println("DMA3 src = ${:X}, dst = ${:X}", srcaddr, dstaddr);
			if( v&0x4000 )
			{
				//ISTAT |= BIT(8);
				//check_irqs();
			}
			dmaregs[addr>>1] &= ~BIT(15);		
		} else if( addr == (0xD2 - 0xb0)/2 ) {
			u32 srcaddr = 0;
			u32 dstaddr = 0;
			memcpy(&srcaddr, &dmaregs[12], 4);
			memcpy(&dstaddr, &dmaregs[14], 4);
			std::println("DMA2 src = ${:X}, dst = ${:X}", srcaddr, dstaddr);
			if( v&0x4000 )
			{
				//ISTAT |= BIT(8);
				//check_irqs();
			}
			dmaregs[addr>>1] &= ~BIT(15);		
		}
	}
	//std::println("DMA ${:X} = ${:X}", addr, v);

	

}

u32 gba::read_dma_io(u32 addr)
{
	addr -= 0x040000B0u;
	return dmaregs[addr>>1];
}

