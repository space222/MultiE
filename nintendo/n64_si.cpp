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
#define CMD_CMDLEN_INDEX 0
#define CMD_RESLEN_INDEX 1
#define CMD_COMMAND_INDEX 2
#define CMD_START_INDEX 3

void n64::pif_run()
{
	if( pifram[0x3f] & 0x10 ) { pifram[0x3f] = 0x80; pif_rom_enabled = false; return; }
	if( pifram[0x3f] & 0x40 ) { pifram[0x3f] = 0; return; }
	if( pifram[0x3f] & 0x20 ) { pifram[0x3f] = 0x80; return; }
	if( pifram[0x3f] & 8 ) { pifram[0x3f] = 0; return; }

	if( !(pifram[0x3f] & 1) ) return;
	//pifram[0x3f] = 0;
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
	
	int pif_channel = 0;
        int i = 0;
        while( i < 63 && pif_channel < 6 ) 
        {
            u8* cmd = &pifram[i++];
            u8 cmdlen = cmd[0] & 0x3F;

            if (cmdlen == 0) {
                pif_channel++;
            } else if (cmdlen == 0x3D) { // 0xFD in PIF RAM = send reset signal to this pif channel
               // pif_channel_reset(pif_channel);
                pif_channel++;
            } else if (cmdlen == 0x3E) { // 0xFE in PIF RAM = end of commands
                break;
            } else if (cmdlen == 0x3F) {
                continue;
            } else {
                u8 r = pifram[i++];
                if (r == 0xFE) { // 0xFE in PIF RAM = end of commands.
                    break;
                }
                u8 reslen = r & 0x3F; // TODO: out of bounds access possible on invalid data
                u8* res = &pifram[i + cmdlen];

                switch (cmd[CMD_COMMAND_INDEX]) {
                    case PIF_COMMAND_RESET:
                        //unimplemented(cmdlen != 1, "Reset with cmdlen != 1");
                        //unimplemented(reslen != 3, "Reset with reslen != 3");
                        //pif_controller_reset(cmd, res);
                        //pif_controller_id(cmd, res);
                        if( pif_channel == 4 )
                        {
                        	res[0] = 0;
                        	res[1] = 0x80;
                        	res[2] = 0;                        
                        } else {
	                        if( pif_channel ) res[0] = 0x80;
        	                else res[0] = 5;
        	                res[1] = 0;
        	                res[2] = 2;
        	        }
                        break;
                    case PIF_COMMAND_CONTROLLER_ID:
                       // unimplemented(cmdlen != 1, "Controller id with cmdlen != 1");
                       // unimplemented(reslen != 3, "Controller id with reslen != 3");
                        //pif_controller_id(cmd, res);
                        if( pif_channel == 4 )
                        {
                        	res[0] = 0;
                        	res[1] = 0x80;
                        	res[2] = 0;                        
                        } else { 
		                if( pif_channel ) res[0] = 0x80;
		                else res[0] = 5;
		                res[1] = 0;
		                res[2] = 2;
		        }
                        break;
                    case PIF_COMMAND_READ_BUTTONS:
                        //unimplemented(cmdlen != 1, "Read buttons with cmdlen != 1");
                        //unimplemented(reslen != 4, "Read buttons with reslen != 4");
                        //pif_read_buttons(cmd, res);
                        if( pif_channel == 0 )
                        {
                        auto keys = SDL_GetKeyboardState(nullptr);
				res[0] = 0;
				res[1] = 0;
				res[2] = 0;
				res[3] = 0;
				if( keys[SDL_SCANCODE_X] ) res[0] ^= 0x80; // A
				if( keys[SDL_SCANCODE_Z] ) res[0] ^= 0x40; // B
				if( keys[SDL_SCANCODE_A] ) res[0] ^= 0x20; // Z
				if( keys[SDL_SCANCODE_S] ) res[0] ^= 0x10; // Start
				//if( keys[SDL_SCANCODE_UP] ) res[0] ^= 8;
				//if( keys[SDL_SCANCODE_DOWN] ) res[0] ^= 4;
				//if( keys[SDL_SCANCODE_LEFT] ) res[0] ^= 2;
				//if( keys[SDL_SCANCODE_RIGHT] ) res[0] ^= 1;
				if( keys[SDL_SCANCODE_UP] ) res[3] = 120;
				else if( keys[SDL_SCANCODE_DOWN] ) res[3] = -120;
				if( keys[SDL_SCANCODE_LEFT] ) res[2] = -120;
				else if( keys[SDL_SCANCODE_RIGHT] ) res[2] = 120;
			} else {
				res[0] = res[1] = res[2] = res[3] = 0;	
			}
                        break;
                    case PIF_COMMAND_MEMPAK_READ:
                        //unimplemented(cmdlen != 3, "Mempak read with cmdlen != 3");
                        //unimplemented(reslen != 33, "Mempak read with reslen != 33");
                        //pif_mempak_read(cmd, res);
                        //memset(res, 0, reslen);
                        printf("Pak Read\n");
                        break;
                    case PIF_COMMAND_MEMPAK_WRITE:
                        //unimplemented(cmdlen != 35, "Mempak write with cmdlen != 35");
                        //unimplemented(reslen != 1, "Mempak write with reslen != 1");
                       // pif_mempak_write(cmd, res);
                       // memset(res, 0, reslen);
                       // res[0] = cmd[3]; // which byte in cmd?
                        printf("Pak Write\n");
                        break;
                    case PIF_COMMAND_EEPROM_READ:
                        //unimplemented(cmdlen != 2, "EEPROM read with cmdlen != 2");
                        //unimplemented(reslen != 8, "EEPROM read with reslen != 8");
                   //unimplemented(n64sys.mem.save_data == NULL, "EEPROM read when save data is uninitialized! Is this game in the game DB?");		
                       //memset(res, 0, reslen);
                        //pif_eeprom_read(cmd, res);
                        printf("EEPROM Read\n");
                        break;
                    case PIF_COMMAND_EEPROM_WRITE:
                       // unimplemented(cmdlen != 10, "EEPROM write with cmdlen != 10");
                        //unimplemented(reslen != 1,  "EEPROM write with reslen != 1");
                //unimplemented(n64sys.mem.save_data == NULL, "EEPROM write when save data is uninitialized! Is this game in the game DB?");
                        //pif_eeprom_write(cmd, res);
                       // memset(res, 0, reslen);
                       printf("EEPROM Write\n");
                        break;
                    default: break;
                       // logfatal("Invalid PIF command: %X", cmd[CMD_COMMAND_INDEX]);
                }
		pif_channel++;
                i += cmdlen + reslen;
	}
	}
	/*for(u32 i = 0, chan = 0; i < 63 && chan < 6; ++i)
	{
		u8 cmd = pifram[i] & 0x3F;
		if( pifram[i] == 0x3f || pifram[i] == 0x3d ) continue;
		if( pifram[i] == 0x3e ) break;
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
		chan+=1;
	}
	*/
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
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 64;  // "points to last word written to" ?
		//si_cycles_til_irq = 0x2000;
		//SI_STATUS |= 1;
		return;
	}
	
	if( addr == 4 )
	{	// SI_PIF_AD_WR64B
		memcpy(pifram, mem.data()+(si_regs[0]&0x7fffff), 64);
		raise_mi_bit(MI_INTR_SI_BIT);
		si_regs[0] += 64; // ??
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




