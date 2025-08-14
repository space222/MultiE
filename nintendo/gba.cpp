#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "gba.h"
#include "Settings.h"

#define DISPCNT lcd.regs[0]
#define DISPSTAT lcd.regs[2]
#define VCOUNT lcd.regs[3]
const u32 cycles_per_frame = 280896;

static void sized_write(u8* data, u32 addr, u32 v, int size)
{
	if( size == 8 ) data[addr] = v; 
	else if( size == 16 ) memcpy(data+(addr&~1), &v, 2);
	else memcpy(data+(addr&~3), &v, 4);
}

static u32 sized_read(u8* data, u32 addr, int size)
{
	if( size == 8 ) return data[addr]; 
	else if( size == 16 ) return *(u16*)&data[addr];
	else return *(u32*)&data[addr];
}

void gba::write(u32 addr, u32 v, int size, ARM_CYCLE ct)
{
	if( ct != ARM_CYCLE::X) cpu.stamp += 1;
	
	if( size == 8 ) v &= 0xff;
	else if( size == 16 ) v &= 0xffff;
	if( addr < 0x02000000 ) { return; } // write to BIOS ignored	
	if( addr < 0x03000000 ) { sized_write(ewram, addr&0x3FFFF, v, size); return; }
	if( addr < 0x04000000 ) { sized_write(iwram, addr&0x7fff, v, size);  return; }
	if( addr < 0x05000000 ) { write_io(addr, v, size); /*std::println("IO Write{} ${:X} = ${:X}", size, addr, v);*/ return;	}
	if( addr < 0x06000000 )
	{
		if( size == 8 ) { size = 16; v = (v&0xff)*0x101; addr &= ~1; }
		sized_write(palette, addr&0x3ff, v, size);
		return;
	}
	if( addr < 0x07000000 )
	{ // VRAM todo mirroring
		if( size == 8 ) { size = 16; addr &= ~1; v = (v&0xff)*0x101; }
		addr &= 0x1ffff;
		if( addr & 0x10000 ) addr &= ~0x8000;
		sized_write(vram, addr, v, size);
		return;
	}
	if( addr < 0x08000000 ) { sized_write(oam, addr&0x3ff, v, size); return; }
	
	if( addr == 0x080000C4 )
	{
		gpio_write(0, v);
		return;
	}
	if( addr == 0x080000C6 )
	{
		gpio_write(1, v);
		return;
	}
	if( addr == 0x080000C8 )
	{
		gpio_write(2, v);
		return;
	}
	
	if( addr >= 0x0d000000 && addr < 0x0e000000 && (save_type == SAVE_TYPE_UNKNOWN || save_type == SAVE_TYPE_EEPROM) )
	{
		eeprom_write(v);
		return;
	}
	
	if( addr >= 0x0e000000 && addr <= 0x0e00ffff )
	{
		if( save_type == SAVE_TYPE_UNKNOWN || save_type == SAVE_TYPE_SRAM ) 
		{
			save[addr&0xffff] = v;
			return;
		}
		
		if( save_type == SAVE_TYPE_UNKNOWN || save_type == SAVE_TYPE_FLASH )
		{
			if( addr == 0x0e005555 )
			{
				if( flash_state == 0 && v == 0xAA ) 
				{ 
					flash_state = 1; 
					save_type = SAVE_TYPE_FLASH;
					return;
				} 
				if( flash_state == 2 ) 
				{
					flash_state = 0;
					if( flash_cmd == 0x80 && v == 0x10 )
					{
						save_written = true;
						memset(save, 0xff, 0x20000);
						flash_cmd = 0;
						return;
					}
					if( flash_cmd == 0x90 && v == 0xf0 )
					{
						flash_cmd = 0;
						return;
					}
					flash_cmd = v;
					return;
				}
			}
			if( addr == 0x0e002AAA && v == 0x55 && flash_state == 1 ) { flash_state = 2; return; }
			
			if( flash_cmd == 0x80 && v == 0x30 )
			{
				flash_cmd = 0;
				flash_state = 0;
				addr &= 0xf000;
				save_written = true;
				for(u32 i = 0; i < 0x1000; ++i) save[(flash_bank<<16)|(addr+i)] = 0xff;			
				return;
			}
			
			if( flash_cmd == 0xB0 && addr == 0x0e000000 )
			{
				flash_bank = v&1;
				flash_cmd = 0;
				return;
			}
			
			if( flash_cmd == 0xA0 )
			{
				save_written = true;
				save[(flash_bank<<16)|(addr&0xffff)] = v;
				flash_cmd = 0;
			}
			return;	
		}
		return;	
	}
	std::println("${:X} = ${:X}", addr, v);
} //end of write

