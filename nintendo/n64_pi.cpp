#include <cstring>
#include "n64.h"

void n64::pi_dma(bool write)
{
	if( write )
	{  // into RAM
		u32 cart = PI_CART_ADDR - 0x10000000;
		if( cart >= ROM.size() ) return;
		u32 ramaddr = (PI_DRAM_ADDR & 0x7ffffe);
		fprintf(stderr, "PI DMA: cart $%X, ram $%X, len $%X\n", cart, ramaddr, PI_WR_LEN+1);
		u32 len = (PI_WR_LEN & 0xffFFff)+1;
		memcpy(mem.data()+ramaddr, ROM.data()+cart, len);
		PI_CART_ADDR += (len+1)&~1;
		PI_DRAM_ADDR += (len+7)&~7;
	} else {
		//todo: writing from RAM to cart's save ram
	}
	
	PI_STATUS |= BIT(3);
	raise_mi_bit(MI_INTR_PI_BIT);
	//printf("PI irq raised. mask = $%X, intr = $%X\n", MI_MASK, MI_INTERRUPT);
}

void n64::pi_write(u32 addr, u32 v)
{
	u32 reg = (addr&0x3F)>>2;
	if( reg > 12 ) return;
	if( reg == 4 )
	{
		if( v & BIT(1) )
		{
			printf("PI: IRQ cleared\n");
			PI_STATUS &= ~BIT(3);
			clear_mi_bit(MI_INTR_PI_BIT);
		}
		return;
	}
	pi_regs[reg] = v;
	if( reg == 2 )
	{
		pi_dma(false);
		return;
	}
	if( reg == 3 ) 
	{
		pi_dma(true);
		return;
	}
	if( reg == 0 )
	{
		pi_regs[reg] &= 0xffFFfe;
		return;
	}
	if( reg == 1 )
	{
		pi_regs[reg] &= ~1;
		return;
	}
}

u32 n64::pi_read(u32 addr)
{
	return pi_regs[(addr&0x3F)>>2];
}


