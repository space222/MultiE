#include <print>
#include "GameCube.h"

#define EXI2DATA_ADDR 0xcc006838
#define EXI2CSR_ADDR  0xcc006828
#define EXI2CR_ADDR   0xcc006834

#define EXI0DATA_ADDR 0xCC006810
#define EXI0CSR_ADDR  0xcc006800
#define EXI0CR_ADDR   0xcc00680C
#define EXI0MAR_ADDR  0xcc006804
#define EXI0LENGTH_ADDR 0xcc006808

#define EXI1CSR_ADDR 0xcc006814
#define EXI1MAR_ADDR 0xcc006818
#define EXI1LENGTH_ADDR 0xcc00681c
#define EXI1CR_ADDR 0xcc006820
#define EXI1DATA_ADDR 0xcc006824


void GameCube::exi_write(u32 addr, u32 v, int size)
{
	switch(addr)
	{
	case EXI2DATA_ADDR: szchk(32); exi.EXI2DATA = v; /*std::println("${:X}: EXI2DATA = ${:X}", cpu.pc-4, exi.EXI2DATA);*/ return;
	case EXI2CSR_ADDR: szchk(32); exi.EXI2CSR = v; return;
	case EXI2CR_ADDR: szchk(32); {
			exi.EXI2CR = v;
			if( v & 1 )
			{ // start EXI xfer, AD16 is the only channel2 device and doesn't do much. so cheap hack time
				//std::println("AD16 exi2cr write with imm = ${:X}", exi.EXI2DATA);
				if( exi.EXI2DATA == 0 ) 
				{
					exi.EXI2DATA = 0x04120000u;
				} else if( exi.EXI2DATA == 0xA0000000u ) {
					; 
				} else {
					std::println("AD16 written with ${:X}", exi.EXI2DATA);
				}
			}
	 	} return;

	case EXI0DATA_ADDR: szchk(32); exi.EXI0DATA = v; return;
	case EXI0CSR_ADDR: szchk(32); exi.EXI0CSR = v; return;
	case EXI0MAR_ADDR: szchk(32); exi.EXI0MAR = v; return;
	case EXI0LENGTH_ADDR: szchk(32); exi.EXI0LENGTH = v; return;
	case EXI0CR_ADDR: szchk(32); {
			exi.EXI0CR = v;
			if( (v & 3)==1 )
			{
				exi.offset = exi.EXI0DATA>>6;
				return;
			}
			if( (v & 3)==3 )
			{ // start EXI xfer
				if( exi.EXI0LENGTH == 0 ) return;
				//std::println("data = ${:X}, addr = ${:X}, len = ${:X}", exi.offset, exi.EXI0MAR, exi.EXI0LENGTH);
				if( exi.offset + exi.EXI0LENGTH >= 2*1024*1024 || exi.EXI0MAR + exi.EXI0LENGTH >= 24*1024*1024 ) return;
				for(u32 i = 0; i < exi.EXI0LENGTH; ++i) mem1[exi.EXI0MAR+i] = ipl_clear[exi.offset+i];
				return;
			}
	 	} return;
	 	
	case EXI1DATA_ADDR: szchk(32); exi.EXI1DATA = v; return;
	case EXI1CSR_ADDR: szchk(32); exi.EXI1CSR = v; return;
	case EXI1MAR_ADDR: szchk(32); exi.EXI1MAR = v; return;
	case EXI1LENGTH_ADDR: szchk(32); exi.EXI1LENGTH = v; return;
	case EXI1CR_ADDR: szchk(32); {
			exi.EXI1CR = v;
			
	 	} return;	
	default:
		std::println("${:X}: exi{} ${:X} = ${:X}", cpu.pc-4, size, addr, v);
		exit(1);
	}
}

u32 GameCube::exi_read(u32 addr, int size)
{
	switch(addr)
	{
	case EXI2DATA_ADDR: szchk(32); return exi.EXI2DATA;
	case EXI2CSR_ADDR: szchk(32); return exi.EXI2CSR;
	case EXI2CR_ADDR: szchk(32); { u32 t = exi.EXI2CR; exi.EXI2CR&=~1; return t; }
	
	case EXI0DATA_ADDR: szchk(32); return exi.EXI0DATA;
	case EXI0CSR_ADDR: szchk(32); return exi.EXI0CSR;
	case EXI0CR_ADDR: szchk(32); { u32 t = exi.EXI0CR; exi.EXI0CR&=~1; return t; }
	case EXI0MAR_ADDR: szchk(32); return exi.EXI0MAR;
	case EXI0LENGTH_ADDR: szchk(32); return exi.EXI0LENGTH;

	case EXI1DATA_ADDR: szchk(32); return exi.EXI1DATA;
	case EXI1CSR_ADDR: szchk(32); return exi.EXI1CSR;
	case EXI1CR_ADDR: szchk(32); { u32 t = exi.EXI1CR; exi.EXI1CR&=~1; return t; }
	case EXI1MAR_ADDR: szchk(32); return exi.EXI1MAR;
	case EXI1LENGTH_ADDR: szchk(32); return exi.EXI1LENGTH;
	
	default:
		std::println("${:X}: exi{} <${:X}", cpu.pc-4, size, addr);
		exit(1);
	}
	return 0;
}

