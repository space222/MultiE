#include <cstdio>
#include <cstring>
#include <cstdlib>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include "types.h"

extern u8 palette[0x400];
extern u8 OAM[0x400];
extern u8 VRAM[0x18000];
extern u8 SRAM[128*1024];
extern u16 IF;

void mem_write8(u32, u8);
void mem_write32(u32, u32);
u32 mem_read32(u32);
u16 mem_read16(u32);
void mem_write16(u32, u16);

u16 dma_regs[24] = {0};

bool eeprom_first = true;
u32 eeprom_size = 0;
u32 eeprom_baddr = 0;
u32 eeprom_cmd = 0;

void dma_write16(u32 addr, u16 val)
{
	u32 src_addr = 0;
	u32 dst_addr = 0;
	u32 ctrl = 0;
	u32 len = 0;
	u32 which = 0;
	if( addr > 0x40000DE ) return;
	dma_regs[(addr-0x40000B0)>>1] = val;
	
	bool exec = false;
	if( addr == 0x40000BA )
	{
		exec = true;
		src_addr = dma_regs[0] | (dma_regs[1]<<16);
		src_addr &= 0x7FFFFFF;
		dst_addr = dma_regs[2] | (dma_regs[3]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[4];
		ctrl = dma_regs[5];
		//if( (ctrl>>12)&3 ) return;
		dma_regs[5] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
	} else if( addr == 0x40000C6 ) {
		exec = true;
		src_addr = dma_regs[6] | (dma_regs[7]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[8] | (dma_regs[9]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[10];
		ctrl = dma_regs[11];
		//if( (ctrl>>12)&3 ) return;
		dma_regs[11] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
		which = 1;
	} else if( addr == 0x40000D2 ) {
		exec = true;
		src_addr = dma_regs[12] | (dma_regs[13]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[14] | (dma_regs[15]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[16];
		ctrl = dma_regs[17];
		//if( (ctrl>>12)&3 ) return;
		dma_regs[17] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
		which = 2;
	} else if( addr == 0x40000DE ) {
		exec = true;
		src_addr = dma_regs[18] | (dma_regs[19]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[20] | (dma_regs[21]<<16);
		dst_addr &= 0x0FFFFFFF;
		len = dma_regs[22];
		ctrl = dma_regs[23];
		//if( (ctrl>>12)&3 ) return;
		dma_regs[23] &= 0x7fff;
		if( len == 0 ) len = 0x10000;
		which = 3;
	}
	
	if( !exec || !(ctrl&0x8000) || ((ctrl>>12)&3) ) return;
	
	printf("DMA! ctrl=$%X from=%X, to=%X, len=$%X\n", ctrl, src_addr, dst_addr, len);
	if( dst_addr > 0x4000056 && dst_addr < 0x5000000 ) return;
	if( dst_addr == 0 || src_addr == 0 ) return;
	
	if( dst_addr == 0x0d000000 || dst_addr == 0x0DFFFF00 )
	{ //16bit?
		if( eeprom_cmd == 0 )
		{
			for(u32 i = 0; i < 2; ++i)
			{
				eeprom_cmd = (eeprom_cmd<<1) | (mem_read16(src_addr)&1); src_addr+=2;
			}
			if( eeprom_first )
			{
				eeprom_first = false;
				if( eeprom_cmd == 3 ) eeprom_size = (len>9);
				else eeprom_size = (len>73);
			}
			eeprom_baddr = 0;
			for(u32 i = 0; i < (eeprom_size ? 14 : 6); ++i)
			{
				eeprom_baddr = (eeprom_baddr<<1) | (mem_read16(src_addr)&1); src_addr+=2;
			}
			eeprom_baddr <<= 6;
			//printf("EEPROM Addr = $%X\n", eeprom_baddr);
			// get to end here for read setup. technically there's a last zero as part
			// of the protocol, but actually need it for emulating.
			if( eeprom_cmd == 3 )
			{  //todo: DMA completion IRQ important here?
				//printf("EEPROM Read Setup!\n");
				return;	
			}
			
			// if we get here, eeprom writes are done with a single DMA
			// rest of data is actual bits to write to eeprom (other than a stop bit)
			eeprom_cmd = 0;
			len -= 3; // 2 start and 1 stop bits
			len -= (eeprom_size ? 6 : 14);  // sub out the address bits
			//printf("EEPROM Write(len=%i): cmd=%i, addr=$%X.\n", len, eeprom_cmd, eeprom_baddr);
			
			for(u32 i = 0; i < len; ++i)
			{
				u16 v = mem_read16(src_addr); src_addr += 2;
				u32 bit = 7 - (eeprom_baddr&7);
				SRAM[eeprom_baddr>>3] &= ~(1u<<bit);
				SRAM[eeprom_baddr>>3] |= (v&1)<<bit;
				eeprom_baddr++;
			}
			return;
		}
		if( ctrl & (1u<<14) )
		{
			IF |= (1u<<(which+8));
		}
		return;
	} else if( src_addr == 0x0d000000 || src_addr == 0x0DFFFF00 ) {
		if( eeprom_cmd != 3 ) return;
		eeprom_cmd = 0;
		//printf("Read EEPROM Addr = $%X\n", eeprom_baddr);
		dst_addr += 8;
		for(u32 i = 0; i < 64; ++i)
		{
			u16 v = SRAM[eeprom_baddr>>3]>>(7-(eeprom_baddr&7));
			eeprom_baddr++;
			mem_write16(dst_addr, v&1); dst_addr += 2;
		}
		if( ctrl & (1u<<14) )
		{
			IF |= (1u<<(which+8));
		}
		return;
	} 
	
	for(u32 i = 0; i < len; ++i)
	{
		if( ctrl & (1u<<10) )
		{
			u32 sval = mem_read32(src_addr);
			mem_write32(dst_addr, sval);
			if( ((ctrl>>5)&3) == 0 || ((ctrl>>5)&3) == 3 )
				dst_addr += 4;
			else if( ((ctrl>>5)&3) == 1 )
				dst_addr -= 4;
			if( ((ctrl>>7)&3) == 0 )
				src_addr += 4;
			else if( ((ctrl>>7)&3) == 1 )
				src_addr -= 4;
		} else {
			u16 sval = mem_read16(src_addr);
			mem_write16(dst_addr, sval);
			if( ((ctrl>>5)&3) == 0 || ((ctrl>>5)&3) == 3 )
				dst_addr += 2;
			else if( ((ctrl>>5)&3) == 1 )
				dst_addr -= 2;
			if( ((ctrl>>7)&3) == 0 )
				src_addr += 2;
			else if( ((ctrl>>7)&3) == 1 )
				src_addr -= 2;		
		}
	}
	
	if( ctrl & (1u<<14) )
	{
		IF |= (1u<<(which+8));
	}
	return;
}

/*void dma_exec(int which)
{
	u32 ctrl, src_addr, dst_addr, len, creg;
	if( which == 0 )
	{
		src_addr = dma_regs[0] | (dma_regs[1]<<16);
		src_addr &= 0x7FFFFFF;
		dst_addr = dma_regs[2] | (dma_regs[3]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[4];
		ctrl = dma_regs[5];
		creg = 5;
		dma_regs[5] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
	} else if( which == 1 ) {
		src_addr = dma_regs[6] | (dma_regs[7]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[8] | (dma_regs[9]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[10];
		ctrl = dma_regs[11];
		creg = 11;
		dma_regs[11] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
		which = 1;
	} else if( which == 2 ) {
		src_addr = dma_regs[12] | (dma_regs[13]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[14] | (dma_regs[15]<<16);
		dst_addr &= 0x07FFFFFF;
		len = dma_regs[16];
		ctrl = dma_regs[17];
		creg = 17;
		dma_regs[17] &= 0x7fff;
		if( len == 0 ) len = 0x4000;
	} else {
		src_addr = dma_regs[18] | (dma_regs[19]<<16);
		src_addr &= 0x0FFFFFFF;
		dst_addr = dma_regs[20] | (dma_regs[21]<<16);
		dst_addr &= 0x0FFFFFFF;
		len = dma_regs[22];
		ctrl = dma_regs[23];
		creg = 23;
		dma_regs[23] &= 0x7fff;
		if( len == 0 ) len = 0x10000;
	}
	
	printf("DMA! ctrl=$%X from=%X, to=%X, len=$%X\n", ctrl, src_addr, dst_addr, len);

	if( dst_addr > 0x4000056 && dst_addr < 0x5000000 ) return;
	if( dst_addr == 0 || src_addr == 0 ) return;
	
	for(u32 i = 0; i < len; ++i)
	{
		if( ctrl & (1u<<10) )
		{
			u32 sval = mem_read32(src_addr);
			mem_write32(dst_addr, sval);
			if( ((ctrl>>5)&3) == 0 || ((ctrl>>5)&3) == 3 )
				dst_addr += 4;
			else if( ((ctrl>>5)&3) == 1 )
				dst_addr -= 4;
			if( ((ctrl>>7)&3) == 0 )
				src_addr += 4;
			else if( ((ctrl>>7)&3) == 1 )
				src_addr -= 4;
		} else {
			u16 sval = mem_read16(src_addr);
			mem_write16(dst_addr, sval);
			if( ((ctrl>>5)&3) == 0 || ((ctrl>>5)&3) == 3 )
				dst_addr += 2;
			else if( ((ctrl>>5)&3) == 1 )
				dst_addr -= 2;
			if( ((ctrl>>7)&3) == 0 )
				src_addr += 2;
			else if( ((ctrl>>7)&3) == 1 )
				src_addr -= 2;		
		}
	}
	
	if( (ctrl>>9)&1 )
	{
		//dma_regs[creg] |= 0x8000;
		if( ((ctrl>>5)&3) == 3 ) 
		{
			//dma_regs[creg-3] = dst_addr;
			//dma_regs[creg-2] = dst_addr>>16;
		}
	}
	
	if( ctrl & (1u<<14) )
	{
		IF |= (1u<<(which+8));
	}
	return;
}*/

u16 dma_read16(u32 addr)
{
	if( addr == 0x40000BA )
	{
		return dma_regs[5];
	} else if( addr == 0x40000C6 ) {
		return dma_regs[11];
	} else if( addr == 0x40000D2 ) {
		return dma_regs[17];
	} else if( addr == 0x40000DE ) {
		return dma_regs[23];
	}

	return 0;
}



