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
	
	case 0xCC005020: if( size == 16 ) { dsp.io.ramaddr &= 0xffff; dsp.io.ramaddr |= v<<16; return; } dsp.io.ramaddr = v; return;
	case 0xCC005022: dsp.io.ramaddr &= 0xffff0000; dsp.io.ramaddr |= v&0xffff; return;
	case 0xCC005024: if( size == 16 ) { dsp.io.aramaddr &= 0xffff; dsp.io.aramaddr |= v<<16; return; } dsp.io.aramaddr = v; return;
	case 0xCC005026: dsp.io.aramaddr &= 0xffff0000; dsp.io.aramaddr |= v&0xffff; return;
	case 0xCC005028: {
			if( size == 16 ) 
			{
				dsp.io.aramdmactrl &= 0xffff;
				dsp.io.aramdmactrl |= v<<16;
				return;
			}
			dsp.io.aramdmactrl = v;
			if( v & BIT(31) )
			{
				std::println("Attempt to DMA from ARAM to MEM1");
				exit(1);
			} else {
				std::println("ARAM DMA ${:X} to ${:X}, size ${:X} bytes", dsp.io.ramaddr, dsp.io.aramaddr, v);
				v &= 0xffffff;
				dsp.io.ramaddr &= 0x0fffFFFFu;
				for(u32 i = 0; i < (v&~BIT(31)); ++i)
				{
					//dsp.aram[dsp.io.aramaddr+i] = mem1[(dsp.io.ramaddr+i)];
					if( once ) std::println("${:X}", mem1[(dsp.io.ramaddr+i)]);
				}
				once = false;
				dsp.io.aramaddr += v&~BIT(31);
				dsp.io.ramaddr  += v&~BIT(31);
				dsp.io.aramdmactrl &= ~1;
			}
		} return;
	case 0xCC00502A:
		dsp.io.aramdmactrl &= 0xffff0000u;
		dsp.io.aramdmactrl |= v;
		if( dsp.io.aramdmactrl & BIT(31) )
		{
			std::println("Attempt to DMA from ARAM to MEM1");
			exit(1);
		} else {
			std::println("ARAM16 DMA ${:X} to ${:X}, size ${:X} bytes", dsp.io.ramaddr, dsp.io.aramaddr, dsp.io.aramdmactrl);
			dsp.io.aramaddr &= 0xffffff;
			dsp.io.ramaddr &= 0x0fffFFFFu;
			for(u32 i = 0; i < dsp.io.aramdmactrl; ++i)
			{
				//dsp.aram[dsp.io.aramaddr+i] = mem1[(dsp.io.ramaddr+i)];
				if( once ) std::println("${:X}", mem1[(dsp.io.ramaddr+i)]);
			}
			once = false;
			dsp.io.aramaddr += dsp.io.aramdmactrl;
			dsp.io.ramaddr  += dsp.io.aramdmactrl;
			dsp.io.aramdmactrl &= ~1;
		}
		std::println("DMA complete");
		return;
		
	case 0xCC00500A: dsp.io.csr = v; std::println("${:X}: 500A = ${:X}", cpu.pc-4, v); return;
	
	case 0xCC005036: return; //todo
	default:
		exit(1);
	}


}

u16 fiddle = 0x8000;

u32 GameCube::dsp_io_read(u32 addr, int size)
{
	std::println("${:X}: read{} <${:X}", cpu.pc-4, size, addr);
	if( addr == 0x8135ebc0 ) std::println("yay");
	switch( addr )
	{
	case 0xCC005000: return dsp.io.dspboxH;
	case 0xCC005002: return dsp.io.dspboxL;
	case 0xCC005004: { u16 t = fiddle; fiddle ^= 0x8000; return t; }// return 0x8000; //dsp.io.cpuboxH;
	case 0xCC005006: return 0; //dsp.io.cpuboxL;
	
	case 0xCC005012: return dsp.io.AR_SIZE;
	case 0xCC00501a: return dsp.io.AR_REFRESH;
	
	case 0xCC005016: return 1; // loop at 8137094C wants to see it???
	
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

