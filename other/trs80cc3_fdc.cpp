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
		if( io[0x48] != 0x80 ) return 0;
		u8 r = secoffs < 256 ? floppyA[track*(18*256) + ((sector-1)*256) + secoffs] : 0;
		secoffs += 1;
		if( secoffs >= 256 )
		{
			io[0x48] = 0;
			secoffs = 0;
			fdc_do_nmi = true;
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
	case 0x40:
		io[a] = v;
		if( fdc_do_nmi )
		{
			fdc_do_nmi = false;
			cpu.nmi_line = true;
		}
		return;
	case 0x48:
		io[a] = v;
		if( v == 0x17 )
		{
			io[0x49] = track = io[0x4B];
			secoffs = 0;
		} else if( v == 0x80 ) {
			secoffs = 0;
		} else if( v == 0xD0 ) {
			secoffs = 0;
		} else {
			std::println("Floppy unsupported cmd = ${:X}", v);
		}
		return;
	case 0x4A: /*std::println("sector set to {}", v);*/ sector = io[a] = v; secoffs = 0; return;
	}
	io[a] = v;
}

