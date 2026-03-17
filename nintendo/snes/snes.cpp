#include <print>
#include "util.h"
#include "snes.h"

void snes::write(u32 a, u8 v)
{
	u8 bank = a>>16;
	a &= 0xffff;
	if( bank == 0x7E || bank == 0x7F ) { ram[((bank&1)<<16)|a] = v; return; }
	
	std::println("snes wr ${:X}:${:X} = ${:X}", bank, a, v);
}

u8 snes::read(u32 a)
{
	u8 bank = a>>16;
	a &= 0xffff;

	if( bank == 0x7E || bank == 0x7F )
	{
		return ram[((bank&1)<<16)|a];
	}
	
	if( a >= 0x8000 )
	{
		std::println("snes rd ${:X}:${:X}", bank, a);
		return ROM[bank*1024*32 + (a&0x7fff)];
	}
	
	std::println("snes rd ${:X}:${:X}", bank, a);
	return 0;
}

void snes::run_frame()
{
	ppu.frame_complete = false;
	while( !ppu.frame_complete )
	{
		s64 master_cycles = cpu_run();
		ppu_mc(master_cycles);	
	}
}

bool snes::loadROM(std::string fname)
{
	if( !freadall(ROM, fopen(fname.c_str(), "rb")) )
	{
		std::println("Unable to open snes rom '{}'", fname);
		return false;
	}
	cpu.bus_read = [&](u32 a)->u8 { return read(a); };
	cpu.bus_write = [&](u32 a, u8 v) { write(a,v); };

	//find rom header to get board info: rom/ram sizes, coprocessor, mapping type (loRom, hiRom, exhiRom)
	u32 hd = 0x7fc0;
	if( (*(u16*)&ROM[0x7fde]^0xffff) == *(u16*)&ROM[0x7fdc] || (ROM[0x7fdc]=='C'&&ROM[0x7fdd]=='C'&&ROM[0x7fde]=='C'&&ROM[0x7fdf]=='S' ) )
	{
		std::println("SNES: Using LoROM Mapping");
		cart.mapping_type = MAPPING_LOROM;
		if( (ROM[hd+0x15]&15) != 0 )
		{
			std::println("warning: detected LoROM header, but {} != {}", ROM[hd+0x15]&15, 0);
		}
	} else if( (*(u16*)&ROM[0xffde]^0xffff) == *(u16*)&ROM[0xffdc] ) {
		std::println("SNES: Using HiROM Mapping");
		cart.mapping_type = MAPPING_HIROM;
		hd = 0xffc0;
		if( (ROM[hd+0x15]&15) != 1 )
		{
			std::println("warning: detected HiROM header, but {} != {}", ROM[hd+0x15]&15, 1);
		}
	} else {
		std::println("SNES: Failing into ExHiROM");
		cart.mapping_type = MAPPING_EXHIROM;
		hd = 0x40ffc0;
		if( (ROM[hd+0x15]&15) != 5 )
		{
			std::println("warning: detected ExHiROM header, but {} != {}", ROM[hd+0x15]&15, 5);
		}
	}
	
	cart.chipset = ROM[hd+0x16];
	cart.fastROM = (ROM[hd+0x15]>>4)&1;
	cart.rom_size = (ROM[hd+0x17]*32_KB);
	cart.ram_size = (1<<ROM[hd+0x18])*1024;
	SRAM.resize(cart.ram_size);
		
	return true;
}


