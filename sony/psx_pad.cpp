#include <cstdio>
#include <cstdlib>
#include <SDL.h>
#include "psx.h"

u8 cont1LO();
u8 cont1HI();

void psx::pad_event(u64 oldstamp, u32 code)
{
	if( code == 0x81 )
	{ // ack time complete
		pad_stat &= ~(1u<<7);
		I_STAT |= 0x80;
		pad_stat |= (1u<<9);
		return;
	}

	if( code == 0x80 )
	{  // transfer complete
		u8 data = pad_tx_fifo;
		u32 sn = (pad_ctrl>>13)&1;
		if( sn == 1 )
		{
			//printf("PAD: use of pad1\n");
			pad_rx_fifo = 0xff;
			pad_rfifo_pos = 1;
			pad_stat &= ~(1u<<7);
			pad_stat |= 5;
			slot[sn].pos = 0;
			return;
		}
		u8 R = 0xff;
		//printf("got here, sn = %i, pos = %i\n", sn, slot[sn].pos);
		if( slot[sn].pos == 0 )
		{
			slot[sn].device = data;
			slot[sn].pos = 1;
		} else {
			if( slot[0].device == 0x81 )
			{
				//printf("PAD: < $%X\n", data);
				if( slot[sn].pos < 4 )
				{
					switch( slot[sn].pos )
					{
					case 1: R = slot[sn].flag; slot[sn].cmd = data; slot[sn].pos += 1;
						/*printf("cmd = $%X\n", data);*/ 
						if( 0 ) // data != 'R' && data != 'W' )
						{
							printf("CARD: Unsupported cmd = $%X\n", slot[sn].cmd);
							slot[sn].pos = 0;
							R = 0xff;
						}
						break;
					case 2: R = 0x5A; slot[sn].pos += 1; break;
					case 3: R = 0x5D; slot[sn].pos += 1; break;
					}
				} else {
					if( slot[sn].cmd == 'R' ) 
					{
						R = memcard_read(sn, data);
					} else if( slot[sn].cmd == 'W' ) {
						R = memcard_write(sn, data);
					}		
				}			
			/*	// unused code for no-card-inserted
				pad_rx_fifo = 0xff;
				pad_rfifo_pos = 1;
				pad_stat &= ~(1u<<7);
				pad_stat |= 5;
				slot[sn].pos = 0;
				return;
			*/
			} else {
				switch( slot[sn].pos )
				{
				case 1: R = 0x41;
					slot[sn].pos+=1;
					if( data != 0x42 ) slot[sn].pos = 0;
					break;
				case 2: R = 0x5A; slot[sn].pos+=1; break;
				case 3: R = sn ? 0xff : cont1LO(); slot[sn].pos+=1; break;
				case 4: R = sn ? 0xff : cont1HI(); slot[sn].pos = 0;  break;
				}
				if( slot[sn].device == 0x81 ) R = 0xff;
			}
		}
		
		pad_rx_fifo = R;
		pad_rfifo_pos = 1;
		//printf("PAD: xfer complete, rfifo = $%lX\n", pad_rx_fifo);
		
		pad_stat &= ~(1u<<7);
		pad_stat |= (slot[sn].pos!=0) ? (1u<<7) : 0;
		
		if( slot[sn].pos != 0 ) sched.add_event(oldstamp+338, 0x81);		
		return;
	}

}

void psx::pad_write(u32 addr, u32 val)
{
	if( addr == 0x1F801040 )
	{
		pad_tx_fifo = 0x100 | (val&0xff);
		pad_stat |= 1;
		pad_stat &= ~4;
		sched.add_event(stamp+670, 0x80);
		return;
	}

	if( addr == 0x1F80104A )
	{
		pad_ctrl = val;
		if( !(pad_ctrl&2) )
		{
			slot[0].pos = slot[1].pos = 0;
		}
		if( pad_ctrl & 0x40 )
		{
			pad_rx_fifo = 0;
			pad_rfifo_pos = 0;
			pad_stat = 5;
			pad_ctrl = 0;
			slot[0].pos = slot[1].pos = 0;
		}
		if( pad_ctrl & 0x10 )
		{
			pad_stat &= ~(1u<<9);
		}
		return;
	}
	
	if( addr == 0x1F80104E ) { pad_baud = val; return; }
	if( addr == 0x1F801048 ) { pad_mode = val&0x1ff; return; }
}