u32 gba::read(u32 addr, int size, ARM_CYCLE ct)
{
	if( ct != ARM_CYCLE::X) cpu.stamp += 1;
	
	if( addr < 0x02000000ul )
	{ // BIOS
		if( cpu.r[15] < 0x4000 ) return bios_open_bus = sized_read(bios, addr&0x3fff, size);
		//std::println("BIOS read with PC = ${:X}", cpu.r[15]);
		return bios_open_bus;
	}
	if( addr < 0x03000000ul )
	{ // WRAM_lo
		return sized_read(ewram, addr&0x3ffff, size);	
	}
	if( addr < 0x04000000ul )
	{ // WRAM_hi
		return sized_read(iwram, addr&0x7fff, size);
	}
	if( addr < 0x05000000ul )
	{ // IO
		return read_io(addr, size);
	}
	if( addr < 0x06000000ul )
	{ // palette
		return sized_read(palette, addr&0x3ff, size);
	}
	if( addr < 0x07000000ul )
	{ // VRAM
		if( addr & 0x10000 ) addr &= ~0x8000;
		return sized_read(vram, addr&0x1ffff, size);
	}
	if( addr < 0x08000000ul )
	{ // OAM
		return sized_read(oam, addr&0x3ff, size);
	}
	if( gpio_reads_en )
	{
		if( addr == 0x080000C4 ) { return gpio_read(0); }
		if( addr == 0x080000C6 ) { return gpio_read(1); }
		if( addr == 0x080000C8 ) { return gpio_read(2); }
	}
	if( addr < 0x10000000ul )
	{  // either ROM or various types of SaveRAM that isn't supported yet
		if( (save_type == SAVE_TYPE_UNKNOWN || save_type == SAVE_TYPE_EEPROM)
			&& addr >= 0x0d000000 && addr < 0x0e000000 ) return eeprom_read();
		if( addr >= 0x0e000000 && addr <= 0x0e00FFFF )
		{
			if( save_type == SAVE_TYPE_FLASH && flash_cmd == 0x90 && addr == 0x0E000000 ) return save_size==128 ? 0x62 : 0x32;
			if( save_type == SAVE_TYPE_FLASH && flash_cmd == 0x90 && addr == 0x0E000001 ) return save_size==128 ? 0x13 : 0x1B;
			if( save_type != SAVE_TYPE_EEPROM )
			{
				return save[(flash_bank<<16)|(addr&0xffff)];
			}
			return save[(flash_bank<<16)|(addr&0xffff)];
		}
		addr &= 0x01ffFFFf;
		if( addr > ROM.size() )
		{
			addr %= ROM.size();
		}
		//printf("ROM Read8 %X\n", addr);
		return sized_read(ROM.data(), addr, size);
	}
	
	//std::println("Garbage read{} ${:X}", size, addr);
	return 0;	
}

static u32 snd_out_cycles = 380;
static u64 last_stamp = 0;

