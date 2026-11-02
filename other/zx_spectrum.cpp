#include <print>
#include <string>
#include "zx_spectrum.h"

u8 zx_spectrum::read(u16 a)
{
	if( a < 0x4000 ) { return bios[a]; }
	return ram[a];
}

void zx_spectrum::write(u16 a, u8 v)
{
	ram[a] = v;
	return;
}

u8 zx_spectrum::in(u16 p)
{
	std::println("in ${:X}", p);
	
	return 0xFF;
}

void zx_spectrum::out(u16 p, u8 v)
{
	std::println("out ${:X} = ${:X}", p, v);
}

void zx_spectrum::run_frame()
{
	for(u32 line = 0; line < 312; ++line)
	{
		target = last_target + 224;
		while( stamp < target )
		{
			u64 cc = cpu.step();
			stamp += cc;
		}
		last_target = target;
		
		if( line == 192 )
		{
			cpu.irq_line = true;
		} else {
			cpu.irq_line = false;
		}
	}
	
	for(u32 y = 0; y < 192; ++y)
	{
		for(u32 x = 0; x < 256; ++x)
		{
			u32 addr_hi = ((y&0xc0)>>3) | (y&7);
			u32 addr_lo = ((y&0x38)<<2) | (x>>3);
			u8 B = ram[0x4000 + (addr_hi<<8) + addr_lo];
			B >>= (7^x)&7;
			B &= 1;
			fbuf[y*256+x] = (B ? 0xffFFff00:0);	
		}
	}
}

void zx_spectrum::reset()
{
	memset((void*)&cpu, 0, sizeof(cpu));
	cpu.read = [&](u16 a)->u8 { return read(a); };
	cpu.write= [&](u16 a, u8 v) { write(a,v); };
	cpu.in = [&](u16 p)-> u8 { return in(p); };
	cpu.out = [&](u16 p, u8 v) { out(p, v); };
	cpu.halted = false;
	stamp = target = last_target = 0;
}

bool zx_spectrum::loadROM(const std::string /*fname*/)
{
	FILE* fp = fopen("./bios/48.rom", "rb");
	[[maybe_unused]] int unu = fread(bios, 1, 16*1024, fp);
	fclose(fp);
	
	return true;
}