u32 psx::pad_read(u32 addr)
{
	if( addr == 0x1F801040 )
	{
		u32 v = pad_rx_fifo;
		//printf("PAD: rx fifo = $%lX\n", pad_rx_fifo);
		pad_rx_fifo = 0xff;
		pad_rfifo_pos = 0;
		return v&0xff;
	}

	if( addr == 0x1F80104A ) return pad_ctrl;
	if( addr == 0x1F80104E ) return pad_baud;
	if( addr == 0x1F801048 ) return pad_mode;
	if( addr == 0x1F801044 ) return (5 | pad_stat | (pad_rfifo_pos?2:0)) | (pad_baud<<11);
	if( addr == 0x1F801054 ) return 7; // SIO1 not implemented, hopefully nothing spins on this
	printf("PAD: R$%X\n", addr);
	return 0;
}

u8 psx::memcard_read(u32 sn, u8 val)
{
	if( slot[sn].pos == 4 )
	{
		slot[sn].msb = slot[sn].sum = val;
		++slot[sn].pos;
		return 0;
	}
	if( slot[sn].pos == 5 )
	{
		slot[sn].lsb = val;
		slot[sn].sum ^= val;
		slot[sn].cardaddr = ((slot[sn].msb<<8)|slot[sn].lsb)*128;
		++slot[sn].pos;
		return slot[sn].msb;
	}
	if( slot[sn].pos == 6 ) { ++slot[sn].pos; return 0x5c; }
	if( slot[sn].pos == 7 ) { ++slot[sn].pos; return 0x5d; }
	if( slot[sn].pos == 8 ) { ++slot[sn].pos; return slot[sn].msb; }
	if( slot[sn].pos == 9 ) { ++slot[sn].pos; return slot[sn].lsb; }
	if( slot[sn].pos < 128+10 )
	{
		u8 res = (sn ? memcard1[slot[sn].cardaddr] : memcard0[slot[sn].cardaddr]);
		slot[sn].cardaddr += 1;
		//printf("PAD: card read <$%X\n", res);
		slot[sn].sum ^= res;
		++slot[sn].pos;
		return res;	
	}
	if( slot[sn].pos == 128+10 ) { ++slot[sn].pos; return slot[sn].sum; }
	slot[sn].pos = 0;
	return 0x47;
}

u8 psx::memcard_write(u32 sn, u8 val)
{
	if( slot[sn].pos == 4 )
	{
		slot[sn].msb = slot[sn].sum = val;
		++slot[sn].pos;
		return 0;
	}
	if( slot[sn].pos == 5 )
	{
		slot[sn].lsb = val;
		slot[sn].cardaddr = ((slot[sn].msb<<8)|slot[sn].lsb)*128;
		//printf("CARD: writing sector $%X\n", slot[sn].cardaddr);
		slot[sn].sum ^= val;
		++slot[sn].pos;
		return slot[sn].msb;
	}
	if( slot[sn].pos < 128+6 )
	{
		if( sn )
		{
			memcard1[slot[sn].cardaddr++] = val;
		} else {
			memcard0[slot[sn].cardaddr++] = val;
		}
		slot[sn].sum ^= val;
		++slot[sn].pos;
		return 0;	
	}
	if( slot[sn].pos == 128+6 ) { ++slot[sn].pos; if( slot[sn].sum != val ) { printf("Pad: bad checky.. don't care\n"); } return 0; }
	if( slot[sn].pos == 128+7 ) { ++slot[sn].pos; return 0x5c; }
	if( slot[sn].pos == 128+8 ) { ++slot[sn].pos; return 0x5d; }
	slot[sn].pos = 0;
	slot[sn].flag = 0;
	return 0x47;
}

u8 cont1LO()
{
	u8 val = 0;
	auto keys = SDL_GetKeyboardState(nullptr);
	if( keys[SDL_SCANCODE_S] ) val |= 1; // select
	if( keys[SDL_SCANCODE_RETURN] ) val |= 8; // start
	if( keys[SDL_SCANCODE_UP] ) val |= 0x10;
	if( keys[SDL_SCANCODE_RIGHT] ) val |= 0x20;
	if( keys[SDL_SCANCODE_DOWN] ) val |= 0x40;
	if( keys[SDL_SCANCODE_LEFT] ) val |= 0x80;
	return ~val;
}

u8 cont1HI()
{
	u8 val = 0;
	auto keys = SDL_GetKeyboardState(nullptr);
	if( keys[SDL_SCANCODE_R] ) val |= 1; // L2
	if( keys[SDL_SCANCODE_Y] ) val |= 2; // R2
	if( keys[SDL_SCANCODE_4] ) val |= 4; // L2
	if( keys[SDL_SCANCODE_6] ) val |= 8; // R2
	if( keys[SDL_SCANCODE_T] ) val |= 0x10; // triangle
	if( keys[SDL_SCANCODE_H] ) val |= 0x20; // circle
	if( keys[SDL_SCANCODE_G] ) val |= 0x40; // X
	if( keys[SDL_SCANCODE_F] ) val |= 0x80; // square
	return ~val;
}