void gba::reset()
{
	cpu.read = [&](u32 a, int s, ARM_CYCLE c) -> u32 { return read(a,s,c); };
	cpu.write= [&](u32 a, u32 v, int s, ARM_CYCLE c) { write(a,v,s,c); };
	for(int i = 0; i < 16; ++i) cpu.r[i] = 0;
	cpu.reset();
	halted = false;
	
	tmr[0].ctrl = tmr[1].ctrl = tmr[2].ctrl = tmr[3].ctrl = 0;
	tmr[0].last_read = tmr[1].last_read = tmr[2].last_read = tmr[3].last_read = 0;
	
	sched->reset();
	sched->add_event(280896, EVENT_FRAME_COMPLETE);
	sched->add_event(0, EVENT_SND_OUT);
	sched->add_event(0, EVENT_SCANLINE_START);
	VCOUNT = 227;
	
	gpio_reads_en = false;
	
	ISTAT = IMASK = IME = 0;
	
	snd_fifo_a.clear();
	snd_fifo_b.clear();
	
	pcmA = pcmB = 0;
	dma_sound_mix = 0;
	snd_master_en = 0;
	snd_out_cycles = last_stamp = 0;
	flash_state = flash_bank = flash_cmd = 0;
	
	memset(iwram, 0, 32*1024);
	memset(ewram, 0, 256*1024);
	memset(lcd.regs, 0, 48*2);
	memset(dmaregs, 0, 2*32);
	memset(vram, 0, 96*1024);
	
	lcd.regs[0x10] = lcd.regs[0x11] = lcd.regs[0x12] = lcd.regs[0x13] = 0x100;
	lcd.regs[0x18] = lcd.regs[0x19] = lcd.regs[0x1A] = lcd.regs[0x1B] = 0x100;
	
	if( save_written )
	{
		FILE* fs = fopen(savefile.c_str(), "wb");
		if( !fs )
		{
			std::println("GBA: Unable to write out save data to <{}>", savefile);
			return;
		}
		[[maybe_unused]] int unu = fwrite(save, 1, 0x20000, fs);
		fclose(fs);
	}
	save_written = false;
}

gba::gba() : sched(new Scheduler(this)), lcd(vram, palette, oam)
{
	setVsync(false);
}

void gba::run_frame()
{
	frame_complete = false;
	while( !frame_complete )
	{
		if( !halted )
		{
			cpu.step();
		} else {
			if( sched->next_stamp() == 0xffffFFFFffffFFFFull )
			{
				std::println("CPU Halted with no future events");
				exit(1);
			}
			cpu.stamp = sched->next_stamp();
		}
		while( cpu.stamp >= sched->next_stamp() )
		{
			sched->run_event();
		}
	}
}

bool gba::loadROM(const std::string fname)
{
	{
		std::string biosfile = Settings::get<std::string>("gba", "bios");
		if( biosfile.empty() ) biosfile = "bios/GBABIOS.BIN";
		FILE* fbios = fopen(biosfile.c_str(), "rb");
		if( !fbios )
		{
			std::println("Unable to open gba bios file <{}>", biosfile);
			return false;
		}
		[[maybe_unused]] int unu = fread(bios, 1, 16*1024, fbios);
		fclose(fbios);
	}

	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsz);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsz, fp);
	fclose(fp);
	
	savefile = fname.substr(0, fname.rfind('.'));
	savefile += ".sav";
	save_written = false;
	
	FILE* fs = fopen(savefile.c_str(), "rb");
	if( fs )
	{
		unu = fread(save, 1, 0x20000, fs);
		fclose(fs);
	} else {
		memset(save, 0xff, 0x20000);
	}
	
	save_size = 0;
	if( ROM[0xAC] == 'F' )
	{
		save_type = SAVE_TYPE_EEPROM;
		save_size = 9;
	} else {
		save_type = SAVE_TYPE_UNKNOWN;
		for(u32 i = 0; i < ROM.size(); i+=4)
		{
			if( !strncmp((const char*)ROM.data()+i, "EEPROM_V", 8) )
			{
				save_type = SAVE_TYPE_EEPROM;
				break;
			}
			if( !strncmp((const char*)ROM.data()+i, "SRAM_V", 6) )
			{
				save_type = SAVE_TYPE_SRAM;
				break;
			}
			if( !strncmp((const char*)ROM.data()+i, "FLASH_V", 7) || !strncmp((const char*)ROM.data()+i, "FLASH512_V",10) )
			{
				save_type = SAVE_TYPE_FLASH;
				save_size = 64;
				break;
			}
			if( !strncmp((const char*)ROM.data()+i, "FLASH1M_V", 9) )
			{
				save_type = SAVE_TYPE_FLASH;
				save_size = 128;
				break;
			}
		}
		std::println("Save type {:X} detected", save_type);
	}
	
	return true;	
}

