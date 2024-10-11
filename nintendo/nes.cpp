#include <cstdio>
#include <cstdlib>
#include <string>
#include <SDL.h>
#include <iostream>
#include <chrono>
#include "Settings.h"
#include "nes.h"
using namespace std::literals;

extern SDL_AudioDeviceID audio_dev;
extern console* sys;
#define NES (*dynamic_cast<nes*>(sys))

void mapper_setup(nes*);

void nes::run_frame()
{
	while( !frame_complete )
	{
		cpu.step();
		apu_clock();
		ppu_dot();
		ppu_dot();
		ppu_dot();
		if( isFDS ) 
		{	
			fds_clock();
			cpu.irq_assert = (fds_timer_irq || fds_disk_irq);
		}
	}
	frame_complete = false;
}

void nes::write(u16 addr, u8 val)
{
	if( addr < 0x2000 )
	{
		RAM[addr&0x7ff] = val;
		return;
	}
	if( addr < 0x4000 )
	{
		ppu_write(addr&7, val);
		return;
	}
	if( addr == 0x4014 )
	{
		//for(int i = 0; i < 253; ++i) ppu_dot();
		u32 a = val<<8;
		for(int i = 0; i < 256; ++i)
		{
			OAM[i] = RAM[a+i];
		}
		return;
	}
	if( addr == 0x4016 )
	{
		if( val & 1 )
		{
			pad_strobe = 1;
			return;
		} else {
			if( pad_strobe )
			{
				pad_strobe = 0;
				pad1_index = pad2_index = 0;
			}
		}
		return;
	}
	if( addr == 0x4017 )
	{
		apu_4017 = val;
		return;
	}
	if( addr < 0x4018 )
	{
		apu_write(addr, val);
		return;
	}
	if( isFDS && addr < 0x5000 )
	{
		fds_reg_write(addr, val);
		return;
	}	
	if( addr >= 0x6000 && addr < (isFDS ? 0xE000 : 0x8000) )
	{
		if( !SRAM.size() ) return;
		if( SRAM.size() <= 0x2000 ) SRAM[addr&0x1fff] = val;
		else SRAM[addr-0x6000] = val;
		return;
	}
	if( addr >= 0x8000 && !isFDS ) 
	{
		mapper_write(this, addr, val);
		return;
	}
	//printf("W$%04X = $%02X\n", addr, val);
}

u8 nes::read(u16 addr)
{
	if( addr >= 0x8000 && !isFDS )
	{
		return prgbanks[(addr>>13)&3][addr&0x1fff];	
	}
	if( addr >= 0xE000 && isFDS )
	{
		return fds_bios[addr&0x1fff];
	}
	if( addr < 0x2000 ) return RAM[addr&0x7ff];
	if( addr < 0x4000 ) return ppu_read(addr&7);
	if( addr == 0x4016 )
	{
		u8 res = 0;
		auto K = SDL_GetKeyboardState(nullptr);
		switch( pad1_index )
		{
		case 7: if( K[SDL_SCANCODE_RIGHT] ) res = 1; break;
		case 6: if( K[SDL_SCANCODE_LEFT] ) res = 1; break;
		case 5: if( K[SDL_SCANCODE_DOWN] ) res = 1; break;
		case 4: if( K[SDL_SCANCODE_UP] ) res = 1; break;
		case 3: if( K[SDL_SCANCODE_S] ) res = 1; break;
		case 2: if( K[SDL_SCANCODE_A] ) res = 1; break;
		case 1: if( K[SDL_SCANCODE_X] ) res = 1; break;
		case 0: if( K[SDL_SCANCODE_Z] ) res = 1; break;
		default: break;
		}
		pad1_index++;
		return 0x40|res;
	}
	if( addr == 0x4017 ) return 0x40;
	if( isFDS && addr < 0x5000 )
	{
		return fds_reg_read(addr);
	}
	if( addr >= 0x6000 && addr < (isFDS ? 0xE000 : 0x8000) )
	{
		if( !SRAM.size() ) return 0xff;
		if( SRAM.size() <= 0x2000 ) return SRAM[addr&0x1fff];
		return SRAM[addr-0x6000];
	}
	//printf("R$%04X\n", addr);
	return 0xff;
}

