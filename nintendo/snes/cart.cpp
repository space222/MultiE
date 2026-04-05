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

u8 snes::hirom_romsel_read(u32 addr)
{
	return ROM[(addr&((1u<<22)-1)) % ROM.size()];
}
void snes::hirom_romsel_write(u32, u8) {}

u8 snes::hirom_cart_io_read(u32 addr)
{
	u32 bank = addr >> 16;
	u32 a = addr & 0xffff;
	if( cart.ram_size && addr>=0x6000 && addr<0x8000 && bank>=0x30 && bank<0x40 )
	{
		return SRAM[((bank-0x30)*8_KB + (a&0x1fff)) % cart.ram_size];
	}
	return 0;
}

void snes::hirom_cart_io_write(u32 addr, u8 v) 
{
	u32 bank = addr >> 16;
	u32 a = addr & 0xffff;
	if( cart.ram_size && addr>=0x6000 && addr<0x8000 && bank>=0x30 && bank<0x40 )
	{
		SRAM[((bank-0x30)*8_KB + (a&0x1fff)) % cart.ram_size] = v;
		return;
	}
	return;
}

u8 snes::superfx_romsel_read(u32 addr)
{
	u32 bank = addr>>16;
	u32 a = addr&0xffff;
	if( (bank&0x7F)<0x40 && (a&0x8000) ) return ROM[((bank*32_KB)+(a&0x7fff))%ROM.size()];
	if( (bank&0x7F)>=0x40 && (bank&0x7F)<0x60 ) return ROM[((bank*64_KB)+a) % ROM.size()];
	if( bank==0x70 || bank==0x71 ) return extram[(((bank&1)*64_KB)+a) % extram.size()];
	if( cart.ram_size && (bank==0x78||bank==0x79) ) return SRAM[(((bank&1)*64_KB)+a) % cart.ram_size];
	
	std::println("superfx unimpl romsel read ${:X}:${:X}", bank, a);
	return 0;
}

void snes::superfx_romsel_write(u32 addr, u8 v)
{
	u32 bank = addr>>16;
	u32 a = addr&0xffff;
	if( cart.ram_size && (bank==0x78||bank==0x79) ) 
	{
		SRAM[(((bank&1)*64_KB)+a) % cart.ram_size] = v;
		return;
	}
	if( bank==0x70 || bank==0x71 ) 
	{ 
		//std::println("snes: ext wr ${:X}(${:X}%) = ${:X}", (((bank&1)*64_KB)+a), (((bank&1)*64_KB)+a)%extram.size(), v);
		extram[(((bank&1)*64_KB)+a) % extram.size()] = v;
		return;
	}
	std::println("superfx unimpl romsel wr ${:X}:${:X} = ${:X}", bank, a, v);
	
}

u8 snes::superfx_cart_io_read(u32 addr)
{
	u32 bank = addr>>16;
	u32 a = addr&0xffff;
	
	switch( a )
	{
	case 0x3000: return gsu.r[0];
	case 0x3001: return gsu.r[0]>>8;
	case 0x3002: return gsu.r[1];
	case 0x3003: return gsu.r[1]>>8;
	case 0x3004: return gsu.r[2];
	case 0x3005: return gsu.r[2]>>8;
	case 0x3006: return gsu.r[3];
	case 0x3007: return gsu.r[3]>>8;
	case 0x3008: return gsu.r[4];
	case 0x3009: return gsu.r[4]>>8;
	case 0x300A: return gsu.r[5];
	case 0x300B: return gsu.r[5]>>8;
	case 0x300C: return gsu.r[6];
	case 0x300D: return gsu.r[6]>>8;
	case 0x300E: return gsu.r[7];
	case 0x300F: return gsu.r[7]>>8;
	case 0x3010: return gsu.r[8];
	case 0x3011: return gsu.r[8]>>8;
	case 0x3012: return gsu.r[9];
	case 0x3013: return gsu.r[9]>>8;
	case 0x3014: return gsu.r[10];
	case 0x3015: return gsu.r[10]>>8;
	case 0x3016: return gsu.r[11];
	case 0x3017: return gsu.r[11]>>8;
	case 0x3018: return gsu.r[12];
	case 0x3019: return gsu.r[12]>>8;
	case 0x301A: return gsu.r[13];
	case 0x301B: return gsu.r[13]>>8;
	case 0x301C: return gsu.r[14];
	case 0x301D: return gsu.r[14]>>8;
	case 0x301E: return gsu.r[15];
	case 0x301F: return gsu.r[15]>>8;



	case 0x3030: return gsu.F.v;
	case 0x3031: { u8 t = gsu.F.v>>8; gsu.F.b.IRQ=0; cpu.irq_line = false; return t; }

	case 0x3034: return gsu.pb;

	case 0x3036: return gsu.romb;

	case 0x303C: return gsu.ramb&1;
	}
	
	std::println("${:X}:${:X}: io rd ${:X}:${:X}", cpu.pb>>16, cpu.pc, bank, a);
	return 0;
}

