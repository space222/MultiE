#include <print>
#include <cstring>
#include <cstdio>
#include "n64.h"

#define PIF_COMMAND_CONTROLLER_ID 0x00
#define PIF_COMMAND_READ_BUTTONS  0x01
#define PIF_COMMAND_MEMPAK_READ   0x02
#define PIF_COMMAND_MEMPAK_WRITE  0x03
#define PIF_COMMAND_EEPROM_READ   0x04
#define PIF_COMMAND_EEPROM_WRITE  0x05
#define PIF_COMMAND_RESET         0xFF

void n64::pif_parse()
{
	u32 i = 0;
	u32 chan = 0;
	for(u32 c = 0; c < 5; ++c) pifchan[c].start = 0xff;
	while( i < 63 && chan < 5 )
	{
		u8 v = pifram[i++];
		if( v == 0xfe ) { break; }
		if( v == 0xff || v == 0xfd ) { continue; }
		if( v == 0 ) { chan+=1; continue; }
		
		pifchan[chan].start = i-1;
		i += (pifram[i]&0x3f) + (pifram[i+1]&0x3f) + 1;
		if( i > 64 ) pifchan[chan].start = 0xff;
		chan+=1;
	}
}

void n64::pif_shake()
{
	for(u32 c = 0; c < 5; ++c)
	{
		if( pifchan[c].start == 0xff ) continue;
		u8 i = pifchan[c].start;
		u8 TX = pifram[i++];
		if( TX & 0x80 ) continue;
		TX &= 0x3F;
		u8 RX = pifram[i++] & 0x3F;
		u8 CMD = pifram[i++];
		pif_device(c, TX-1, RX, CMD, i, i+TX-1);		
	}
}

void n64::pif_device(u32 c, u8 tx, u8 rx, u8 cmd, u8 ts, u8 rs)
{
	if( c < 4 )
	{
		switch( cmd )
		{
		case PIF_COMMAND_RESET:
		case PIF_COMMAND_CONTROLLER_ID:
			if( 1 ) // c == 0 )
			{
				pifram[rs] = 5;
				pifram[rs+1] = 0;
				pifram[rs+2] = 2;
			}
			return;
		case PIF_COMMAND_MEMPAK_READ:
			std::println("Mempak read");
			for(u32 i = 0; i < rx; ++i) pifram[rs+i] = 0;
			return;
		case PIF_COMMAND_READ_BUTTONS:
			pifram[rs] = pifram[rs+1] = pifram[rs+2] = pifram[rs+3] = 0;
			if( c != 0 ) return;
			auto keys = SDL_GetKeyboardState(nullptr);
			if( getInputState(1, 0) ) pifram[rs+0] ^= 0x80; // A
			if( getInputState(1, 1) ) pifram[rs+0] ^= 0x40; // B
			if( getInputState(1, 2) ) pifram[rs+0] ^= 0x20; // Z
			if( getInputState(1, 3) ) pifram[rs+0] ^= 0x10; // Start
			if( getInputState(1, 6) ) pifram[rs+0] ^= 8;
			if( getInputState(1, 7) ) pifram[rs+0] ^= 4;
			if( getInputState(1, 8) ) pifram[rs+0] ^= 2;
			if( getInputState(1, 9) ) pifram[rs+0] ^= 1;
			if( keys[SDL_SCANCODE_KP_8] ) pifram[rs+1] ^= 8;
			if( keys[SDL_SCANCODE_KP_4] ) pifram[rs+1] ^= 4;
			if( keys[SDL_SCANCODE_KP_2] ) pifram[rs+1] ^= 2;
			if( keys[SDL_SCANCODE_KP_6] ) pifram[rs+1] ^= 1;
			if( getInputState(1, 10) ) pifram[rs+3] = 120;
			else if( getInputState(1, 11) ) pifram[rs+3] = -120;
			if( getInputState(1, 12) ) pifram[rs+2] = -120;
			else if( getInputState(1, 13) ) pifram[rs+2] = 120;
		}	
		return;
	}
	if( cmd == PIF_COMMAND_RESET || cmd == PIF_COMMAND_CONTROLLER_ID )
	{
		pifram[rs] = 0;
		pifram[rs+1] = 0x80;
		pifram[rs+2] = 0;
		return;
	}
	if( cmd == PIF_COMMAND_EEPROM_READ )
	{
		u32 page = pifram[ts];
		for(u32 n = 0; n < 8; ++n) pifram[rs+n] = eeprom[page*8 + n];
		return;
	}
	if( cmd == PIF_COMMAND_EEPROM_WRITE )
	{
		u32 page = pifram[ts];
		for(u32 n = 0; n < 8; ++n) eeprom[page*8 + n] = pifram[ts+1+n];
		pifram[rs] = 0;
		eeprom_written = true;
		return;
	}
	for(u32 i = 0; i < rx; ++i) pifram[rs+i] = 0;
}

void n64::pif_run()
{
	if( pifram[0x3f] & 0x10 ) { pifram[0x3f] = 0x80; pif_rom_enabled = false; return; }
	if( pifram[0x3f] & 0x40 ) { pifram[0x3f] = 0; return; }
	if( pifram[0x3f] & 0x20 ) { pifram[0x3f] = 0x80; return; }
	if( pifram[0x3f] & 8 ) { pifram[0x3f] = 0; return; }

	if( pifram[0x3f] & 1 ) 
	{
		pif_parse();
		pifram[0x3f] = 0;
		return;
	}	
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
		pif_shake();
		memcpy(mem.data()+(si_regs[0]&0x7fffff), pifram, 64);
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 60;  // "points to last word written to" ?
		//si_cycles_til_irq = 0x2000;
		//SI_STATUS |= 1;
		return;
	}
	
	if( addr == 4 )
	{	// SI_PIF_AD_WR64B
		memcpy(pifram, mem.data()+(si_regs[0]&0x7fffff), 64);
		pif_run();
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 60; // ??
		//si_cycles_til_irq = 0x2000;
		//SI_STATUS |= 1;
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