bool nes::loadROM(std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( ! fp ) return false;
	
	[[maybe_unused]] int unu = fread(header, 1, 16, fp);
	
	// check for Famicon Disk System. Handled separately from mappers
	if( (header[0] == 'F' && header[1] == 'D' && header[2] == 'S')
		|| strncmp((const char*)(header+2), "NINTENDO", 8) == 0 )
	{
		isFDS = true;
		in_gap = false;
		fds_timer_irq = fds_disk_irq = false;
		mapper = 0;
		mapper_setup(this);
		fds_state_clocks = 0;
		fds_disk_stat = fds_ctrl = 0;
		floppy_pos = 0;
		
		u32 offset = (header[0] == 'F' && header[1] == 'D' && header[2] == 'S')?16:0;
		fseek(fp, 0, SEEK_END);
		auto fsz = ftell(fp);
		fseek(fp, offset, SEEK_SET);
		
		std::vector<u8> file(fsz);
		fds_sides = fsz/65500;
		unu = fread(file.data(), 1, fsz, fp);
		fclose(fp);
		
		load_floppy(file);
		
		FILE* fb = fopen("./bios/fds_bios.bin", "rb");
		if( ! fb )
		{
			printf("unable to load fds_bios.bin\n");
			return false;
		}
		unu = fread(fds_bios, 1, 8*1024, fb);
		fclose(fb);
		
		chrram = true;
		CHR.resize(8*1024);
		for(u32 i = 0; i < 8; ++i) chrbanks[i] = &CHR[i*1024];
		SRAM.resize(32*1024);		
		nametable[0] = &ntram[0];
		nametable[1] = nametable[0];
		nametable[2] = nametable[3] = &ntram[1024];
		return true;
	}
	
	isFDS = false;
	PRG.resize(header[4] * 16 * 1024);
	unu = fread(PRG.data(), 1, header[4] * 16 * 1024, fp);
		
	if( header[4] == 1 )
	{
		PRG.resize(32*1024);
		for(int i = 0; i < 16*1024; ++i) PRG[i+(16*1024)] = PRG[i];
	}
	
	if( header[5] )
	{
		chrram = false;
		CHR.resize(header[5] * 8 * 1024);
		unu = fread(CHR.data(), 1, header[5] * 8 * 1024, fp);
	} else {
		chrram = true;
		CHR.resize(8*1024);
	}
	fclose(fp);
	
	for(int i = 0; i < 8; ++i) chrbanks[i] = CHR.data() + (i * 0x400);
	//for(int i = 0; i < 4; ++i) prgbanks[i] = PRG.data() + (i * 8 * 1024);
	
	prgbanks[3] = PRG.data() + (PRG.size() - 0x2000);
	prgbanks[2] = PRG.data() + (PRG.size() - 0x4000);
	prgbanks[1] = PRG.data() + 0x2000;
	prgbanks[0] = PRG.data();
	
	mapper = header[6]>>4;
	mapper_setup(this);
	
	nametable[0] = &ntram[0];
	
	if( header[6] & 1 )
	{
		nametable[2] = nametable[0];
		nametable[1] = nametable[3] = &ntram[1024];
	} else {
		nametable[1] = nametable[0];
		nametable[2] = nametable[3] = &ntram[1024];
	}
	
	//std::string sdir = Settings::get<std::string>("dirs"s, "saves"s);
	//printf("sdir = <%s>\n", sdir.c_str());
	
	SRAM.resize(0x2000);
		
	return true;
}

void nes_write(u16 addr, u8 val) { NES.write(addr,val); }
u8 nes_read(u16 addr) { return NES.read(addr); }

void nes::reset()
{
	cpu.reset();
	for(int i = 0; i < 8; ++i) ppu_regs[i] = 0;
	for(int i = 0; i < 0x20; ++i) ppu_palmem[i] = 0;
	for(int i = 0; i < 0x100; ++i) OAM[i] = 0;
	scanline = 255;
	dot = 0;
	frame_complete = false;
	ppu_regs[0] = ppu_regs[1] = apu_stamp = apu_4017 = 0;
	apu_regs[0x15] = apu_regs[0x11] = 0;
	noise_shift = 1;
	apu_reset();
}

nes::nes()
{
	setVsync(false);
	fbuf.resize(256*240);
	cpu.read = nes_read;
	cpu.write = nes_write;
	pad1_index = pad2_index = pad_strobe = 0;
}


