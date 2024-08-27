#include <iostream>
#include <string>
#include <SDL.h>
#include "itypes.h"
#include "a26.h"

extern console* sys;

static u16 a26_timer_s = 0;
static u16 a26_timer_interval = 0;
static u16 a26_timer_intset = 0;
static u8 a26_timer_val = 0;
static u8 a26_INSTAT = 0;
static u8 SWCHA = 0xff;

void tia_set_button(u8 p0, u8 p1);

void keys()
{
	SWCHA = 0xff;
	auto K = SDL_GetKeyboardState(nullptr);
	if( K[SDL_SCANCODE_UP] )  { SWCHA ^= 0x10; }
	if( K[SDL_SCANCODE_DOWN]) { SWCHA ^= 0x20; }
	if( K[SDL_SCANCODE_LEFT]) { SWCHA ^= 0x40; }
	if( K[SDL_SCANCODE_RIGHT]){ SWCHA ^= 0x80; }
	u8 v = 0xff;
	if( K[SDL_SCANCODE_Z]){ v ^= 0x80; }
	tia_set_button(v, 0xff);
}


void a26_timer_step()
{
	a26_timer_s++;
	if( a26_timer_s == a26_timer_interval )
	{
		a26_timer_val--;
		a26_timer_s = 0;
		if( a26_timer_val == 0xFF )
		{
			a26_timer_interval = 1;
		}
	}
}

extern bool tia_frame_ready;
extern bool tia_wsync;
void tia_dot();

void a26::run_frame()
{
	keys();
	int cycles = 0;
	while( !tia_frame_ready && cycles++ < 262*76 )
	{
		if( !tia_wsync ) cpu.step();
		a26_timer_step();
		tia_dot();
		tia_dot();
		tia_dot();
	}
	tia_frame_ready = false;
}

void tia_write(u16 addr, u8 val);
u8 tia_read(u16);
void pia_write(u16 addr, u8 val)
{
	switch( addr )
	{
	case 0x284:
	case 0x294: a26_timer_intset = a26_timer_interval = 1; a26_timer_val = val-1; break;
	case 0x285:
	case 0x295: a26_timer_intset = a26_timer_interval = 8; a26_timer_val = val-1; break;
	case 0x286:
	case 0x296: a26_timer_intset = a26_timer_interval = 64; a26_timer_val = val-1; break;
	case 0x287:
	case 0x297: a26_timer_intset = a26_timer_interval = 1024; a26_timer_val = val-1; break;
	default:
		printf("PIA $%02X = $%02X\n", addr, val);
		break;
	}
	
}

void a26_write(u16 addr, u8 val)
{
	addr &= 0x1FFF;
	if( addr >= 0x1000 ) return;
	u8 A9 = (addr>>9)&1;
	u8 A7 = (addr>>7)&1;
	
	if( A7 == 0 )
	{
		tia_write(addr, val);
		return;
	}
	
	if( A9 == 0 )
	{
		((a26*)sys)->RAM[addr&0x7f] = val;
		return;
	} else {
		pia_write(addr, val);
		return;
	}
		
	printf("unimpl. write $%04X = $%02X\n", addr, val);
}

u8 a26_read(u16 addr)
{
	addr &= 0x1FFF;
	if( addr >= 0x1000 ) return ((a26*)sys)->ROM[addr&0xfff];
	u8 A9 = (addr>>9)&1;
	u8 A7 = (addr>>7)&1;
	
	if( A7 == 0 ) return tia_read(addr);
	if( A9 == 0 ) return ((a26*)sys)->RAM[addr&0x7f];
	
	if( A9 == 1 )
	{
		if( addr == 0x282 || addr == 0x292 ) return 0xff;
		if( addr == 0x280 || addr == 0x290 ) return SWCHA;
		if( addr == 0x284 || addr == 0x294 )
		{
			a26_timer_interval = a26_timer_intset;
			return a26_timer_val;
		}
		return 0xff;
	}
	
	printf("unimpl read $%02X\n", addr);
	return 0xff;		
}

a26::a26() : RAM(128)
{
	cpu.allow_decimal = false;
	cpu.read = a26_read;
	cpu.write = a26_write;
}

bool a26::loadROM(std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( ! fp ) return false;
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fsize += 0xfff;
	fsize &= ~0xfff;
	if( fsize < 2048 ) fsize = 2048;
	ROM.resize(fsize);
	[[maybe_unused]] int unu = fread(ROM.data(), 1, fsize, fp);
	fclose(fp);
	
	if( fsize == 2048 )
	{
		for(int i = 0; i < 2048; ++i) ROM[2048+i] = ROM[i];
	}
	return true;
}

extern bool tia_frame_ready;
extern bool tia_wsync;
void tia_dot();


