#include <iostream>
#include "jag.h"
extern console* sys;

void jaguar::write(u32 addr, u32 val, int size)
{
	addr &= 0xffFFff;
	if( !memcon_written )
	{
		if( addr == 0xF00000 ) 
		{
			memcon_written = true;
			MEMCON1 = val;
			return;
		}
		
		return;
	}
	
	if( MEMCON1 & 1 )
	{
		if( addr < 0x400000 )
		{
			if( size == 16 )
			{
				RAM[addr&0x1fffff] = val>>8;
				addr += 1;
			}
			RAM[addr&0x1fffff] = val;
			return;
		}
		if( addr >= 0xF03000 && addr < 0xF04000 ) 
		{
			if( size == 16 )
			{
				gram[addr&0xfff] = val>>8;
				addr += 1;
			}
			gram[addr&0xfff] = val;
			return;
		}
		printf("JAG: write%i $%X = $%X\n", size, addr, val);
		return;
	}
	
	printf("JAG: using ROMHI=0\n");
	exit(1);
}

u32 jaguar::read(u32 addr, int size)
{
	addr &= 0xffFFff;
	
	if( !memcon_written )
	{
		if( addr == 0xF00000 ) return MEMCON1;
		addr &= 0x1FFFFF;
		if( addr >= 0x120000 ) return __builtin_bswap16(*(u16*)&bootrom[(addr-0x120000)&0x1ffff]);
		if( addr < 0x100000 ) return __builtin_bswap16(*(u16*)&bootrom[addr&0x1ffff]);
		printf("JAG: nm read%i $%X\n", size, addr);
		return 0;
	}
	
	if( addr >= 0xE00000 )
	{
		if( addr >= 0xF03000 && addr < 0xF04000 ) return __builtin_bswap16(*(u16*)&gram[addr&0xfff]);
		if( addr == 0xF0223A ) return 1; // blitter stat reg
		if( addr >= 0xF00000 ) { printf("jag: IO read $%X\n", addr); return 0; }
		return __builtin_bswap16(*(u16*)&bootrom[addr&0x1ffff]);
	}
	if( addr >= 0x800000 )
	{
		addr -= 0x800000;
		if( addr < ROM.size() ) return __builtin_bswap16(*(u16*)&ROM[addr]);
		return 0;
	}
	//printf("JAG: read%i $%X\n", size, addr);
	return __builtin_bswap16(*(u16*)&RAM[addr&0x1fffff]);
}

void jaguar::run_frame()
{
	for(int i = 0; i < 50000; ++i)
	{
		cpu.step();
	}
}

bool jaguar::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	fp = fopen("./bios/jagboot.rom", "rb");
	if(!fp) return false;
	unu = fread(bootrom, 1, 0x20000, fp);
	fclose(fp);
	
	return true;
}

void jaguar::reset()
{
	memset(&cpu, 0, sizeof(cpu));
	if( !ROM.empty() )
	{
		cpu.r[15] = *(u32*)&bootrom[0];
		cpu.r[16] = cpu.r[15] = __builtin_bswap32(cpu.r[15]);
		cpu.pc = *(u32*)&bootrom[4];
		cpu.pc = __builtin_bswap32(cpu.pc);
		cpu.sr.raw = 0x2700;	
	}
	
	cpu.mem_read8 = [](u32 addr)->u8 { u16 v = dynamic_cast<jaguar*>(sys)->read(addr&~1, 16); return v >> (((addr&1)^1)*8); };
	cpu.mem_read16=cpu.read_code16=[](u32 addr)->u16{return dynamic_cast<jaguar*>(sys)->read(addr, 16); };
	cpu.mem_read32=[](u32 addr)->u32
		{ u32 v = dynamic_cast<jaguar*>(sys)->read(addr,16)<<16;return v|dynamic_cast<jaguar*>(sys)->read(addr+2, 16); };
		
	cpu.mem_write8 = [](u32 addr, u8 val) { dynamic_cast<jaguar*>(sys)->write(addr, val, 8); };
	cpu.mem_write16= [](u32 addr,u16 val) { dynamic_cast<jaguar*>(sys)->write(addr, val, 16);};
	cpu.mem_write32= [](u32 addr,u32 val) 
	{ 
		dynamic_cast<jaguar*>(sys)->write(addr, val>>16, 16);
		dynamic_cast<jaguar*>(sys)->write(addr+2, val&0xffff, 16);
	};
	
	fbuf.resize(fb_width()*fb_height());
	
	MEMCON1 = MEMCON2 = 0;
	memcon_written = false;
}

