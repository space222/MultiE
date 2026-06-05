#include <print>
#include <functional>
#include "gc_dsp_cpu.h"
#define instr_nop [](GCDsp&, u16) {}

struct dsp_instr
{
	u16 mask, result;
	std::function<void(GCDsp&, u16)> fnc;
};

static dsp_instr dspops[] = {
	{0xffff, 0, instr_nop}




};


void GCDsp::step()
{
	u16 opc = fetch(pc);
	pc += 2;
	
	for(auto& I : dspops)
	{
		if( (opc & I.mask) == I.result ) { I.fnc(*this, opc); return; }
	}
	
	std::println("dsp:${:X}: undef opcode ${:X}", pc-2, opc);
	exit(1);
}

u16 GCDsp::fetch(u16 addr)
{
	if( addr < 0x8000 ) return __builtin_bswap16(((u16*)aram)[addr&0xfff]);
	return __builtin_bswap16( irom[addr&0xfff] );
}

u16 GCDsp::read(u16 addr)
{
	if( addr < 0x1000 ) return __builtin_bswap16(dram[addr]);
	if( addr < 0x1800 ) return __builtin_bswap16(coef[addr&0x7ff]);
	if( addr >= 0xff00 ) return readio(addr);
	return 0;
}

void GCDsp::write(u16 addr, u16 v)
{
	if( addr < 0x1000 ) { dram[addr] = __builtin_bswap16(v); return; }
	if( addr >= 0xff00 ) { readio(addr); return; }
	return;
}

u16 GCDsp::readio(u16)
{
	return 0;
}

void GCDsp::reset()
{
	pc = 0x8000;
}

