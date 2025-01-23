#include <cstring>
#include "n64.h"

void n64::pi_dma(bool write)
{
	u32 len = 0;
	if( write )
	{  // into RAM
		u32 cart = PI_CART_ADDR - 0x10000000;
		if( cart >= ROM.size() ) return;
		u32 ramaddr = (PI_DRAM_ADDR & 0x7ffffe);
		//fprintf(stderr, "PI DMA: cart $%X, ram $%X, len $%X\n", cart, ramaddr, PI_WR_LEN+1);
		len = (PI_WR_LEN & 0xffFFff)+1;
		if( cart + len < ROM.size() )
		{
			memcpy(mem.data()+ramaddr, ROM.data()+cart, len);
		} else {
			memset(mem.data()+ramaddr, 0, len);
			//fprintf(stderr, "PI DMA: dma included data past ROM size of %i bytes\n", int(ROM.size()));
			memcpy(mem.data()+ramaddr, ROM.data()+cart, ROM.size()-cart);
		}
		PI_CART_ADDR += (len+1)&~1;
		PI_DRAM_ADDR += (len+7)&~7;
	} else {
		//todo: writing from RAM to cart's save ram
		len = (PI_WR_LEN & 0xffFFff)+1;
		PI_CART_ADDR += (len+1)&~1;
		PI_DRAM_ADDR += (len+7)&~7;
	}
	
	//pi_cycles_til_irq = len * 4;
	//PI_STATUS |= 3;
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
	addr = (addr&0x3F)>>2;
	if( addr == 4 )
	{
		return PI_STATUS | ((MI_INTERRUPT & BIT(MI_INTR_PI_BIT)) ? 8 : 0);
	}
	return pi_regs[addr];
}


