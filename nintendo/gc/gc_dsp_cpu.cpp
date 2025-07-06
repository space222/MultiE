#include <print>
#include "gc_dsp_cpu.h"

void GCDsp::step()
{
	u16 opc = fetch(pc);
	pc += 2;






}

u16 GCDsp::fetch(u16 addr)
{
	if( addr & 0x8000u ) return irom[addr&0xfff];
	return ((u16*)&aram[0])[addr&0xfff];
}

u16 GCDsp::read(u16 addr)
{
	if( addr >= 0xff00u ) { return readio(addr&0xff); }
	if( addr & 0x8000u )  { return coef[addr&0x7ff]; }
	return dram[addr&0xfff];
}

u16 GCDsp::readio(u16 addr)
{
	switch( addr & 0xff )
	{
	
	default:
		std::println("DSP io <$ff{:X}", addr);
		exit(1);
	}
	return 0;
}

void GCDsp::reset()
{
	pc = 0x8000;
}

