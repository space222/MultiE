#include <cstdio>
#include <string>
#include <iostream>
#include <SDL.h>
#include "dmg.h"

extern console* sys;

#define GB (*dynamic_cast<dmg*>(sys))
#define LCDC IO[0x40]
#define STAT IO[0x41]
#define LY   IO[0x44]
#define LYC  IO[0x45]
#define IF   IO[0x0F]
#define IE   IO[0xFF]

static u32 dmgpal[] = { 0xFFFFFF00, 0xC0C0C000, 0x60606000, 0x20202000 };

void dmg::scanline(int line)
{
	const u16 bgmap = (LCDC&8) ? 0x1c00 : 0x1800;
	const u16 bgtiles = (LCDC&0x10) ? 0 : 0x1000;
	
	u8 bgdata[160];
	
	for(int p = 0; p < 160; ++p)
	{
		const int y = (line + IO[0x42])&0xff;
		const int x = (p + IO[0x43])&0xff;
		const u16 n = VRAM[bgmap + ((y/8)*32) + (x/8)];
		u8 t1=0,t2=0;
		if( LCDC&0x10 )
		{
			t1 = VRAM[bgtiles + (n*16) + ((y&7)*2)];
			t2 = VRAM[bgtiles + (n*16) + ((y&7)*2) + 1];
		} else {
			t1 = VRAM[bgtiles + (s8(n)*16) + ((y&7)*2)];
			t2 = VRAM[bgtiles + (s8(n)*16) + ((y&7)*2) + 1];
		}
		u8 b1 = (t1 >> (7^(x&7))) & 1;
		u8 b2 = (t2 >> (7^(x&7))) & 1;
		b1 |= b2<<1;
		bgdata[p] = b1;
	}
	
	u8 sprpix[160];
	u8 sprind[160];
	u8 sprpri[160];
	memset(sprpix, 0, 160);
	memset(sprind, 0xff, 160);
	memset(sprpri, 0, 160);
	int ysize = (LCDC&4) ? 16 : 8;
	int numfound = 0;
	for(int p = 0; p < 160; ++p)
	{
		for(u32 spr = 0; spr < 40 && numfound < 10; ++spr)
		{
			int Y = OAM[spr*4];
			Y -= 16;
			if( !(line >= Y && line - Y < ysize) ) continue;
			
			int X = OAM[spr*4 + 1];
			X -= 8;
			if( !(p >= X && p - X < 8) ) continue;		
			u8 tile = OAM[spr*4 + 2];
			u8 L = line - Y;
			if( OAM[spr*4+3]&0x40 ) L ^= ysize-1;
			if( LCDC & 4 ) tile &= 0xFE;
			/*if( LCDC & 4 )
			{	
				tile &= 0xfe;
				if( L >= 8 )
				{
					tile |= 1;
					L -= 8;
				}				
			}*/
			u8 x = 7^((p-X)&7);
			if( OAM[spr*4+3]&0x20 ) x ^= 7;
			u8 b = (VRAM[tile*16 + (L*2)] >> x)&1;
			b |= ((VRAM[tile*16 + (L*2) + 1] >> x)&1)<<1;
			if( b )
			{
				sprpix[p] = b | (OAM[spr*4 + 3]&0x10);
				sprind[p] = spr;
				sprpri[p] = OAM[spr*4 + 3] & 0x80;
				break;
			}
		}
	}
	
	for(int x = 0; x < 160; ++x)
	{
		if( (sprpix[x]&3) && !(sprpri[x]&0x80) )
		{
			u8 pal = IO[0x48];
			if( sprpix[x]&0x10 ) pal = IO[0x49];
			sprpix[x] &= 3;
			pal >>= sprpix[x]*2;
			pal &= 3;
			fbuf[line*160 + x] = dmgpal[pal];
			continue;
		}
		if( bgdata[x] )
		{
			u8 pal = IO[0x47];
			pal >>= bgdata[x]*2;
			pal &= 3;
			fbuf[line*160 + x] = dmgpal[pal];
			continue;
		}
		if( sprpix[x]&3 )
		{
			u8 pal = IO[0x48];
			if( sprpix[x]&0x10 ) pal = IO[0x49];
			sprpix[x] &= 3;
			pal >>= sprpix[x]*2;
			pal &= 3;
			fbuf[line*160 + x] = dmgpal[pal];
			continue;
		}
		u8 bgc = IO[0x47]&3;
		fbuf[line*160 + x] = dmgpal[bgc];
	}
}

