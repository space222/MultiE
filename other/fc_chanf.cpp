#include <vector>
#include "fc_chanf.h"
extern console* sys;

void fc_chanf::write(u16 addr, u8 val)
{
	if( addr >= 0x800 + rom.size() )
	{
		ram[addr] = val;
		return;
	}
}

void fc_chanf::out(u16 p, u8 val)
{
	if( p == 0 )
	{
		if( (port[0]&BIT(5)) && !(val&BIT(5)) )
		{
			plot_pixel();
		}
		port[0] &= 0xf;
		port[0] |= val & 0x60;
		return;
	}
	if( p < 6 ) 
	{
		port[p] = val;
		return;
	}
	printf("chanF: out $%X = $%X\n", p, val);
}

u8 fc_chanf::read(u16 addr)
{
	if( addr < 0x800 ) return bios[addr];
	if( addr < 0x800 + rom.size() ) return rom[addr-0x800];
	return ram[addr];
}

u8 fc_chanf::in(u16 p)
{
	if( p > 5 ) printf("chanF: in $%X\n", p);
	if( p == 0 )
	{
		u8 v = port[0];
		v |= 0xf;
		auto keys = SDL_GetKeyboardState(nullptr);
		if( keys[SDL_SCANCODE_Z] ) v ^= 1;
		if( keys[SDL_SCANCODE_X] ) v ^= 2;
		if( keys[SDL_SCANCODE_C] ) v ^= 4;
		if( keys[SDL_SCANCODE_V] ) v ^= 8;
		return v;	
	}
	if( p == 5 ) return port[5];
	return 0xff;
}

static u32 fcpal[] = { 0, 0xeeEEee00, 0x00004400, 0x00440000 };

void fc_chanf::run_frame()
{
	const u32 CYCLES_PER_FRAME = 17897725/60;
	u64 target = last_target + CYCLES_PER_FRAME;
	while( stamp < target )
	{
		stamp += cpu.step();
	}
	last_target = target;
	
	for(u32 y = 0; y < 64; ++y)
	{
		for(u32 x = 0; x < 124; ++x)
		{
			fbuf[y*128 + x] = fcpal[vram[y*128 + x]&3];
		}
	}
}

void fc_chanf::plot_pixel()
{
	u32 x = port[4]&0x7f;
	x ^= 0x7f;
	u32 y = port[5]&0x3f;
	y ^= 0x3f;
	vram[y*128 + x] = port[1]>>6;
}

bool fc_chanf::loadROM(const std::string fname)
{
	FILE* fb = fopen("./bios/fc_chanf.bios", "rb");
	if( !fb )
	{
		printf("Unable to load fc_chanf.bios\n");
		return false;
	}
	[[maybe_unused]] int unu = fread(bios, 1, 8*1024, fb);
	fclose(fb);
	
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to load ROM <%s>\n", fname.c_str());
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	rom.resize(fsz);
	unu = fread(rom.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void fc_chanf::reset()
{
	stamp = last_target = 0;
	memset(vram, 0, 128*64);
	memset(port, 0xff, 6);
	cpu.reset();
	cpu.read = [](u16 a) { return dynamic_cast<fc_chanf*>(sys)->read(a); };
	cpu.in = [](u16 p) { return dynamic_cast<fc_chanf*>(sys)->in(p); };
	cpu.write=[](u16 a, u8 v) { dynamic_cast<fc_chanf*>(sys)->write(a,v); };
	cpu.out=[](u16 p, u8 v) { dynamic_cast<fc_chanf*>(sys)->out(p,v); };
}



