#include <print>
#include "trs80cc3.h"

u8 trs80cc3::disk_io_read(u8 a)
{
	switch( a )
	{
	case 0x48:
		if( io[0x48] == 0x80 ) return 2;
		return 0;
		
	case 0x4B:{
		u8 r = floppyA[track*(18*256) + ((sector-1)*256) + secoffs];
		secoffs += 1;
		if( secoffs >= 256 )
		{
			io[0x40] = 0;
			cpu.nmi_line = true;
			secoffs = 0;
			sector += 1;
			if( sector >= 18 )
			{
				sector = 1;
				track += 1;
				io[0x49] = track;
			}
			io[0x4A] = sector;
		}
		return r; 
	}
	}
	return 0;
}

void trs80cc3::disk_io_write(u8 a, u8 v)
{
	switch( a )
	{
	case 0x48:
		io[a] = v;
		if( v == 0x17 )
		{
			io[0x49] = track = io[0x4B];
			secoffs = 0;
		}
		return;
	case 0x4A: /*std::println("sector set to {}", v);*/ sector = io[a] = v; return;
	}
	io[a] = v;
}