u8 dmg_read(u16);

void dmg_io_write(u16 addr, u8 val)
{
	if( addr >= 0xff80 )
	{
		GB.IO[addr&0xff] = val;
		return;
	}

	if( addr == 0xff44 ) return;
	if( addr == 0xff04 )
	{
		GB.DIV = 0;
		return;
	}
	if( addr == 0xff41 ) 
	{
		GB.STAT &= 7;
		GB.STAT |= val&~7;
		return;
	}
	if( addr == 0xff46 )
	{
		addr = val<<8;
		for(int i = 0; i < 0xA0; ++i) GB.OAM[i] = dmg_read(addr+i);
		return;
	}
	if( addr == 0xff0f )
	{
		GB.IO[0xF] = val&0x1F;
		return;	
	}
	
	GB.IO[addr&0xff] = val;
	//printf("IO $%X = $%X\n", addr, val);
}

u8 dmg_io_read(u16 addr)
{
	if( addr >= 0xff80 ) return GB.IO[addr&0xff];
	
	if( addr == 0xff00 )
	{
		GB.IO[0] &= 0x30;
		auto keys = SDL_GetKeyboardState(nullptr);
		if( GB.IO[0] & 0x10 )
		{
			u8 K = (keys[SDL_SCANCODE_S]<<3)|(keys[SDL_SCANCODE_A]<<2) 
					| (keys[SDL_SCANCODE_X]<<1) | keys[SDL_SCANCODE_Z];	
			K ^= 0xF;
			return GB.IO[0]|K;
		} else if( GB.IO[0] & 0x20 ) {
			u8 K = (keys[SDL_SCANCODE_DOWN]<<3)|(keys[SDL_SCANCODE_UP]<<2)
					| (keys[SDL_SCANCODE_LEFT]<<1)|keys[SDL_SCANCODE_RIGHT];
			K ^= 0xF;
			return GB.IO[0]|K;
		}
		return GB.IO[0]|0xf;
	}
	if( addr == 0xff04 ) return GB.DIV>>8;
	if( addr != 0xff44 ) printf("IO <$%X\n", addr);
	return GB.IO[addr&0xff];
}

void mapper_write(u16 addr, u8 val)
{
	switch( GB.mapper )
	{
	case NO_MBC: 
		if( addr >= 0xA000 && addr < 0xC000 ) 
		{
			GB.SRAM[(GB.rambank*0x2000)|(addr&0x1fff)] = val;
			return;
		}
		return;
	case MBC1:
		if( addr >= 0x2000 && addr < 0x4000 ) 
		{
			GB.rombank = val&0x1F;
			if( GB.rombank >= GB.num_rombanks ) GB.rombank %= GB.num_rombanks;
			if( GB.rombank == 0 ) GB.rombank = 1;
			return;
		}
		return;
	case MBC3:
		if( addr >= 0x2000 && addr < 0x4000 ) 
		{
			GB.rombank = val&0x7F;
			if( GB.rombank == 0 ) GB.rombank = 1;
			//else if( GB.rombank >= GB.num_rombanks ) GB.rombank %= GB.num_rombanks;
			return;
		} else if( addr >= 0x4000 && addr < 0x6000 ) {
			if( val < 4 )
			{
				GB.rambank = val;
				//if( GB.rambank >= GB.num_rambanks ) GB.rambank %= GB.num_rambanks;
				GB.rtc_regs_active = false;
			} else {
				GB.rtc_regs_active = false;
			}
			return;
		}
		return;
	default:
		printf("Unimpl mapper %i\n", GB.mapper);
		exit(1);	
	}
}

