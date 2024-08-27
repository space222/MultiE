#include <iostream>
#include <string>
#include <unordered_map>
#include <SDL.h>
#include "console.h"
#include "Settings.h"
#include "6502.h"
#include "c64.h"

extern console* sys;
extern u32 palette_c64[]; // forward declared. defined eof

static u8 dd00 = 0;
static u8 d011 = 0;
static u8 d012 = 0;
static u8 d021 = 0;
static u8 cia_imask = 0;
static u8 cia_istat = 0;
static u8 pra = 0;
static u16 timer1 = 0;
static u8 timer1H = 0, timer1L = 0;
static u8 timer1ctrl = 0;
static u16 timer2 = 0;
static u8 timer2H = 0, timer2L = 0;
static u8 timer2ctrl = 0;

static u8 color_ram[0x400];

static bool tape_started = false;
static u64 tape_next_edge = 0;

void c64_draw_scanline(int line)
{
	c64 &C = *(c64*)sys;
	
	if( d011 & 0x20 )
	{
		for(u32 i = 0; i < 320; ++i)
		{
			const u32 cpos = (line>>3)*40 + (i>>3);
			const u8 b = 1 & (C.RAM[0x400 + line*40 + (i>>3)] >> (7-(i&7)));
			
			u32 color = 0;
			if( b )
			{
				color = palette_c64[color_ram[cpos] & 0xf];
			} else {
				color = palette_c64[color_ram[cpos]>>4];
			}
			C.fbuf[line*320 + i] = color<<8;	
		}	
		return;
	}
	
	for(u32 i = 0; i < 320; ++i)
	{
		const u32 cpos = (line>>3)*40 + (i>>3);
		const u8 ch = C.RAM[0x400 + cpos];
		
		const u8 b = 1 & (C.CHARSET[ch*8 + (line&7)] >> (7-(i&7)));
		
		u32 color = 0;
		if( b )
		{
			color = palette_c64[color_ram[cpos] & 0xf];
		} else {
			color = palette_c64[d021&0xf];
		}
		C.fbuf[line*320 + i] = color<<8;	
	}
}



