#include <print>
#include <iostream>
#include "dreamcast.h"

// implements the interal registers of the SH4 in the dreamcast. Using the SH4 elsewhere will require copying this

void dreamcast::cpureg_write(u32 addr, u64 v, u32 sz)
{
	switch( addr )
	{
	case 0xFF00001C: intern.CCR = v; return;
	case 0xFF000038: intern.QACR0 = v&0x1c; return;
	case 0xFF00003C: intern.QACR1 = v&0x1c; return;

	case 0xFFD00004: intern.IPRA = v; return;
	case 0xFFD00008: intern.IPRB = v; return;
	case 0xFFD0000C: intern.IPRC = v; return;
	
	case 0xFFE80004: intern.SCBRR2 = v; return;
	case 0xFFE8000C: std::cerr << (char)v; return;
	case 0xFFE80010: return;

	default: break;
	}
	
	if( addr >= 0xFFD80000u && addr < 0xFFD80030u )
	{
		timer_write(addr, v, sz);
		return;
	}

	std::println("cpureg{} ${:X} = ${:X}", sz, addr, v);
	//exit(1);
}

u64 dreamcast::cpureg_read(u32 addr, u32 sz)
{
	switch( addr )
	{
	case 0xFF000010: return 0; // MMUCR
	
	case 0xFF00001C: return intern.CCR;
	case 0xFF000038: return intern.QACR0;
	case 0xFF00003C: return intern.QACR1;

	case 0xFFE80004: return intern.SCBRR2;
	case 0xFFE80010: return 0x60; //SCFSR2 (value indicates ready to transmit)
	case 0xFFE80024: return 0; //SCLSR2 never line errors
	
	case 0xFFC0000C: return intern.WTCNT;
	
	case 0xFFD00004: return intern.IPRA;
	case 0xFFD00008: return intern.IPRB;
	case 0xFFD0000C: return intern.IPRC;

	case 0xFFA0003C: return intern.CHCR3;

	
	case 0xFF800030: return 0x30; // gpio a
	
	case 0xFF000084: std::println("${:X}: Undoc perf counter $FF000084",cpu.pc); return 0; // ??? not in manual
	
	default: break;
	}
	if( addr >= 0xFFD80000u && addr < 0xFFD80030u )
	{
		return timer_read(addr, sz);
	}
	std::println("cpureg{} <${:X}", sz, addr);
	exit(1);
	return 0;
}

