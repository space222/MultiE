#include <print>
#include "snes.h"

u8 snes::lorom_romsel_read(u32 addr)
{
	u32 bank = addr>>16;
	u32 a = addr & 0xffff;
	
	if( cart.ram_size && a < 0x8000 && bank >= 0x70 && bank < 0x7E )
	{
		return SRAM[((bank-0x70)*32_KB + a) % cart.ram_size];
	}
	
	return ROM[((bank&0x7f)*32_KB + (a&0x7fff)) % ROM.size()];
}

void snes::lorom_romsel_write(u32 addr, u8 v)
{
	u32 bank = addr>>16;
	u32 a = addr & 0xffff;

	if( cart.ram_size && a < 0x8000 && bank >= 0x70 && bank < 0x7E )
	{
		SRAM[((bank-0x70)*32_KB + a) % cart.ram_size] = v;
		save_written = true;
		return;
	}
}

u8 snes::lorom_cart_io_read(u32) { return 0; }  // lorom doesn't have any extra stuff in the IO space
void snes::lorom_cart_io_write(u32, u8) {}

u8 snes::hirom_romsel_read(u32 a)
{



	return 0;
}

void snes::hirom_romsel_write(u32 a, u8 v)
{

}

u8 snes::hirom_cart_io_read(u32)
{
	return 0;
}

void snes::hirom_cart_io_write(u32, u8) {}


