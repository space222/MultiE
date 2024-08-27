#include "ibmpc.h"

static u8 stat = 0;
static u8 temp_com1_ie = 0;
static u8 dor = 4;
static u8 was_reset = 0;

void ibmpc::fdc_write(u16 port, u8 val)
{
	printf("FDC port $%X = $%X\n", port, val);
	if( port == 0x3F9 )
	{
		temp_com1_ie = val;
		return;
	}
	if( port == 0x3F2 )
	{
		u8 old = dor;
		dor = val;
		if( (dor&4) && !(old&4) )
		{
			cpu.pic.irr |= (1<<6);
			was_reset = 1;
			stat &= ~0x10;
			fdc_cmd_fifo.clear();
			fdc_res_fifo.clear();
		}
		return;
	}
	if( port == 0x3F5 )
	{
		fdc_cmd_fifo.push_back(val);
		if( fdc_cmd_fifo.size() == 1 )
		{
			stat |= 0x10;
			u8 cmd = fdc_cmd_fifo[0] & 0xf;
			if( cmd != 7 && cmd != 3 && cmd != 8 )
			{
				printf("Unknown FDC Cmd = $%X\n", cmd);
				//exit(1);
			}
		}

		if( fdc_cmd_fifo[0] == 7 && fdc_cmd_fifo.size() == 2 )
		{
			cpu.pic.irr |= (1<<6);
			fdc_cmd_fifo.clear();
			stat &= ~0x10;
			return;
		}
		
		if( fdc_cmd_fifo[0] == 3 && fdc_cmd_fifo.size() == 3 )
		{
			fdc_cmd_fifo.clear();
			stat &= ~0x10;
			return;
		}
		
		if( fdc_cmd_fifo[0] == 8 && fdc_cmd_fifo.size() == 1 )
		{
			fdc_cmd_fifo.clear();
			fdc_res_fifo.push_front(was_reset ? 0xc0 : 0x20); 
			was_reset = 0;
			fdc_res_fifo.push_front(0x00);
			return;
		}
		return;
	}
}

u8 ibmpc::fdc_read(u16 port)
{
	printf("FDC IN $%X\n", port);
	if( port == 0x3F9 ) return temp_com1_ie;
	if( port == 0x3F4 )
	{
		u8 r = stat | 0x80 | (fdc_res_fifo.empty()?0:0x40);
		return r;
	}
	
	if( port == 0x3F5 )
	{
		u8 r = 0;
		if( !fdc_res_fifo.empty() )
		{
			r = fdc_res_fifo.back();
			fdc_res_fifo.pop_back();
		}
		if( fdc_res_fifo.empty() ) stat &= ~0x10;
		printf("FDC IN Port $%X = $%X\n", port, r);
		return r;
	}
	return 0;
}