u8 c64_io_read(u16 addr)
{
	if( addr == 0xD012 ) return d012;
	if( addr >= 0xD800 && addr < 0xDC00 ) return color_ram[addr&0x3ff];
	if( addr == 0xDC04 )
	{
		return timer1;
	}
	if( addr == 0xDC05 )
	{
		return timer1>>8;
	}
	if( addr == 0xDC06 )
	{
		return timer2;
	}
	if( addr == 0xDC07 )
	{
		return timer2>>8;
	}
	if( addr == 0xDD00 )
	{
		//printf("<$DD00\n");
		//dd00 ^= 0x40;
		u8 res = 0;
		if( dd00 & 8 ) res |= 0x80;
		return (dd00&0x3F) | res | (((dd00>>2)^0xc0)&0xc0);
	}
	if( addr == 0xDC0D )
	{
		//printf("<IO$DC0D\n");
		//printf("IRQ cleared!\n");
		((c64*)sys)->cpu.irq_assert = false;
		u8 retval = cia_istat;
		cia_istat = 0;
		return retval;
	}
	if( addr == 0xDC0E )
	{
		return timer1ctrl;
	}
	if( addr == 0xDC0F )
	{
		return timer2ctrl;
	}
	if( addr == 0xDC01 )
	{
		auto K = SDL_GetKeyboardState(nullptr);
		u8 ret = 0xff;
		if( !(pra & 0x80) )
		{
			u8 val = 0xff;
			//if( K[SDL_SCANCODE_LSHIFT] ) val ^= 0x80;
			if( K[SDL_SCANCODE_Q] ) val ^= 0x40;
			if( K[SDL_SCANCODE_APOSTROPHE] ) val ^= 0x20;
			if( K[SDL_SCANCODE_SPACE] ) val ^= 0x10;
			if( K[SDL_SCANCODE_2] ) val ^= 0x08;
			//if( K[SDL_SCANCODE_A] ) val ^= 0x04;
			//if( K[SDL_SCANCODE_W] ) val ^= 0x02;
			if( K[SDL_SCANCODE_1] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 0x20) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_COMMA] ) val ^= 0x80;
			if( K[SDL_SCANCODE_2] && K[SDL_SCANCODE_LSHIFT] ) val ^= 0x40;
			if( K[SDL_SCANCODE_SEMICOLON] && K[SDL_SCANCODE_LSHIFT] ) val ^= 0x20;
			if( K[SDL_SCANCODE_PERIOD] ) val ^= 0x10;
			if( K[SDL_SCANCODE_MINUS] ) val ^= 0x08;
			if( K[SDL_SCANCODE_L] ) val ^= 0x04;
			if( K[SDL_SCANCODE_P] ) val ^= 0x02;
			if( K[SDL_SCANCODE_EQUALS] && K[SDL_SCANCODE_LSHIFT] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 0x40) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_SLASH] ) val ^= 0x80;
			if( K[SDL_SCANCODE_6] && K[SDL_SCANCODE_LSHIFT] ) val ^= 0x40;
			if( K[SDL_SCANCODE_EQUALS] ) val ^= 0x20;
			if( K[SDL_SCANCODE_RSHIFT] ) val ^= 0x10;
			//if( K[SDL_SCANCODE_0] ) val ^= 0x08;
			if( K[SDL_SCANCODE_SEMICOLON] ) val ^= 0x04;
			if( K[SDL_SCANCODE_BACKSLASH] ) val ^= 0x02;
			//if( K[SDL_SCANCODE_9] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 0x10) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_N] ) val ^= 0x80;
			if( K[SDL_SCANCODE_O] ) val ^= 0x40;
			if( K[SDL_SCANCODE_K] ) val ^= 0x20;
			if( K[SDL_SCANCODE_M] ) val ^= 0x10;
			if( K[SDL_SCANCODE_0] ) val ^= 0x08;
			if( K[SDL_SCANCODE_J] ) val ^= 0x04;
			if( K[SDL_SCANCODE_I] ) val ^= 0x02;
			if( K[SDL_SCANCODE_9] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 8) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_V] ) val ^= 0x80;
			if( K[SDL_SCANCODE_U] ) val ^= 0x40;
			if( K[SDL_SCANCODE_H] ) val ^= 0x20;
			if( K[SDL_SCANCODE_B] ) val ^= 0x10;
			if( K[SDL_SCANCODE_8] ) val ^= 0x08;
			if( K[SDL_SCANCODE_G] ) val ^= 0x04;
			if( K[SDL_SCANCODE_Y] ) val ^= 0x02;
			if( K[SDL_SCANCODE_7] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 4) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_X] ) val ^= 0x80;
			if( K[SDL_SCANCODE_T] ) val ^= 0x40;
			if( K[SDL_SCANCODE_F] ) val ^= 0x20;
			if( K[SDL_SCANCODE_C] ) val ^= 0x10;
			if( K[SDL_SCANCODE_6] ) val ^= 0x08;
			if( K[SDL_SCANCODE_D] ) val ^= 0x04;
			if( K[SDL_SCANCODE_R] ) val ^= 0x02;
			if( K[SDL_SCANCODE_5] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 2) )
		{
			u8 val = 0xff;
			if( K[SDL_SCANCODE_LSHIFT] ) val ^= 0x80;
			if( K[SDL_SCANCODE_E] ) val ^= 0x40;
			if( K[SDL_SCANCODE_S] ) val ^= 0x20;
			if( K[SDL_SCANCODE_Z] ) val ^= 0x10;
			if( K[SDL_SCANCODE_4] ) val ^= 0x08;
			if( K[SDL_SCANCODE_A] ) val ^= 0x04;
			if( K[SDL_SCANCODE_W] ) val ^= 0x02;
			if( K[SDL_SCANCODE_3] ) val ^= 0x01;
			ret &= val;
		}
		if( !(pra & 1) )
		{
			u8 val = 0xff;
			//if( K[SDL_SCANCODE_LSHIFT] ) val ^= 0x80;
			//if( K[SDL_SCANCODE_E] ) val ^= 0x40;
			//if( K[SDL_SCANCODE_S] ) val ^= 0x20;
			//if( K[SDL_SCANCODE_Z] ) val ^= 0x10;
			//if( K[SDL_SCANCODE_4] ) val ^= 0x08;
			//if( K[SDL_SCANCODE_A] ) val ^= 0x04;
			if( K[SDL_SCANCODE_RETURN] ) val ^= 0x02;
			//if( K[SDL_SCANCODE_3] ) val ^= 0x01;
			ret &= val;
		}
		return ret;
	}
	//printf("%li:$%04X: IO <$%X\n", ((c64*)sys)->cpu.stamp, ((c64*)sys)->cpu.PC, addr);
	
	return 0;
}