u8 dmg_read(u16 addr)
{
	if( addr >= 0xFF00 ) return dmg_io_read(addr);
	if( addr >= 0xFEA0 ) return 0; //unusable range
	if( addr >= 0xFE00 ) 
	{
		if( 1 /*(STAT & 3) < 2 || !(LCDC&0x80)*/ ) return GB.OAM[addr&0xff];
		//return 0xff;
	}
	
	if( addr >= 0xC000 ) return GB.RAM[addr&0x1fff];
	if( addr >= 0xA000 )
	{
		if( GB.rtc_regs_active ) return 0;
		if( GB.ram_type == NO_RAM ) return 0;
		if( GB.ram_type == MBC2_RAM )
		{
			return GB.SRAM[addr&0x1ff] & 0xf;		
		}
		if( GB.ram_type == KB2 )
		{
			return GB.SRAM[addr&0x3ff];
		}
		
		return GB.SRAM[(GB.rambank*0x2000) + (addr&0x1fff)];
	}
	
	if( addr >= 0x8000 )
	{
		//if( ((STAT & 3) == 3) && (LCDC & 0x80) ) return 0xff;
		return GB.VRAM[addr&0x1fff];	
	}
	
	if( addr >= 0x4000 )
	{
		return GB.ROM[(GB.rombank*0x4000)+(addr&0x3fff)];
	}
	return GB.ROM[addr&0x3fff];
}

void dmg_write(u16 addr, u8 val)
{
	if( addr >= 0xFF00 )
	{
		dmg_io_write(addr, val);
		return;
	}

	if( addr >= 0xFEA0 ) return; // unusable range

	if( addr >= 0xFE00 )
	{
		if( 1 /* ((STAT & 3) > 1) || !(LCDC&0x80) */ ) GB.OAM[addr&0xff] = val;
		return;
	}
	
	if( addr >= 0xC000 )
	{
		GB.RAM[addr&0x1fff] = val;
		return;
	}
	
	if( addr >= 0xA000 )
	{
		if( GB.rtc_regs_active ) return;
		if( GB.ram_type == NO_RAM ) return;
		if( GB.ram_type == MBC2_RAM )
		{
			GB.SRAM[addr&0x1ff] = val & 0xf;
		} else if( GB.ram_type == KB2 ) {
			GB.SRAM[addr&0x7ff] = val;
		} else {
			GB.SRAM[(GB.rambank*0x2000)|(addr&0x1fff)] = val;
		}
		return;
	}
	
	if( addr >= 0x8000 )
	{
		//if( (STAT & 3) == 3 && (LCDC & 0x80) ) return;
		GB.VRAM[addr&0x1fff] = val;
		return;
	}
	
	mapper_write(addr, val);
}

void system_cycles(int);

void dmg::run_frame()
{
	while( cpu.stamp < 70224 )
	{
		auto cc = cpu.step();
		system_cycles(cc);
	}
	cpu.stamp -= 70224;
	return;
	/*
	if( !(LCDC&0x80) )
	{
		for(int line = 0; line < 154; ++line) 
		{
			LY = line;
			while(cpu.stamp < 114*4) DIV += cpu.step();
			cpu.stamp -= 114*4;
		}
	}

	GB.IO[0x41] &= ~7;
	GB.IO[0x41] |= 3;
	for(int line = 0; line < 154; ++line)
	{
		LY = line;
		STAT &=~4;
		if( LY == LYC )
		{
			STAT |= 4;
			if( STAT & 0x40 ) IF |= 2;
		}
		
		if( line < 144 )
		{
			STAT &= ~7;
			STAT |= 2;
			while( cpu.stamp < 80 ) DIV += cpu.step();
			STAT &= ~7;
			STAT |= 3;
			while( cpu.stamp < 172+80 ) DIV += cpu.step();
			STAT &= ~7;
			while( cpu.stamp < 172+80+204 ) DIV += cpu.step();
			cpu.stamp -= 172+80+204;
			scanline(line);
		} else {
			while( cpu.stamp < 456 ) DIV += cpu.step();
			cpu.stamp -= 456;
		}

		if( line == 144 )
		{
			GB.IO[0x41] &= ~7;
			GB.IO[0x41] |= 1;
			IF |= 1;
			//printf("IF = $%X, IE = $%X\n", GB.IO[0xf], GB.IO[0xff]);
			if( STAT & 0x10 ) IF |= 2;  // STAT has it's own mode-based vblank interrupt
		}
	}
	*/
}

