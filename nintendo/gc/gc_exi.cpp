#include <print>
#include "GameCube.h"
#define szchk(a) if( size != (a) ) { std::println("addr ${:X} size check fail g{}!=a{}",addr,size,a); exit(1); }

#define EXI2DATA_ADDR 0xcc006838
#define EXI2CSR_ADDR  0xcc006828
#define EXI2CR_ADDR   0xcc006834

#define EXI0DATA_ADDR 0xCC006810
#define EXI0CSR_ADDR  0xcc006800
#define EXI0CR_ADDR   0xcc00680C
#define EXI0MAR_ADDR  0xcc006804
#define EXI0LENGTH_ADDR 0xcc006808

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
			if( v & 1 )
			{ // start EXI xfer
							
			}
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
	
	default:
		std::println("${:X}: exi{} <${:X}", cpu.pc-4, size, addr);
		exit(1);
	}
	return 0;
}