void c64_io_write(u16 addr, u8 val)
{
	//printf("$%04X: IO$%04X = $%02X\n", ((c64*)sys)->cpu.PC, addr, val);
	if( addr >= 0xD800 && addr < 0xDC00 ) 
	{
		color_ram[addr&0x3ff] = val;
		return;
	}
	if( addr == 0xD011 )
	{
		d011 = val;
		return;
	}
	if( addr == 0xDC0D )
	{
		if( val & 0x80 ) cia_imask |= val;
		else cia_imask &= ~val;
		return;
	}
	if( addr == 0xDC04 )
	{
		timer1L = val;
		return;
	}
	if( addr == 0xDC05 )
	{
		timer1H = val;
		return;
	}
	if( addr == 0xDC06 )
	{
		timer2L = val;
		return;
	}
	if( addr == 0xDC07 )
	{
		timer2H = val;
		return;
	}
	if( addr == 0xD021 )
	{
		d021 = val;
		return;
	}
	if( addr == 0xDC0E )
	{
		timer1ctrl = val;
		if( val & 0x10 ) timer1 = timer1L | (timer1H<<8);
		//printf("%li: timer val = $%X\n", ((c64*)sys)->cpu.stamp, timer1);
		return;
	}
	if( addr == 0xDC0F )
	{
		timer2ctrl = val;
		if( val & 0x10 ) timer2 = timer2L | (timer2H<<8);
		return;
	}
	if( addr == 0xDD00 )
	{
		//printf("W$DD00 = $%X\n", val);
		dd00 = val;
	}
	if( addr == 0xDC00 )
	{
		pra = val;
		return;
	}
	
}

void c64_write(u16 addr, u8 val)
{
	c64& C = *(c64*)sys;
	if( addr == 1 )
	{
		u8 old = C.RAM[1]&0x10;
		C.RAM[1] = (C.RAM[1]&~C.RAM[0]) | (val&C.RAM[0]);
		C.RAM[1] &= ~0x10;
		C.RAM[1] |= old;
		return;
	}
	
	if( addr >= 0xD000 && addr < 0xE000 && (C.RAM[1]&4) )
	{
		c64_io_write(addr, val);
		return;
	}
	
	C.RAM[addr] = val;
	//printf("$%04X: W$%04X = $%02X\n", C.cpu.PC, addr, val);
}

bool wasdown = false;

u8 c64_read(u16 addr)
{
	c64& C = *(c64*)sys;
	if( addr == 1 )
	{
		auto K = SDL_GetKeyboardState(nullptr);
		if( K[SDL_SCANCODE_ESCAPE] )
		{
			if( !wasdown )
			{
				printf("got here\n");
				wasdown = true;
				C.RAM[1] ^= 0x10;
			}
		} else wasdown = false;
		printf("<1 = $%X\n", C.RAM[1]);
		return C.RAM[1];
	}
	if( addr < 0xA000 ) return C.RAM[addr];
	if( addr < 0xC000 )
	{
		if( C.RAM[1] & 1 )
		{
			return C.BASIC[addr&0x1FFF];
		} else {
			return C.RAM[addr];
		}
	}
	if( addr < 0xD000 ) return C.RAM[addr];
	if( addr < 0xE000 )
	{
		if( C.RAM[1] & 4 )
		{
			return c64_io_read(addr);
		} else {
			return C.CHARSET[addr&0xfff];
		}
	}
	if( C.RAM[1] & 2 )
	{
		return C.BIOS[addr&0x1FFF];
	}
	return C.RAM[addr];
}

