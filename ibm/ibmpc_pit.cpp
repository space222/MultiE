#include "ibmpc.h"

void ibmpc::pit_write(u8 port, u8 val)
{
	if( port == 0x43 )
	{
		u8 chan = val>>6;
		u8 access = (val>>4)&3;
		printf("pit ctrl, chan = $%X, access = $%X, mode = $%X\n", chan, access, (val>>1)&7);
		if( access == 0 )
		{
			pitchan[chan].latch = true;
			return;
		}
		pitchan[chan].ctrl = val;
		pitchan[chan].reset = 1;
		pitchan[chan].hilo_r = pitchan[chan].hilo_w = 0;
		return;
	}
	
	port &= 3;
	
	switch( (pitchan[port].ctrl>>4) & 3 )
	{
	case 1: 
		pitchan[port].initial &= 0xff00; 
		pitchan[port].initial |= val;
		pitchan[port].CE = (pitchan[port].initial == 0) ? 65536 : pitchan[port].initial;
		pitchan[port].reset = 0;
		break;
	case 2: 
		pitchan[port].initial &= 0xff; 
		pitchan[port].initial |= val<<8;
		pitchan[port].CE = (pitchan[port].initial == 0) ? 65536 : pitchan[port].initial;
		pitchan[port].reset = 0;
		break;
	case 3:
		if( pitchan[port].hilo_w == 0 ) 
		{
			pitchan[port].initial = (pitchan[port].initial&0xff00)|val;
			pitchan[port].hilo_w = 1;
		} else {
			pitchan[port].initial = (pitchan[port].initial&0xff)|(val<<8);
			pitchan[port].hilo_w = 0;
			pitchan[port].CE = (pitchan[port].initial == 0) ? 65536 : pitchan[port].initial;
			pitchan[port].reset = 0;
			printf("port 0x4%X CE = $%X\n", port, pitchan[port].CE);
		}		
		break;
	}
	printf("pit write $%X = $%X\n", port, val);
}

u8 c = 0xf0;

u8 ibmpc::pit_read(u8 port)
{
	printf("pit read $%X\n", port);
	port &= 3;
	if( port == 3 ) return 0;
	u8 mode = (pitchan[port].ctrl>>4)&3;
	switch( mode )
	{
	case 1:
		pitchan[port].latch = false;
		return pitchan[port].OL;
	case 2:
		pitchan[port].latch = false;
		return pitchan[port].OL>>8;
	case 3:
		if( pitchan[port].hilo_r == 0 )
		{
			pitchan[port].hilo_r = 1;
			return pitchan[port].OL;
		} else {
			pitchan[port].latch = false;
			pitchan[port].hilo_r = 0;
			return pitchan[port].OL>>8;
		}
	}
	return 0xca;
}

void ibmpc::pitcycle()
{
	if( pitdiv != 0 )
	{
		pitdiv--;
		return;
	}
	pitdiv = 8;
	
	for(u32 C = 0; C < 3; ++C)
	{
		if( pitchan[C].gate == 0 || pitchan[C].reset ) continue;
	
		pitchan[C].CE--;
	
		if( ! pitchan[C].latch ) pitchan[C].OL = pitchan[C].CE;
	}
}


