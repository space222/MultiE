#include "ibmpc.h"

void ibmpc::pic_write(u16 port, u8 val)
{
	if( port == 0x20 )
	{
		if( val & 0x20 )
		{
			if( cpu.pic.isr ) cpu.pic.isr &= ~(1<<std::countr_zero(cpu.pic.isr));
			printf("PIC: EOI!\n");
		}
		return;
	}
	if( port == 0x21 )
	{
		cpu.pic.imr = val;
		return;
	}
}

u8 ibmpc::pic_read(u16 port)
{
	if( port == 0x21 ) 
	{
		return cpu.pic.imr;
	}
	printf("pic read $%X\n", port);
	return 0;
}

