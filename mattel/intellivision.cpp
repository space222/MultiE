#include <vector>
#include <string>
#include <SDL.h>
#include "intellivision.h"

extern console* sys;
#define ITV (*dynamic_cast<intellivision*>(sys))

static u32 itv_palette[] = { 0x000000, 0x0827ff, 0xff3a06, 0xcad565, 0x017907, 0x02a91b, 0xfaeb25, 0xfffcff,
			     0xa8a9a9, 0x5bccff, 0xffa707, 0x395801, 0xff2c77, 0xbf96ff, 0x6dce2d, 0xc9107e   };

void intellivision::write(u16 addr, u16 val)
{
	if( in_vblank && addr == 0x20 ) { stic_strobe = true; return; }
	if( in_vblank && addr == 0x21 ) { stic_mode = 1; return; }
	if( in_vblank && addr < 0x30 ) { stic_regs[addr] = val; return; }
	if( addr == 0x1FE || addr == 0x1FF ) { printf("W$%X = $%X\n", addr, val); return; }
	if( addr >= 0x100 && addr < 0x200 ) { scratch[addr&0xff] = val; return; }
	if( addr >= 0x200 && addr < 0x360 ) { sysram[addr&0x1FF] = val; return; }
	if( !in_vblank && addr >= 0x3800 && addr < 0x4000 ) { gram[addr&0x3ff] = val; return; }	
	printf("W $%X = $%X\n", addr, val);
}

u16 intellivision::read(u16 addr)
{
	if( addr >= 0x1000 && addr < 0x2000 ) return exec[addr&0xfff];
	if( addr >= 0x5000 && addr < 0x7000 ) return ROM[addr-0x5000];
	printf("read <$%X\n", addr);
	
	if( addr == 0x21 ) { stic_mode = 0; return 0xffff; }
	if( addr < 0x30 ) { return stic_regs[addr]; }
	if( addr == 0x1FF || addr == 0x1FE ) { return read_left_cont(); }
	if( addr >= 0x100 && addr < 0x200 ) return scratch[addr&0xff];
	if( addr >= 0x200 && addr < 0x360 ) return sysram[addr&0x1ff];
	if( in_vblank && addr >= 0x3800 && addr < 0x4000 ) return gram[addr&0x3ff];
	if( addr >= 0x3000 && addr < 0x3800 ) return grom[addr&0x7ff];
	return 0xffff;
}

u8 intellivision::read_left_cont()
{
	u8 res = 0xff;
	auto K = SDL_GetKeyboardState(nullptr);
	
	if( K[SDL_SCANCODE_0] ) res &= ~0x88;
	if( K[SDL_SCANCODE_KP_PERIOD] ) res &= ~0x48;
	if( K[SDL_SCANCODE_KP_ENTER] ) res &= ~0x28;
	if( K[SDL_SCANCODE_1] ) res &= ~0x84;
	if( K[SDL_SCANCODE_2] ) res &= ~0x44;
	if( K[SDL_SCANCODE_3] ) res &= ~0x24;
	if( K[SDL_SCANCODE_4] ) res &= ~0x82;
	if( K[SDL_SCANCODE_5] ) res &= ~0x42;
	if( K[SDL_SCANCODE_6] ) res &= ~0x22;
	if( K[SDL_SCANCODE_7] ) res &= ~0x81;
	if( K[SDL_SCANCODE_8] ) res &= ~0x41;
	if( K[SDL_SCANCODE_9] ) res &= ~0x21;

	if( K[SDL_SCANCODE_W] ) res &= ~0xA0; // top fire button
	if( K[SDL_SCANCODE_A] ) res &= ~0x60; // left
	if( K[SDL_SCANCODE_D] ) res &= ~0xC0; // right

	
	return res;
}

void intellivision::run_frame()
{
	stic_strobe = false;
	in_vblank = false;
	while(cpu.stamp < 11115)
	{
		cpu.step();
	}
	cpu.stamp -= 11115;
	
	cpu.irq();
	in_vblank = true;
	while(cpu.stamp < 3819) cpu.step();
	cpu.stamp -= 3819;
	
	cpu.irq_asserted = false;
	
	if( !stic_strobe )
	{
		for(int i = 0; i < 159*192; ++i) fbuf[i] = 0;
		return;
	}
	stic_stack = 0;
	for(int line = 0; line < 96; ++line)
	{
		u8 cur_stack = stic_stack;
		for(int t = 0; t < 20; ++t)
		{
			u16 G = sysram[(line/8)*20 + t];
			u32 f= G&7;
			u32 b = (G>>9) & 3;
			u32 card=0;
			if( stic_mode )
			{
				card = (G>>3)&0x3F;
				if( G&(1<<11) )
				{
					card = gram[card*8 + (line&7)];
				} else {
					card = grom[card*8 + (line&7)];
				}
				if( G&(1<<12) ) b |= 8;
				if( G&(1<<13) ) b |= 4;	
			} else {
				card = (G>>3)&0xff;
				b=stic_regs[0x28+cur_stack]&0xf;
			
				if( G&(1<<11) )
				{
					card &= 0x3f;
					card = gram[card*8 + (line&7)];
					if( G&(1<<12) ) f |= 8;
				} else {
					card = grom[card*8 + (line&7)];
				}
				if( G & (1<<13) ) cur_stack++;
				cur_stack &= 3;
			}
			for(int x = 0; x < (t==19?7:8); ++x)
			{
				u32 color = (((card>>(7-(x&7)))&1) ? itv_palette[f] : itv_palette[b]) << 8;
				fbuf[(line*2)*159+t*8+x] = color;
				fbuf[(line*2+1)*159+t*8+x] = color;
			}
			if( line && (line&7) == 0 ) stic_stack = cur_stack;
		}
	}
}

void intellivision::reset()
{
	cpu.reset();
	for(int i = 0; i < 0x30; ++i) stic_regs[i] = 0;
}

bool intellivision::loadROM(std::string fname)
{
	FILE* exc = fopen("./bios/exec.bin", "rb");
	if( !exc ) return false;
	[[maybe_unused]] int unu = fread(exec, 1, 0x2000, exc);
	fclose(exc);
	for(int i = 0; i < 0x1000; ++i) exec[i] = (exec[i]>>8)|(exec[i]<<8);
	
	FILE* g = fopen("./bios/grom.bin", "rb");
	if(!g) return false;
	unu = fread(grom, 1, 0x800, g);
	fclose(g);
	
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return false;
	fseek(fp, 0, SEEK_END);
	auto rsize = ftell(fp);
	ROM.resize(rsize>>1);
	fseek(fp, 0, SEEK_SET);
	unu = fread(ROM.data(), 1, rsize, fp);
	fclose(fp);
	for(u32 i = 0; i < ROM.size(); ++i) ROM[i] = (ROM[i]>>8)|(ROM[i]<<8);
	
	return true;
}

void itv_write(u16 a, u16 v) { ITV.write(a,v); }
u16 itv_read(u16 a) { return ITV.read(a); }

intellivision::intellivision()
{
	cpu.read = itv_read;
	cpu.write = itv_write;
	fbuf.resize(159*192);
	stic_stack = stic_mode = 0;
	stic_strobe = false;
}

