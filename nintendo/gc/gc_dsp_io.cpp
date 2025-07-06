#include <print>
#include "GameCube.h"
static bool once = true;

void GameCube::dsp_io_write(u32 addr, u32 v, int size)
{
	std::println("${:X}: write{} ${:X} = ${:X}", cpu.pc-4, size, addr, v);
	switch( addr )
	{
	case 0xCC005000: dsp.io.dspboxH = v|0x8000; return;
	case 0xCC005002: dsp.io.dspboxL = v; return;
	case 0xCC005012: dsp.io.AR_SIZE = v; return;
	case 0xCC00501a: dsp.io.AR_REFRESH=v; return;
	
	case 0xCC005020: dsp.io.ramaddr = v; return;
	case 0xCC005024: dsp.io.aramaddr = v; return;
	case 0xCC005028: {
			dsp.io.aramdmactrl = v;
			if( v & BIT(31) )
			{
				std::println("Attempt to DMA from ARAM to MEM1");
				exit(1);
			} else {
				for(u32 i = 0; i < (v&~BIT(31)); ++i)
				{
					dsp.aram[dsp.io.aramaddr+i] = mem1[(dsp.io.ramaddr+i)];
					if( once ) std::println("${:X}", mem1[(dsp.io.ramaddr+i)]);
				}
				once = false;
				dsp.io.aramaddr += v&~BIT(31);
				dsp.io.ramaddr  += v&~BIT(31);
				dsp.io.aramdmactrl &= ~1;
			}
		} return;
	
	case 0xCC00500A: dsp.io.csr = v; std::println("${:X}: 500A = ${:X}", cpu.pc-4, v); return;
	
	case 0xCC005036: return; //todo
	default:
		exit(1);
	}


}

u32 GameCube::dsp_io_read(u32 addr, int size)
{
	if( addr != 0xcc005004 ) std::println("${:X}: read{} <${:X}", cpu.pc-4, size, addr);
		
	switch( addr )
	{
	case 0xCC005000: return dsp.io.dspboxH;
	case 0xCC005002: return dsp.io.dspboxL;
	case 0xCC005004: return dsp.io.cpuboxH;
	case 0xCC005006: return dsp.io.cpuboxL;

	case 0xCC005012: return dsp.io.AR_SIZE;
	case 0xCC00501a: return dsp.io.AR_REFRESH;
	
	case 0xCC005020: return dsp.io.ramaddr;
	case 0xCC005024: return dsp.io.aramaddr;
	case 0xCC005028: return dsp.io.aramdmactrl;
	
	case 0xcc00500a:{ u32 t = dsp.io.csr; dsp.io.csr &= 0x7fe; return t; }
	default:
		std::println("${:X}: read{} <${:X}", cpu.pc-4, size, addr);
		//exit(1);
	}
	return 0;
}

