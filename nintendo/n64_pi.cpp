#include <print>
#include <cstdio>
#include <cstring>
#include "n64.h"

static u8 sram[128*1024] = {0};

void n64::pi_dma(bool write)
{
	//u32 len = 0;
	if( write )
	{  // into RAM
		if( PI_CART_ADDR < 0x10000000 )
		{
			fprintf(stderr, "PI DMA: cart $%X, ram $%X, len $%X\n", PI_CART_ADDR, PI_DRAM_ADDR, PI_WR_LEN+1);
			u32 cart = PI_CART_ADDR - 0x8000000;		
			u32 ramaddr = (PI_DRAM_ADDR & 0x7ffffe);
			if( ramaddr & 7 )
			{
				//printf("Unaligned DMA\n");
				//exit(1);
			}
			u32 len = (PI_WR_LEN & 0xffFFff)+1;
			if( cart >= 0x8000 ) 
			{
				for(u32 i = 0; i < len; ++i) memset(mem.data()+ramaddr, 0, len);
			} else {
				memcpy(mem.data()+ramaddr, sram+cart, len);
			}
			for(u32 i = (ramaddr)&0x7ffffc; i < ((ramaddr+len+3)&0x7ffffc); i += 4) cpu.invalidate(i);
			PI_CART_ADDR += (len+1)&~1;
			PI_DRAM_ADDR += (len+7)&~7;
		} else {
			u32 length = (PI_WR_LEN & 0x00FFFFFF) + 1;
			u32 cart_addr = PI_CART_ADDR & 0xFFFFFFFE;
			u32 dram_addr = PI_DRAM_ADDR & 0x007FFFFE;

			if(dram_addr & 0x7 && length >= 0x7) 
			{
				length -= dram_addr & 0x7;
			}
			
			PI_WR_LEN = length;
			
			if( 0 ) //ramaddr & 7 )
			{
				//printf("Unaligned DMA\n");
				//exit(1);
			}
			//if( cart + len < ROM.size() )
			//{
				if( dram_addr + length > 0x7fffff )
				{
					std::println("dram ${:X}, len ${:X}, end addr = ${:X}", dram_addr, length, dram_addr+length);
					length = 0x800000 - dram_addr;
				}
				if( (cart_addr&0x0FFFffff) >= ROM.size() )
				{
					memset(mem.data()+dram_addr, 0, length);
				} else {
					memcpy(mem.data()+dram_addr, ROM.data()+(cart_addr-0x10000000), length);
				}
				//std::println("PI dma invalidating ${:X} to ${:X}", dram_addr, dram_addr+length);
				for(u32 i = dram_addr&0x7fffff; i < ((dram_addr+length)&0x7fffff); i += 4) cpu.invalidate(i);
			//} else {
			//	fprintf(stderr, "PI DMA: cart $%X, ram $%X, len $%X\n", cart, ramaddr, PI_WR_LEN+1);
			//	fprintf(stderr, "PI DMA: dma included data past ROM size of %i bytes\n", int(ROM.size()));
			//	memset(mem.data()+ramaddr, 0, len);
			//	memcpy(mem.data()+ramaddr, ROM.data()+cart, ROM.size()-cart);
			//}
			PI_DRAM_ADDR = dram_addr + length;
			PI_CART_ADDR = cart_addr + length;
		}
		//PI_CART_ADDR += (len+1)&~1;
		//PI_DRAM_ADDR += (len+7)&~7;
		//PI_DRAM_ADDR = dram_addr + length;
		//PI_CART_ADDR = cart_addr + length;
	} else {
		//todo: writing from RAM to cart's save ram
		printf("PI DMA write to sram? Addr = $%X\n", PI_CART_ADDR);
		u32 cart = PI_CART_ADDR - 0x8000000;
		u32 ramaddr = (PI_DRAM_ADDR & 0x7ffffe);
		if( ramaddr & 7 )
		{
			//printf("Unaligned DMA\n");
			//exit(1);
		}
		u32 len = (PI_RD_LEN & 0xffFFff)+1;
		
		memcpy(sram+cart, mem.data()+ramaddr, len);
		
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
		//pi_regs[reg] &= 0xffFFfe;
		return;
	}
	if( reg == 1 )
	{
		//pi_regs[reg] &= ~1;
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