u16 gba::getKeys()
{
	auto keys = SDL_GetKeyboardState(nullptr);
	u16 val = 0x3ff;
	if( keys[SDL_SCANCODE_Q] ) val ^= BIT(9);
	if( keys[SDL_SCANCODE_W] ) val ^= BIT(8);
	if( keys[SDL_SCANCODE_DOWN] ) val ^= BIT(7);
	if( keys[SDL_SCANCODE_UP] ) val ^= BIT(6);
	if( keys[SDL_SCANCODE_LEFT] ) val ^= BIT(5);
	if( keys[SDL_SCANCODE_RIGHT] ) val ^= BIT(4);
	if( keys[SDL_SCANCODE_S] ) val ^= BIT(3);
	if( keys[SDL_SCANCODE_A] ) val ^= BIT(2);
	if( keys[SDL_SCANCODE_X] ) val ^= BIT(1);
	if( keys[SDL_SCANCODE_Z] ) val ^= BIT(0);
	return val;
}

void gba::check_irqs()
{
	cpu.irq_line = IME && (ISTAT & IMASK & 0x3fff);
	if( halted ) halted = !(ISTAT & IMASK & 0x3fff);
}

void gba::event(u64 old_stamp, u32 evc)
{
	if( evc == 0 ) return;
	
	if( evc == EVENT_TMR0_CHECK )
	{
		timer_event(old_stamp,0);
		return;
	}
	if( evc == EVENT_TMR1_CHECK )
	{
		timer_event(old_stamp,1);	
		return;
	}
	if( evc == EVENT_TMR2_CHECK )
	{
		timer_event(old_stamp,2);	
		return;
	}
	if( evc == EVENT_TMR3_CHECK )
	{
		timer_event(old_stamp,3);	
		return;
	}
	
	if( evc == EVENT_SCANLINE_START )
	{
		VCOUNT += 1;
		if( VCOUNT >= 228 ) VCOUNT = 0;
		
		if( VCOUNT == 160 )
		{
			if( (dmaregs[5]&0xB000)   == 0x9000 ) { exec_dma(0); }
			if( (dmaregs[6+5]&0xB000) == 0x9000 ) { exec_dma(1); }
			if( (dmaregs[12+5]&0xB000)== 0x9000 ) { exec_dma(2); }
			if( (dmaregs[18+5]&0xB000)== 0x9000 ) { exec_dma(3); }		
		}

		DISPSTAT &= ~7;
		DISPSTAT |= (VCOUNT >= 160 && VCOUNT != 228) ? 1 : 0;
		DISPSTAT |= (VCOUNT == (DISPSTAT>>8)) ? BIT(2) : 0;
		if( VCOUNT == 160 && (DISPSTAT & BIT(3)) )
		{
			ISTAT |= BIT(0);
			check_irqs();
		}
		if( VCOUNT == (DISPSTAT>>8) && (DISPSTAT & BIT(5)) )
		{
			ISTAT |= BIT(2);
			check_irqs();
		}
		
		sched->add_event(old_stamp + 1006, EVENT_HBLANK_START);
		sched->add_event(old_stamp + 1232, EVENT_SCANLINE_START);	
		return;
	}
	
	if( evc == EVENT_HBLANK_START )
	{
		DISPSTAT |= 2;
		if( DISPSTAT & BIT(4) )
		{
			ISTAT |= BIT(1);
			check_irqs();
		}
		if( VCOUNT < 160 )
		{
			lcd.draw_scanline(VCOUNT);
			if( (dmaregs[5]&0xB000)   == 0xA000 ) { exec_dma(0); }
			if( (dmaregs[6+5]&0xB000) == 0xA000 ) { exec_dma(1); }
			if( (dmaregs[12+5]&0xB000)== 0xA000 ) { exec_dma(2); }
			if( (dmaregs[18+5]&0xB000)== 0xA000 ) { exec_dma(3); }
			lcd.bg2x += (s16)lcd.regs[0x11];
			lcd.bg2y += (s16)lcd.regs[0x13];
			lcd.bg3x += (s16)lcd.regs[0x19];
			lcd.bg3y += (s16)lcd.regs[0x1B];
		}
		return;
	}
	
	if( evc == EVENT_FRAME_COMPLETE )
	{
		frame_complete = true;
		memcpy(&lcd.bg2x, &lcd.regs[0x14], 4); lcd.bg2x = (lcd.bg2x<<4)>>4;
		memcpy(&lcd.bg2y, &lcd.regs[0x16], 4); lcd.bg2y = (lcd.bg2y<<4)>>4;
		memcpy(&lcd.bg3x, &lcd.regs[0x1C], 4); lcd.bg3x = (lcd.bg3x<<4)>>4;
		memcpy(&lcd.bg3y, &lcd.regs[0x1E], 4); lcd.bg3y = (lcd.bg3y<<4)>>4;
		sched->add_event(old_stamp+280896, EVENT_FRAME_COMPLETE);
		return;
	}

	if( evc == EVENT_SND_OUT )
	{
		sched->add_event(old_stamp+380, EVENT_SND_OUT);
		float sample = pcmA + pcmB;
		sample /= 2.f;
		audio_add(sample, sample);
		return;
	}
	
	std::println("event called with {}", evc);
	exit(1);
}

