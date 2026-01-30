#include <print>
#include <string>
#include "ps2.h"

void ps2::gs_run_fifo()
{
	while( true )
	{
		if( gs.fifo.empty() ) return;

		u128 GIFtag = gs.fifo.back();
		std::println("GIFtag = ${:X}", GIFtag);
		u32 format = (GIFtag>>58)&3;
		if( format == 3 ) format = 2;
		
		u32 NLOOP = GIFtag&0x7fff;
		u32 NREGS = (GIFtag>>60)&0xf;
		if( NREGS == 0 ) NREGS = 16;
		u64 regs = GIFtag>>64;
		
		std::println("NLOOP*NREGS = {} qwords, fifo sz = {} qwords", NLOOP*NREGS, gs.fifo.size());
		if( NLOOP == 0 )
		{
			gs.fifo.pop_back();
			continue;
		}		
		
		switch( format )
		{
		case 0: // PACKED
			if( gs.fifo.size()-1 < NLOOP*NREGS ) return;
			gs.fifo.pop_back();
			std::println("Would have PACKED");
			std::println("NLOOP*NREGS = {} qwords, fifo sz = {} qwords", NLOOP*NREGS, gs.fifo.size());
			//if( NLOOP*NREGS > 1000 ) exit(1);
			for( u32 i = 0; i < NLOOP*NREGS; ++i )
			{
				std::println("FIFO ${:X}", gs.fifo.back());
				gs.fifo.pop_back();
			}
			break;
		case 1:{ // REGLIST
			u32 rlsize = NREGS*NLOOP;
			if( rlsize & 1 ) rlsize += 1;
			rlsize >>= 1;
			if( gs.fifo.size()-1 < rlsize ) return;
			gs.fifo.pop_back();
			std::println("Would have REGLIST");
			std::println("NLOOP*NREGS = {} qwords, fifo sz = {} qwords", rlsize, gs.fifo.size());
			exit(1);
			}break;
		case 2: // IMAGE
			if( gs.fifo.size()-1 < NLOOP ) return;
			gs.fifo.pop_back();
			std::println("Would have IMAGE");
			std::println("NLOOP = {} qwords, fifo sz = {} qwords", NLOOP, gs.fifo.size());
			exit(1);
			break;
		}
	}

	gs.fifo.clear(); // temporary
	return;
}

