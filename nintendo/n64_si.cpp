#include <cstring>
#include "n64.h"

void n64::pif_run()
{
	if( pifram[0x3f] & 0x10 ) { pifram[0x3f] = 0; pif_rom_enabled = false; return; }
	if( pifram[0x3f] & 0x40 ) { pifram[0x3f] = 0; return; }
	if( pifram[0x3f] & 0x20 ) { pifram[0x3f] = 0x80; return; }


	if( !(pifram[0x3f] & 1) ) return;
	//pifram[0x3f] |= 0x80;
	// from here on it's controller time
	
	/*
	fprintf(stderr, "--------\nBefore:\n");
	for(u32 i = 0; i < 64; ++i)
	{
		if( i && i % 8 == 0 ) fprintf(stderr, "\n");
		fprintf(stderr, "$%X ", pifram[i]);
	}
	fprintf(stderr, "\n");
	*/
	
	for(u32 i = 0, chan = 0; i < 64 && chan < 6; ++i)
	{
		if( pifram[i] == 0xff || pifram[i] == 0xfd ) continue;
		if( pifram[i] == 0xfe ) break;
		if( pifram[i] == 0 ) { chan+=1; continue; }
		u8 tx = pifram[i++];
		u8 rx = pifram[i++];
		u8 cmd = pifram[i];
		if( cmd == 0 || cmd == 0xff )
		{
			i += tx;
			if( chan == 0 )
			{
				pifram[i] = 5;
				pifram[i+1] = 0;
				pifram[i+2] = 2;
			} else {
				pifram[i] = 0x80;
			}
			i += rx-1;
			chan+=1;
			continue;
		}
		if( cmd == 1 )
		{
			i += tx;
			if( chan == 0 )
			{
				auto keys = SDL_GetKeyboardState(nullptr);
				pifram[i] = 0;
				pifram[i+1] = 0;
				pifram[i+2] = 0;
				pifram[i+3] = 0;
				if( keys[SDL_SCANCODE_X] ) pifram[i] ^= 0x80; // A
				if( keys[SDL_SCANCODE_Z] ) pifram[i] ^= 0x40; // B
				if( keys[SDL_SCANCODE_A] ) pifram[i] ^= 0x20; // Z
				if( keys[SDL_SCANCODE_S] ) pifram[i] ^= 0x10; // Start
				if( keys[SDL_SCANCODE_UP] ) pifram[i] ^= 8;
				if( keys[SDL_SCANCODE_DOWN] ) pifram[i] ^= 4;
				if( keys[SDL_SCANCODE_LEFT] ) pifram[i] ^= 2;
				if( keys[SDL_SCANCODE_RIGHT] ) pifram[i] ^= 1;
			} else {
				pifram[i] = 0;
				pifram[i+1] = 0;
				pifram[i+2] = 0;
				pifram[i+3] = 0;
			}
			i += rx-1;
			chan+=1;
			continue;
		}
		i += tx + rx - 1;
	}
	
	/*
	fprintf(stderr, "--------\nAfter:\n");
	for(u32 i = 0; i < 64; ++i)
	{
		if( i && i % 8 == 0 ) fprintf(stderr, "\n");
		fprintf(stderr, "$%X ", pifram[i]);
	}
	fprintf(stderr, "\n");
	*/
}

void n64::si_write(u32 addr, u32 v)
{
	addr = (addr&0x1F)>>2;
	if( addr > 6 ) return;
	if( addr == 6 )
	{	// SI_STATUS: writing clears interrupt
		clear_mi_bit(MI_INTR_SI_BIT);
		//printf("N64: SI irq cleared\n");
		return;
	}
	
	si_regs[addr] = v;
	
	if( addr == 1 )
	{	// SI_PIF_AD_RD64B: run the pif and read the results back to ram
		pif_run();
		memcpy(mem.data()+(si_regs[0]&0x7fffff), pifram, 64);
		//raise_mi_bit(MI_INTR_SI_BIT);
		//si_regs[0] += 60;  // "points to last word written to" ?
		si_cycles_til_irq = 0x2000;
		SI_STATUS |= 1;
		return;
	}
	
	if( addr == 4 )
	{	// SI_PIF_AD_WR64B
		memcpy(pifram, mem.data()+(si_regs[0]&0x7fffff), 64);
		//raise_mi_bit(MI_INTR_SI_BIT);
		//si_regs[0] += 60; // ??
		si_cycles_til_irq = 0x2000;
		SI_STATUS |= 1;
		return;
	}	
}

u32 n64::si_read(u32 addr)
{
	addr = (addr&0x1F)>>2;
	if( addr == 6 )
	{
		return SI_STATUS | ((MI_INTERRUPT & BIT(MI_INTR_SI_BIT))?BIT(12):0);
	}
	if( addr > 6 ) return 0;
	return si_regs[addr];
}