u32 gba::eeprom_read()
{
	if( eeprom_mode != 3 ) return 1;
	if( eeprom_count == 68 ) { eeprom_state = eeprom_mode = eeprom_count = 0; return 1; }
	if( eeprom_count < 4 ) { eeprom_count++; return 1; }
	u32 bit = eeprom_count-4;
	u8 ret = save[(eeprom_addr<<3)+(((bit/8))&7)]>>(7^(bit&7));
	eeprom_count += 1;
	return ret&1;
}

void gba::eeprom_write(u8 v)
{
	if( eeprom_state == 0 )
	{ // grab mode bits
		eeprom_mode <<= 1;
		eeprom_mode |= v&1;
		eeprom_mode &= 3;
		eeprom_count += 1;
		if( eeprom_count >= 2 )
		{
			if( eeprom_mode != 2 && eeprom_mode != 3 ) { eeprom_mode = eeprom_count = 0; return; }
			eeprom_state = 1;
			eeprom_addr = eeprom_count = 0;
		}
		return;
	}
	
	if( eeprom_state == 1 )
	{  // grab address
		eeprom_addr <<= 1;
		eeprom_addr |= v&1;
		eeprom_count += 1;
		if( eeprom_count == save_size+(eeprom_mode==3?1:0))
		{ // writing an address has an extra 'done'ish bit for read mode
			eeprom_state = 2;
			eeprom_count = 0;
			if( eeprom_mode == 3 ) eeprom_addr >>= 1; // get rid of said extra bit
		}
		return;
	}
	
	if( eeprom_mode == 2 && eeprom_state == 2 )
	{
		if( eeprom_count < 64 )
		{
			eeprom_out <<= 1;
			eeprom_out |= v&1;
			eeprom_count += 1;		
		} else {
			*(u64*)&save[eeprom_addr<<3] = __builtin_bswap64(eeprom_out);
			save_written = true;
			eeprom_mode = eeprom_state = eeprom_count = 0;
		}
		return;	
	}	
}

gba::~gba()
{
	if( save_written )
	{
		FILE* fs = fopen(savefile.c_str(), "wb");
		if( !fs )
		{
			std::println("GBA: Unable to write out save data to <{}>", savefile);
			return;
		}
		[[maybe_unused]] int unu = fwrite(save, 1, 0x20000, fs);
		fclose(fs);
	}
	delete sched;
}


