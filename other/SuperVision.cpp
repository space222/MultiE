#include <print>
#include "SuperVision.h"

void SuperVision::write(u16 addr, u8 v)
{
	if( addr < 0x2000 )
	{
		ram[addr] = v;
		return;
	}
	if( addr < 0x4000 )
	{
		std::println("IO w${:X} = ${:X}", addr, v);
		if( addr == 0x2023 )
		{
			timer = v;
			timer_div = 0;
			if( timer == 0 )
			{
				if( !(irq_stat&1) && (sysctrl&BIT(1)) )
				{
					cpu.irq_line = true;
				}
				irq_stat |= 1;
			}
			return;
		}
		if( addr == 0x2026 )
		{
			sysctrl = v;
			bank = v>>5;
			return;
		}
		return;
	}
	if( addr < 0x6000 )
	{
		VRAM[addr&0x1fff] = v;
		return;
	}
}

u8 SuperVision::read(u16 addr)
{
	if( addr < 0x2000 )
	{
		return ram[addr];
	}
	if( addr < 0x4000 )
	{
		std::println("IO read ${:X}", addr);
		if( addr == 0x2020 )
		{	//controller
			return 0xff;
		}
		if( addr == 0x2023 )
		{
			return timer;
		}
		return 0;
	}
	if( addr < 0x6000 )
	{
		return VRAM[addr&0x1fff];
	}
	if( addr < 0x8000 )
	{
		std::println("Unknown read ${:X}", addr);
		return 0;
	}
	if( addr < 0xc000 )
	{
		return ROM[(bank*0x4000)+(addr&0x3fff)];
	}
	return ROM[(ROM.size()-0x4000) + (addr&0x3fff)];
}

void SuperVision::run_frame()
{
	for(u32 i = 0; i < 39360*2; ++i)
	{
		cycle();
		if( timer == 0 ) continue;
		timer_div += 1;
		if( timer_div >= ( (sysctrl & BIT(4)) ? 16384 : 256 ) )
		{
			timer_div = 0;
			timer -= 1;
			if( ! timer )
			{
				if( !(irq_stat&1) && (sysctrl&BIT(1)) )
				{
					cpu.irq_line = true;
				}
				irq_stat |= 1;
			}
		}
	}
	for(u32 y = 0; y < 160; ++y)
	{
		for(u32 x = 0; x < 160; ++x)
		{
			u8 v = VRAM[y*48 + (x>>2)];
			v = (v>>((x&3)*2)) & 3;
			v <<= 6;
			fbuf[y*160 + x] = (v<<24)|(v<<16)|(v<<8);
		}
	}
}

void SuperVision::reset()
{
	cpu.writer = [&](coru6502&, u16 addr, u8 v) { write(addr, v); };
	cpu.reader = [&](coru6502&, u16 addr) -> u8 { return read(addr); };
	cpu.cpu_type = CPU_65C02;
	cycle = cpu.run();
	bank = 0;
}

bool SuperVision::loadROM(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( ! fp )
	{
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