void snes::superfx_cart_io_write(u32 addr, u8 v)
{
	u32 bank = addr>>16;
	u32 a = addr&0xffff;

	//std::println("${:X}:${:X}: io wr ${:X}:${:X} = ${:X}", cpu.pb>>16, cpu.pc, bank, a, v);

	switch( a )
	{
	case 0x3000: gsu.r[0] &= 0xff00; gsu.r[0] |= v; return;
	case 0x3001: gsu.r[0] &= 0xff; gsu.r[0] |= v<<8; return;
	case 0x3002: gsu.r[1] &= 0xff00; gsu.r[1] |= v; return;
	case 0x3003: gsu.r[1] &= 0xff; gsu.r[1] |= v<<8; return;
	case 0x3004: gsu.r[2] &= 0xff00; gsu.r[2] |= v; return;
	case 0x3005: gsu.r[2] &= 0xff; gsu.r[2] |= v<<8; return;
	case 0x3006: gsu.r[3] &= 0xff00; gsu.r[3] |= v; return;
	case 0x3007: gsu.r[3] &= 0xff; gsu.r[3] |= v<<8; return;
	case 0x3008: gsu.r[4] &= 0xff00; gsu.r[4] |= v; return;
	case 0x3009: gsu.r[4] &= 0xff; gsu.r[4] |= v<<8; return;
	case 0x300A: gsu.r[5] &= 0xff00; gsu.r[5] |= v; return;
	case 0x300B: gsu.r[5] &= 0xff; gsu.r[5] |= v<<8; return;
	case 0x300C: gsu.r[6] &= 0xff00; gsu.r[6] |= v; return;
	case 0x300D: gsu.r[6] &= 0xff; gsu.r[6] |= v<<8; return;
	case 0x300E: gsu.r[7] &= 0xff00; gsu.r[7] |= v; return;
	case 0x300F: gsu.r[7] &= 0xff; gsu.r[7] |= v<<8; return;
	case 0x3010: gsu.r[8] &= 0xff00; gsu.r[8] |= v; return;
	case 0x3011: gsu.r[8] &= 0xff; gsu.r[8] |= v<<8; return;
	case 0x3012: gsu.r[9] &= 0xff00; gsu.r[9] |= v; return;
	case 0x3013: gsu.r[9] &= 0xff; gsu.r[9] |= v<<8; return;
	case 0x3014: gsu.r[10] &= 0xff00; gsu.r[10] |= v; return;
	case 0x3015: gsu.r[10] &= 0xff; gsu.r[10] |= v<<8; return;
	case 0x3016: gsu.r[11] &= 0xff00; gsu.r[11] |= v; return;
	case 0x3017: gsu.r[11] &= 0xff; gsu.r[11] |= v<<8; return;
	case 0x3018: gsu.r[12] &= 0xff00; gsu.r[12] |= v; return;
	case 0x3019: gsu.r[12] &= 0xff; gsu.r[12] |= v<<8; return;
	case 0x301A: gsu.r[13] &= 0xff00; gsu.r[13] |= v; return;
	case 0x301B: gsu.r[13] &= 0xff; gsu.r[13] |= v<<8; return;
	case 0x301C: gsu.r[14] &= 0xff00; gsu.r[14] |= v; return;
	case 0x301D: gsu.r[14] &= 0xff; gsu.r[14] |= v<<8; return;
	case 0x301E: gsu.r[15] &= 0xff00; gsu.r[15] |= v; return;
	case 0x301F: gsu.r[15] &= 0xff; gsu.r[15] |= v<<8; gsu.fetch = gsu_read(gsu.pb, gsu.r[15]); gsu.r[15]+=1; gsu.F.b.GO = 1; return;

	case 0x3034: gsu.pb = v; return;
	case 0x3036: gsu.romb = v; return;
	case 0x3038: gsu.scb = v; return;
	
	case 0x303A: gsu.scm = v; return;
	
	case 0x303C: gsu.ramb = (v&1)|0x70; return;
	}

	std::println("${:X}:${:X}: io wr ${:X}:${:X} = ${:X}", cpu.pb>>16, cpu.pc, bank, a, v);
}
