bool dmg::loadROM(std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	ROM.resize(fsize);

	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsize, fp);
	fclose(fp);
	
	switch( ROM[0x147] )
	{
	case 8:
	case 9:
	case 0: mapper = NO_MBC; break;
	case 1:
	case 2:
	case 3: mapper = MBC1; break;
	case 5:
	case 6: mapper = MBC2;  break;
	case 0xF:  //hasRTC = true;
	case 0x10: //hasRTC = true;
	case 0x11:
	case 0x12:
	case 0x13: mapper = MBC3; break;
	case 0x19: 
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E: mapper = MBC5; break;
	case 0x20: mapper = MBC6; break;
	case 0x22: mapper = MBC7; break;
	}
	
	switch( ROM[0x148] )
	{
	case 0: num_rombanks = 1; break;
	case 1: num_rombanks = 4; break;
	case 2: num_rombanks = 8; break;	
	case 3: num_rombanks = 16; break;	
	case 4: num_rombanks = 32; break;
	case 5: num_rombanks = 64; break;
	case 6: num_rombanks = 128; break;
	case 7: num_rombanks = 256; break;
	case 8: num_rombanks = 512; break;
	case 0x52: num_rombanks = 72; break;
	case 0x53: num_rombanks = 80; break;
	case 0x54: num_rombanks = 96; break;	
	default:
		printf("Unsupported bank count = $%X\n", ROM[0x148]);
		exit(1);
	}
	
	printf("RAM type: 0x%x\n", ROM[0x149]);
	switch( ROM[0x149] )
	{
	case 0: if( mapper == MBC2 ) ram_type = MBC2_RAM; else ram_type = NO_RAM; break;
	case 1: ram_type = KB2; break;
	case 2: ram_type = BANKS1; break;
	case 3: ram_type = BANKS4; break;
	case 4: ram_type = BANKS16; break;
	case 5: ram_type = BANKS8; break;	
	}
	
	switch( ROM[0x149] )
	{
	case 0:
	case 1:
	case 2: num_rambanks = 1; break;
	case 3: num_rambanks = 4; break;
	case 4: num_rambanks = 16; break;
	case 5: num_rambanks = 8; break;
	default:
		printf("Unimpl number of ram banks = $%X\n", ROM[0x149]);
		exit(1);	
	}
	
	printf("mapper = $%X\n", ROM[0x147]);
	//todo: attempt to load bootrom, etc
	return true;
}

dmg::dmg()
{
	RAM.resize(1024*8);
	SRAM.resize(1024*128);
	VRAM.resize(1024*8);
	fbuf.resize(160*144);
	cpu.read = dmg_read;
	cpu.write= dmg_write;
	mapper = 0;
	rombank = 1;
	rambank = 0;
	num_rombanks = 2;
	num_rambanks = 1;
	bootrom_active = true;
	rtc_regs_active = false;
}

void dmg::reset()
{
	cpu.reset();
	cpu.halted = false;
	cpu.ime = false;
	//LY = 80;
	IO[0x40] = 0x80;
	rombank = 1;
	rambank = 0;
	cpu.stamp = 0;
}