void c64::run_frame()
{
	for(int line = 0; line < 312; ++line)
	{
		d012 = line;
		for(int i = 0; i < 63; ++i) 
		{
			cpu.step();
			if( !(RAM[1]&0x20) && cpu.stamp >= tape_next_edge )
			{
				if( tape_next_edge == 0 )
				{
					if( tape) tape_next_edge = cpu.stamp + tape->next();
				} else {
					cia_istat |= 0x90;
					tape_next_edge = cpu.stamp + tape->next();
				}
			}
			if( (timer1ctrl & 1) )
			{
				timer1--;
				if( timer1 == 0xffff )
				{
					cia_istat |= 0x81;
					if( timer1ctrl & 8 ) timer1ctrl &= 0xfe;
					timer1 = timer1L|(timer1H<<8);
				}
			}
			if( (timer2ctrl & 1) )
			{
				timer2--;
				if( timer2 == 0xffff )
				{
					cia_istat |= 0x82;
					if( timer2ctrl & 8 ) timer2ctrl &= 0xfe;
					timer2 = timer2L|(timer2H<<8);
				}
			}
			if( cia_istat & cia_imask & 0x1F )
			{
				cpu.irq_assert = true;
			}
		}
		if( line < 200 ) c64_draw_scanline(line);
	}
}

c64::c64() : cpu(), fbuf(320*240), RAM(64*1024), BIOS(8*1024), BASIC(8*1024), CHARSET(4*1024)
{
	cpu.read = c64_read;
	cpu.write = c64_write;
	RAM[1] = 0x3f;
	RAM[0] = 0x2f;
}

extern std::unordered_map<std::string, std::string> cli_options;

bool c64::loadROM(std::string)
{
	std::string biosdir = Settings::get<std::string>("dirs", "firmware");
	std::string biosfile = Settings::get<std::string>("c64", "bios");
	if( biosfile.empty() ) biosfile = "c64.bios";
	if( biosdir.empty() ) biosdir = "./bios/";
	if( biosdir.back() != '/' ) biosdir += '/';
	if( biosfile.find('/') == std::string::npos && biosfile.find('\\') == std::string::npos ) biosfile = biosdir + biosfile;

	FILE* fp = fopen(biosfile.c_str(), "rb");
	if( ! fp )
	{
		printf("Unable to open C64 BIOS <%s>\n", biosfile.c_str());
		printf("Expect ./bios/c64.bios, or use -c64-bios\n");
		return false;
	}
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if( fsize < 16*1024 )
	{
		printf("C64 BIOS file needs to be 16KB\nand consist of BASIC+KERNAL together\n");
		fclose(fp);
		return false;
	}
	[[maybe_unused]] int unu = fread(BASIC.data(), 1, 8*1024, fp);
	unu = fread(BIOS.data(), 1, 8*1024, fp);
	fclose(fp);
	
	std::string charset = "./bios/c64.font";
	if( cli_options.contains("c64-font") ) charset = cli_options["c64-font"];
	fp = fopen(charset.c_str(), "rb");
	if( ! fp )
	{
		printf("Unable to open C64 charset/font <%s>\n", charset.c_str());
		printf("Expect ./bios/c64.font, or use -c64-font\n");
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if( fsize < 4*1024 )
	{
		printf("C64 FONT file needs to be >=4KB\n");
		fclose(fp);
		return false;
	}
	unu = fread(CHARSET.data(), 1, 4*1024, fp);
	fclose(fp);
	
	if( cli_options.contains("c64-tape") )
	{
		tape = TAPFile::load(cli_options["c64-tape"]);
		if( tape == nullptr )
		{
			printf("Unable to load tape file\n");
			exit(1);
		}
	} else {
		tape = nullptr;
	}
	
	return true;
}

u32 palette_c64[] ={
	0x000000, 0xFFFFFF, 0x9F4E44, 0x6ABFC6, 0xA057A3, 0x5CAB5E, 0x50459B, 0xC9D487, 
	0xA1683C, 0x6D5412, 0xCB7E75, 0x626262, 0x898989, 0x9AE29B, 0x887ECB, 0xADADAD
};




